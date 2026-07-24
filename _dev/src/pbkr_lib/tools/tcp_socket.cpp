#include "tools/tcp_socket.hpp"

#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

TcpSocket::~TcpSocket() {
    close();
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
        close();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

void TcpSocket::connect(const std::string& host, uint16_t port) {
    close();

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* results = nullptr;
    std::string portStr = std::to_string(port);
    int rc = ::getaddrinfo(host.c_str(), portStr.c_str(), &hints, &results);
    if (rc != 0) {
        throw std::runtime_error("TcpSocket::connect: resolution DNS echouee pour '" + host +
                                  "': " + gai_strerror(rc));
    }

    int lastErrno = 0;
    int s = -1;

    for (addrinfo* ai = results; ai != nullptr; ai = ai->ai_next) {
        s = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (s < 0) {
            lastErrno = errno;
            continue;
        }

        std::cout<< "Try to connect to " << host <<":" << port << std::endl;
        if (::connect(s, ai->ai_addr, ai->ai_addrlen) == 0) {
            break;
        }

        lastErrno = errno;
        ::close(s);
        s = -1;
    }

    ::freeaddrinfo(results);

    if (s < 0) {
        throw std::runtime_error("TcpSocket::connect: impossible de se connecter a " + host + ":" +
                                  portStr + " (" + std::strerror(lastErrno) + ")");
    }

    fd_ = s;

    // Bascule en non bloquant : send()/recv() ulterieurs renvoient EAGAIN plutot que de bloquer,
    // l'appelant pilote sa propre boucle via waitReadable().
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags >= 0) {
        ::fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
    }

    int one = 1;
    ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
}

void TcpSocket::sendAll(const uint8_t* data, size_t len, int writeTimeoutMs) {
    if (fd_ < 0) {
        throw std::runtime_error("TcpSocket::sendAll: socket non connecte");
    }

    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(fd_, data + sent, len - sent, MSG_NOSIGNAL);

        if (n > 0) {
            sent += static_cast<size_t>(n);
            continue;
        }

        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            pollfd pfd{};
            pfd.fd = fd_;
            pfd.events = POLLOUT;
            int pr = ::poll(&pfd, 1, writeTimeoutMs);
            if (pr == 0) {
                throw std::runtime_error("TcpSocket::sendAll: timeout d'ecriture (socket plein)");
            }
            if (pr < 0) {
                if (errno == EINTR) continue;
                throw std::runtime_error(std::string("TcpSocket::sendAll: erreur poll(): ") +
                                          std::strerror(errno));
            }
            continue;
        }

        if (n < 0 && errno == EINTR) {
            continue;
        }

        throw std::runtime_error(std::string("TcpSocket::sendAll: erreur send(): ") + std::strerror(errno));
    }
}

void TcpSocket::sendAll(const std::vector<uint8_t>& data, int writeTimeoutMs) {
    if (!data.empty()) {
        sendAll(data.data(), data.size(), writeTimeoutMs);
    }
}

void TcpSocket::sendAll(const std::string& data, int writeTimeoutMs) {
    if (!data.empty()) {
        sendAll(reinterpret_cast<const uint8_t*>(data.data()), data.size(), writeTimeoutMs);
    }
}

ssize_t TcpSocket::recv(uint8_t* buffer, size_t maxLen, bool& wouldBlock) {
    wouldBlock = false;

    if (fd_ < 0) {
        throw std::runtime_error("TcpSocket::recv: socket non connecte");
    }

    ssize_t n = ::recv(fd_, buffer, maxLen, 0);
    if (n >= 0) {
        return n; // 0 = fermeture propre par le pair
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        wouldBlock = true;
        return -1;
    }

    if (errno == EINTR) {
        wouldBlock = true; // rien de recu cette fois, l'appelant peut reessayer immediatement
        return -1;
    }

    throw std::runtime_error(std::string("TcpSocket::recv: erreur recv(): ") + std::strerror(errno));
}

bool TcpSocket::waitReadable(int timeoutMs) {
    if (fd_ < 0) {
        return false;
    }

    pollfd pfd{};
    pfd.fd = fd_;
    pfd.events = POLLIN;

    int pr = ::poll(&pfd, 1, timeoutMs);
    if (pr < 0) {
        if (errno == EINTR) return false;
        throw std::runtime_error(std::string("TcpSocket::waitReadable: erreur poll(): ") +
                                  std::strerror(errno));
    }

    return pr > 0 && (pfd.revents & POLLIN) != 0;
}

void TcpSocket::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}
