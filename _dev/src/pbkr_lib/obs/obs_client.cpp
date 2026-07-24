#include "obs/obs_client.hpp"
#include "obs/obs_auth.hpp"
#include "tools/random_bytes.hpp"

#include <stdexcept>

void ObsClient::connect(const std::string& host, uint16_t port, const std::string& password) {
    password_ = password;
    identified_ = false;
    pendingRequests_.clear();

    ws_.onMessage([this](const std::string& text, bool /*isBinary*/) { handleMessage(text); });

    ws_.onError([this](const std::string& message) {
        if (errorHandler_) errorHandler_(message);
    });

    ws_.onClose([this](uint16_t code, const std::string& reason) {
        identified_ = false;
        if (errorHandler_) {
            errorHandler_("ObsClient: connexion fermee (code " + std::to_string(code) +
                          (reason.empty() ? "" : (": " + reason)) + ")");
        }
    });

    ws_.connect(host, port, "/");
}

void ObsClient::onEvent(EventHandler handler) { eventHandler_ = std::move(handler); }
void ObsClient::onConnected(ConnectedHandler handler) { connectedHandler_ = std::move(handler); }
void ObsClient::onError(ErrorHandler handler) { errorHandler_ = std::move(handler); }

void ObsClient::handleMessage(const std::string& rawText) {
    nlohmann::json message;
    try {
        message = nlohmann::json::parse(rawText);
    } catch (const std::exception& e) {
        if (errorHandler_) errorHandler_(std::string("ObsClient: JSON invalide recu: ") + e.what());
        return;
    }

    ObsOpcode op;
    try {
        op = ObsProtocol::extractOpcode(message);
    } catch (const std::exception& e) {
        if (errorHandler_) errorHandler_(e.what());
        return;
    }

    switch (op) {
        case ObsOpcode::Hello: {
            ObsHello hello;
            try {
                hello = ObsProtocol::parseHello(message);
            } catch (const std::exception& e) {
                if (errorHandler_) errorHandler_(e.what());
                return;
            }

            std::string authResponse;
            if (hello.authenticationRequired) {
                authResponse = ObsAuth::computeAuthResponse(password_, hello.authSalt, hello.authChallenge);
            }

            std::string identify = ObsProtocol::buildIdentify(hello.rpcVersion, authResponse);
            ws_.sendText(identify);
            break;
        }

        case ObsOpcode::Identified: {
            identified_ = true;
            if (connectedHandler_) connectedHandler_();
            break;
        }

        case ObsOpcode::Event: {
            try {
                ObsEvent event = ObsProtocol::parseEvent(message);
                if (eventHandler_) eventHandler_(event.eventType, event.eventData);
            } catch (const std::exception& e) {
                if (errorHandler_) errorHandler_(e.what());
            }
            break;
        }

        case ObsOpcode::RequestResponse: {
            ObsRequestResponse resp;
            try {
                resp = ObsProtocol::parseRequestResponse(message);
            } catch (const std::exception& e) {
                if (errorHandler_) errorHandler_(e.what());
                return;
            }

            auto it = pendingRequests_.find(resp.requestId);
            if (it != pendingRequests_.end()) {
                RequestCallback callback = std::move(it->second);
                pendingRequests_.erase(it);
                if (callback) callback(resp.success, resp.statusCode, resp.comment, resp.responseData);
            }
            break;
        }

        case ObsOpcode::Reidentify:
        case ObsOpcode::Identify:
        case ObsOpcode::Request:
        case ObsOpcode::RequestBatch:
        case ObsOpcode::RequestBatchResponse:
            // Identify/Reidentify (client->serveur ou renegociation) et les variantes Batch
            // ne sont pas gerees pour l'instant : ignorees silencieusement plutot que de
            // faire echouer la connexion sur un message que le serveur n'aurait de toute
            // facon jamais du renvoyer au client.
            break;
    }
}

void ObsClient::call(const std::string& requestType, const nlohmann::json& requestData,
                      RequestCallback callback) {
    if (!identified_) {
        throw std::runtime_error("ObsClient::call: non identifie aupres du serveur "
                                  "(Identified pas encore recu)");
    }

    std::string requestId = generateRequestId();
    pendingRequests_[requestId] = std::move(callback);

    std::string message = ObsProtocol::buildRequest(requestType, requestId, requestData);
    ws_.sendText(message);
}

void ObsClient::call(const std::string& requestType, RequestCallback callback) {
    call(requestType, nullptr, std::move(callback));
}

void ObsClient::closeConnection(uint16_t code, const std::string& reason) {
    ws_.closeConnection(code, reason);
}

void ObsClient::poll(int timeoutMs) {
    ws_.poll(timeoutMs);
}

void ObsClient::run() {
    ws_.run();
}

std::string ObsClient::generateRequestId() {
    auto bytes = RandomBytes::generate(16);

    static const char* hexChars = "0123456789abcdef";
    std::string out;
    out.reserve(32);
    for (uint8_t b : bytes) {
        out.push_back(hexChars[b >> 4]);
        out.push_back(hexChars[b & 0x0F]);
    }
    return out;
}
