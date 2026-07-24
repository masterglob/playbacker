#pragma once
// obs/obs_client.hpp — Client OBS websocket v5 complet : orchestre WebSocketClient
// (transport WS generique) + ObsProtocol (format des messages) + ObsAuth (challenge/response).
//
// Cycle de vie : connect() etablit la connexion TCP+WS puis attend le message Hello.
// Des reception du Hello (asynchrone, via poll()/run()), le client calcule l'authentification
// si necessaire et envoie Identify automatiquement. Une fois Identified recu, isConnected()
// devient true et onConnected() est appele : c'est a partir de ce moment que call() peut
// etre utilise pour envoyer des requetes.
//
// Comme WebSocketClient, aucun callback n'est jamais invoque depuis un autre thread que
// celui qui appelle poll()/run().

#include "tools/websocket_client.hpp"
#include "obs/obs_protocol.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>

class ObsClient {
public:
    using EventHandler = std::function<void(const std::string& eventType, const nlohmann::json& eventData)>;
    using ConnectedHandler = std::function<void()>;
    using ErrorHandler = std::function<void(const std::string& message)>;
    using RequestCallback = std::function<void(bool success, int statusCode,
                                                const std::string& comment,
                                                const nlohmann::json& responseData)>;

    ObsClient() = default;

    // Etablit la connexion TCP+WS (bloquant, cf. WebSocketClient::connect()). La suite de la
    // negociation OBS (Hello -> Identify -> Identified) se deroule ensuite de facon
    // asynchrone au fil des appels a poll()/run(). password peut etre vide si le serveur
    // OBS n'a pas de mot de passe configure (aucune authentification ne sera alors envoyee).
    void connect(const std::string& host, uint16_t port, const std::string& password = "");

    // Callbacks appeles depuis poll()/run(). A enregistrer avant le premier appel a poll().
    void onEvent(EventHandler handler);
    void onConnected(ConnectedHandler handler);
    void onError(ErrorHandler handler);

    // Envoie une requete OBS ; callback appele avec la reponse correspondante des qu'elle
    // arrive (correlation automatique via un requestId genere en interne).
    // Leve std::runtime_error si isConnected() est faux (Identified pas encore recu).
    void call(const std::string& requestType, const nlohmann::json& requestData,
              RequestCallback callback);
    void call(const std::string& requestType, RequestCallback callback);

    // Ferme proprement la connexion (RFC 6455 closing handshake, cf. WebSocketClient).
    void closeConnection(uint16_t code = 1000, const std::string& reason = "");

    void poll(int timeoutMs);
    void run();

    // true uniquement une fois le message Identified recu : avant cela, call() echoue.
    bool isConnected() const { return identified_; }

private:
    void handleMessage(const std::string& rawText);
    static std::string generateRequestId();

    WebSocketClient ws_;
    std::string password_;
    bool identified_ = false;

    std::unordered_map<std::string, RequestCallback> pendingRequests_;

    EventHandler eventHandler_;
    ConnectedHandler connectedHandler_;
    ErrorHandler errorHandler_;
};
