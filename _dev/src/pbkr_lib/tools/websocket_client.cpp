#include "tools/websocket_client.hpp"
#include "tools/ws_handshake.hpp"

#include <stdexcept>
#include <chrono>

void WebSocketClient::connect(const std::string& host, uint16_t port, const std::string& path) {
    socket_.connect(host, port);

    std::string key = WsHandshake::generateKey();
    std::string hostHeader = host + ":" + std::to_string(port);
    std::string request = WsHandshake::buildRequest(hostHeader, path, key);

    socket_.sendAll(request);

    std::string response = readHttpResponseHeaders(5000);

    if (!WsHandshake::validateResponse(response, key)) {
        socket_.close();
        throw std::runtime_error("WebSocketClient::connect: handshake invalide (statut HTTP ou "
                                  "Sec-WebSocket-Accept incorrect)");
    }

    state_ = WsState::Open;
}

std::string WebSocketClient::readHttpResponseHeaders(int timeoutMs) {
    std::string acc;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

    while (true) {
        size_t headerEnd = acc.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            // Ce qui suit la fin des en-tetes appartient deja au flux de trames websocket
            // (rare mais possible si le serveur envoie son message Hello tres vite) : on le garde.
            std::string leftover = acc.substr(headerEnd + 4);
            recvBuffer_.insert(recvBuffer_.end(), leftover.begin(), leftover.end());
            return acc.substr(0, headerEnd + 4);
        }

        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            throw std::runtime_error("WebSocketClient::connect: timeout en attente de la "
                                      "reponse HTTP du handshake");
        }

        int remaining = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count());

        if (!socket_.waitReadable(remaining)) {
            continue; // le tour de boucle suivant revalidera le deadline
        }

        uint8_t buf[4096];
        bool wouldBlock = false;
        ssize_t n = socket_.recv(buf, sizeof(buf), wouldBlock);

        if (n == 0) {
            throw std::runtime_error("WebSocketClient::connect: connexion fermee par le "
                                      "serveur pendant le handshake");
        }
        if (n > 0) {
            acc.append(reinterpret_cast<char*>(buf), static_cast<size_t>(n));
        }
    }
}

void WebSocketClient::onMessage(MessageHandler handler) { messageHandler_ = std::move(handler); }
void WebSocketClient::onClose(CloseHandler handler) { closeHandler_ = std::move(handler); }
void WebSocketClient::onError(ErrorHandler handler) { errorHandler_ = std::move(handler); }

void WebSocketClient::sendFrame(WsOpcode opcode, const uint8_t* data, size_t len, bool fin) {
    WsFrame frame;
    frame.fin = fin;
    frame.opcode = opcode;
    frame.payload.assign(data, data + len);

    // RFC 6455 5.1 : un client DOIT masquer toutes les trames qu'il envoie.
    auto encoded = WsFrameCodec::encode(frame, /*masked=*/true);
    socket_.sendAll(encoded);
}

void WebSocketClient::sendText(const std::string& text) {
    if (state_ != WsState::Open) {
        throw std::runtime_error("WebSocketClient::sendText: connexion non ouverte");
    }
    sendFrame(WsOpcode::Text, reinterpret_cast<const uint8_t*>(text.data()), text.size());
}

void WebSocketClient::sendBinary(const std::vector<uint8_t>& data) {
    if (state_ != WsState::Open) {
        throw std::runtime_error("WebSocketClient::sendBinary: connexion non ouverte");
    }
    sendFrame(WsOpcode::Binary, data.data(), data.size());
}

