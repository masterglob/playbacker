#pragma once
// obs/obs_protocol.hpp — Construction et analyse des messages du protocole obs-websocket v5.
//
// Couche purement fonctionnelle : transforme des nlohmann::json en structures typees et
// vice-versa. Ne connait rien au reseau (WebSocketClient) ni a l'authentification
// (ObsAuth) : ObsClient orchestre les deux en s'appuyant sur cette classe pour le format
// des messages.
//
// Reference : https://github.com/obsproject/obs-websocket/blob/master/docs/generated/protocol.md

#include <nlohmann/json.hpp>

#include <string>

enum class ObsOpcode : int {
    Hello                = 0,
    Identify             = 1,
    Identified           = 2,
    Reidentify           = 3,
    Event                = 5,
    Request              = 6,
    RequestResponse      = 7,
    RequestBatch         = 8,
    RequestBatchResponse = 9,
};

struct ObsHello {
    int rpcVersion = 1;
    bool authenticationRequired = false;
    std::string authChallenge;
    std::string authSalt;
};

struct ObsIdentified {
    int negotiatedRpcVersion = 0;
};

struct ObsRequestResponse {
    std::string requestType;
    std::string requestId;
    bool success = false;
    int statusCode = 0;
    std::string comment;
    nlohmann::json responseData; // null si absent (requete sans donnees de reponse)
};

struct ObsEvent {
    std::string eventType;
    int eventIntent = 0;
    nlohmann::json eventData; // null si absent
};

class ObsProtocol {
public:
    // Determine l'opcode d'un message brut deja parse en JSON.
    // Leve std::runtime_error si le champ "op" est absent, non entier, ou d'une valeur
    // ne correspondant a aucun opcode connu du protocole v5.
    static ObsOpcode extractOpcode(const nlohmann::json& message);

    // --- Parsing des messages entrants ---
    // Chacune leve std::runtime_error si l'opcode du message ne correspond pas a celui attendu.
    static ObsHello parseHello(const nlohmann::json& message);
    static ObsIdentified parseIdentified(const nlohmann::json& message);
    static ObsRequestResponse parseRequestResponse(const nlohmann::json& message);
    static ObsEvent parseEvent(const nlohmann::json& message);

    // --- Construction des messages sortants ---
    // Renvoie le JSON deja serialise (dump()), pret a passer a WebSocketClient::sendText().

    // authenticationResponse vide => champ "authentication" omis (serveur sans mot de passe).
    static std::string buildIdentify(int rpcVersion, const std::string& authenticationResponse = "");

    // requestData.is_null() => champ "requestData" omis (requete sans parametres).
    static std::string buildRequest(const std::string& requestType, const std::string& requestId,
                                     const nlohmann::json& requestData = nullptr);
};
