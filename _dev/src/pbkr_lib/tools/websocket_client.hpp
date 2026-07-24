#pragma once
// tools/websocket_client.hpp — Client WebSocket (RFC 6455) au-dessus de TcpSocket/WsHandshake/WsFrameCodec.
//
// Protocole generique : ne connait rien a OBS ni au JSON. Expose des messages Text/Binary
// complets (apres reassemblage d'une eventuelle fragmentation), gere automatiquement
// Ping->Pong et la fermeture (Close). La couche superieure (ObsClient, a venir) s'appuiera
// dessus pour le handshake Hello/Identify/Identified et le JSON.
//
// Modele d'utilisation : appeler connect(), enregistrer les callbacks, puis appeler poll()
// en boucle (ou run()) depuis le thread applicatif. Aucun callback n'est jamais invoque
// depuis un autre thread que celui qui appelle poll()/run().

#include "tools/tcp_socket.hpp"
#include "tools/ws_frame.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <chrono>

class WebSocketClient {
public:
    using MessageHandler = std::function<void(const std::string& payload, bool isBinary)>;
    using CloseHandler = std::function<void(uint16_t code, const std::string& reason)>;
    using ErrorHandler = std::function<void(const std::string& message)>;

    WebSocketClient() = default;

    // Etablit la connexion TCP puis effectue le handshake HTTP Upgrade (RFC 6455 section 4).
    // Bloquant. Leve std::runtime_error en cas d'echec : DNS, connexion refusee, timeout,
    // statut HTTP != 101, ou Sec-WebSocket-Accept invalide.
    void connect(const std::string& host, uint16_t port, const std::string& path = "/");

    // Callbacks appeles depuis poll()/run(). A enregistrer avant le premier appel a poll().
    void onMessage(MessageHandler handler);
    void onClose(CloseHandler handler);
    void onError(ErrorHandler handler);

    // Envoie un message complet en une seule trame (masquee, comme l'exige la RFC 6455 5.1
    // pour tout ce qu'un client envoie). Leve std::runtime_error si la connexion n'est pas Open.
    void sendText(const std::string& text);
    void sendBinary(const std::vector<uint8_t>& data);

    // Delai par defaut (ms) apres l'envoi d'une trame Close initiee par nous, avant de
    // forcer la fermeture si le pair ne renvoie jamais sa propre trame Close en echo.
    static constexpr int kDefaultCloseTimeoutMs = 3000;

    // Envoie une trame Close avec le code et la raison donnes (RFC 6455 5.5.1), puis passe
    // en etat "Closing" : la fermeture est normalement confirmee (callback onClose,
    // isOpen()==false) a reception de la trame Close en retour du pair, au prochain poll().
    // Si le pair ne repond jamais, poll() force la fermeture apres timeoutMs et invoque
    // onClose(1006, "..."), pour ne jamais rester bloque en "Closing" indefiniment.
    // timeoutMs <= 0 desactive cette detection (attente indefinie du pair, deconseille).
    void closeConnection(uint16_t code = 1000, const std::string& reason = "",
                          int timeoutMs = kDefaultCloseTimeoutMs);

    // Traite les evenements reseau disponibles pendant au plus timeoutMs
    // (0 = ne bloque pas si rien n'est disponible). A appeler en boucle par l'application.
    // Ne leve jamais d'exception : les erreurs reseau sont rapportees via onError() et
    // ferment la connexion (isOpen() devient false).
    void poll(int timeoutMs);

    // Boucle poll() en continu (timeout de 1s par iteration) jusqu'a fermeture complete.
    void run();

    bool isOpen() const { return state_ == WsState::Open; }

private:
    enum class WsState { Connecting, Open, Closing, Closed };

    std::string readHttpResponseHeaders(int timeoutMs);
    void handleFrame(const WsFrame& frame);
    void deliverMessage(WsOpcode opcode, const std::vector<uint8_t>& payload);
    void sendFrame(WsOpcode opcode, const uint8_t* data, size_t len, bool fin = true);
    void sendCloseFrame(uint16_t code, const std::string& reason);
    void failConnection(const std::string& message);

    TcpSocket socket_;
    WsState state_ = WsState::Connecting;

    std::vector<uint8_t> recvBuffer_;

    // Reassemblage d'un message fragmente (opcode Text/Binary suivi de Continuation...).
    bool fragmented_ = false;
    WsOpcode fragmentedOpcode_ = WsOpcode::Text;
    std::vector<uint8_t> fragmentedPayload_;

    bool closeSent_ = false;
    bool closeReceived_ = false;

    // Suivi du timeout de fermeture (cf. closeConnection()).
    bool closeDeadlineActive_ = false;
    std::chrono::steady_clock::time_point closeDeadline_{};

    MessageHandler messageHandler_;
    CloseHandler closeHandler_;
    ErrorHandler errorHandler_;
};