void WebSocketClient::sendCloseFrame(uint16_t code, const std::string& reason) {
    if (closeSent_) return;

    std::vector<uint8_t> payload;
    payload.push_back(static_cast<uint8_t>((code >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>(code & 0xFF));
    payload.insert(payload.end(), reason.begin(), reason.end());

    try {
        sendFrame(WsOpcode::Close, payload.data(), payload.size());
    } catch (...) {
        // La connexion TCP est peut-etre deja rompue : on l'ignore, la fermeture va se
        // finaliser cote appelant de toute facon (recv() renverra 0 ou une erreur au prochain poll()).
    }
    closeSent_ = true;
}

void WebSocketClient::closeConnection(uint16_t code, const std::string& reason, int timeoutMs) {
    if (state_ == WsState::Closed) return;
    sendCloseFrame(code, reason);
    state_ = WsState::Closing;

    if (timeoutMs > 0) {
        closeDeadlineActive_ = true;
        closeDeadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    } else {
        closeDeadlineActive_ = false;
    }
}

void WebSocketClient::failConnection(const std::string& message) {
    if (errorHandler_) errorHandler_(message);
    sendCloseFrame(1002, "protocol error");
    state_ = WsState::Closed;
    socket_.close();
}

void WebSocketClient::deliverMessage(WsOpcode opcode, const std::vector<uint8_t>& payload) {
    if (messageHandler_) {
        std::string text(payload.begin(), payload.end());
        messageHandler_(text, opcode == WsOpcode::Binary);
    }
}

void WebSocketClient::handleFrame(const WsFrame& frame) {
    switch (frame.opcode) {
        case WsOpcode::Text:
        case WsOpcode::Binary: {
            if (fragmented_) {
                // RFC 6455 5.4 : aucune nouvelle trame Text/Binary ne doit commencer avant
                // la fin (fin=true) du message fragmente en cours.
                failConnection("WebSocketClient: trame Text/Binary recue avant la fin du "
                                "message fragmente precedent");
                return;
            }
            if (frame.fin) {
                deliverMessage(frame.opcode, frame.payload);
            } else {
                fragmented_ = true;
                fragmentedOpcode_ = frame.opcode;
                fragmentedPayload_ = frame.payload;
            }
            break;
        }

        case WsOpcode::Continuation: {
            if (!fragmented_) {
                failConnection("WebSocketClient: trame Continuation recue hors de tout "
                                "message fragmente");
                return;
            }
            fragmentedPayload_.insert(fragmentedPayload_.end(),
                                       frame.payload.begin(), frame.payload.end());
            if (frame.fin) {
                deliverMessage(fragmentedOpcode_, fragmentedPayload_);
                fragmented_ = false;
                fragmentedPayload_.clear();
            }
            break;
        }

        case WsOpcode::Ping: {
            // RFC 6455 5.5.2 : repondre par un Pong avec le meme payload, des que possible.
            try {
                sendFrame(WsOpcode::Pong, frame.payload.data(), frame.payload.size());
            } catch (...) {
                // La connexion est de toute facon compromise si l'envoi echoue ; le prochain
                // poll() le detectera via recv().
            }
            break;
        }

        case WsOpcode::Pong: {
            // Rien d'obligatoire (RFC 6455 5.5.3). Pourrait alimenter un suivi de
            // keepalive/latence dans une version future.
            break;
        }

        case WsOpcode::Close: {
            closeReceived_ = true;

            uint16_t code = 1005; // RFC 6455 7.4.1 : "No Status Rcvd" si aucun code fourni
            std::string reason;
            if (frame.payload.size() >= 2) {
                code = static_cast<uint16_t>((frame.payload[0] << 8) | frame.payload[1]);
                reason.assign(frame.payload.begin() + 2, frame.payload.end());
            }

            if (!closeSent_) {
                // RFC 6455 5.5.1 : repondre par une trame Close en echo pour completer
                // le closing handshake.
                sendCloseFrame(code, "");
            }

            state_ = WsState::Closed;
            socket_.close();

            if (closeHandler_) closeHandler_(code, reason);
            break;
        }
    }
}

void WebSocketClient::poll(int timeoutMs) {
    if (state_ == WsState::Closed) return;

    if (state_ == WsState::Closing && closeDeadlineActive_ &&
        std::chrono::steady_clock::now() >= closeDeadline_) {
        // Le pair n'a jamais renvoye sa trame Close en echo : on force la fermeture plutot
        // que de rester bloque en "Closing" indefiniment (RFC 6455 7.1.1 ne garantit aucun
        // delai maximal cote pair, donc rien ne l'oblige a repondre vite, voire a repondre).
        state_ = WsState::Closed;
        socket_.close();
        if (closeHandler_) closeHandler_(1006, "timeout en attente de la trame Close du pair");
        return;
    }

    // On n'attend une NOUVELLE donnee socket que si le buffer ne contient deja aucun octet
    // en attente de decodage (ex. leftover arrive groupe avec la reponse de handshake, ou
    // reste d'une trame precedente). Sinon, si aucune nouvelle donnee n'arrivait jamais,
    // le contenu deja bufferise ne serait jamais decode : bug reel observe en pratique
    // quand un pair envoie sa reponse HTTP et sa premiere trame dans le meme paquet TCP.
    int waitTimeout = recvBuffer_.empty() ? timeoutMs : 0;

    if (socket_.waitReadable(waitTimeout)) {
        uint8_t buf[65536];

        while (true) {
            bool wouldBlock = false;
            ssize_t n;

            try {
                n = socket_.recv(buf, sizeof(buf), wouldBlock);
            } catch (const std::exception& e) {
                if (errorHandler_) errorHandler_(std::string("WebSocketClient::poll: ") + e.what());
                state_ = WsState::Closed;
                socket_.close();
                return;
            }

            if (wouldBlock) break;

            if (n == 0) {
                // Fermeture TCP brutale, sans trame Close prealable (RFC 6455 7.1.5 : 1006,
                // reserve pour signaler ce cas en interne, jamais envoye sur le fil).
                state_ = WsState::Closed;
                socket_.close();
                if (closeHandler_) closeHandler_(1006, "connexion fermee sans handshake de fermeture");
                return;
            }

            recvBuffer_.insert(recvBuffer_.end(), buf, buf + n);

            if (!socket_.waitReadable(0)) break; // plus rien d'immediatement disponible pour l'instant
        }
    }

    // Decode toujours ce qui est present dans recvBuffer_, meme si aucune nouvelle donnee
    // n'a ete lue ci-dessus (cas du leftover deja bufferise, cf. commentaire plus haut).
    size_t offset = 0;
    while (offset < recvBuffer_.size()) {
        auto result = WsFrameCodec::decode(recvBuffer_.data() + offset, recvBuffer_.size() - offset);

        if (result.status == WsDecodeStatus::NeedMoreData) {
            break;
        }

        if (result.status == WsDecodeStatus::ProtocolError) {
            failConnection("WebSocketClient::poll: trame invalide recue (violation du protocole)");
            recvBuffer_.clear();
            return;
        }

        handleFrame(result.frame);
        offset += result.consumed;

        if (state_ == WsState::Closed) {
            recvBuffer_.clear();
            return;
        }
    }

    recvBuffer_.erase(recvBuffer_.begin(), recvBuffer_.begin() + static_cast<long>(offset));
}

void WebSocketClient::run() {
    while (state_ != WsState::Closed) {
        poll(1000);
    }
}
