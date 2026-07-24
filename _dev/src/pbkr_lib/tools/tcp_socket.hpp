#pragma once
// tools/tcp_socket.hpp — Wrapper RAII autour des sockets POSIX (Linux uniquement :
// utilise MSG_NOSIGNAL, absent sur macOS/BSD).
//
// Ne connait rien a HTTP ni au WebSocket : juste "envoyer/recevoir des octets sur une
// connexion TCP", avec un mode non bloquant pour laisser l'appelant piloter sa propre
// boucle d'evenements (WebSocketClient utilisera waitReadable() dans sa boucle run()/poll()).

#include <cstdint>
#include <cstddef>
#include <sys/types.h> // ssize_t
#include <string>
#include <vector>

class TcpSocket {
public:
    TcpSocket() = default;
    ~TcpSocket();

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;

    // Resout host (nom d'hote ou adresse IPv4/IPv6 litterale) et etablit la connexion TCP.
    // Bloquant le temps de la connexion. Bascule ensuite le socket en mode non bloquant
    // et desactive l'algorithme de Nagle (TCP_NODELAY : on echange des messages JSON
    // complets, la latence prime sur le regroupement de petits paquets).
    // Leve std::runtime_error en cas d'echec (DNS, connexion refusee, timeout...).
    void connect(const std::string& host, uint16_t port);

    // Envoie l'integralite du buffer, en reessayant/attendant si le socket n'est pas
    // immediatement pret en ecriture (EAGAIN/EWOULDBLOCK). Leve en cas d'erreur ou de
    // timeout d'ecriture (writeTimeoutMs, 5000 ms par defaut).
    void sendAll(const uint8_t* data, size_t len, int writeTimeoutMs = 5000);
    void sendAll(const std::vector<uint8_t>& data, int writeTimeoutMs = 5000);
    void sendAll(const std::string& data, int writeTimeoutMs = 5000);

    // Tentative de lecture non bloquante : jusqu'a maxLen octets dans buffer.
    // Retour >= 1 : nombre d'octets lus.
    // Retour == 0 : le pair a ferme la connexion proprement (EOF).
    // Retour == -1 avec wouldBlock=true : aucune donnee disponible pour l'instant,
    //   pas une erreur — l'appelant doit reessayer plus tard (ex. apres waitReadable()).
    // Leve std::runtime_error pour toute autre erreur.
    ssize_t recv(uint8_t* buffer, size_t maxLen, bool& wouldBlock);

    // Bloque jusqu'a ce que le socket ait des donnees a lire, ou jusqu'a timeoutMs.
    // Retourne true si des donnees sont disponibles, false si le delai a expire.
    bool waitReadable(int timeoutMs);

    // Ferme la connexion si elle est ouverte. Idempotent, aussi appele par le destructeur.
    void close();

    bool isOpen() const { return fd_ >= 0; }

    // Acces bas niveau au descripteur, pour un appelant qui voudrait multiplexer
    // plusieurs sockets lui-meme (poll()/select() sur plusieurs fd a la fois).
    int fd() const { return fd_; }

private:
    int fd_ = -1;
};
