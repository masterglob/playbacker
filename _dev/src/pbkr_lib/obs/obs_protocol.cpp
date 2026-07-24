#include "obs/obs_protocol.hpp"

#include <stdexcept>

ObsOpcode ObsProtocol::extractOpcode(const nlohmann::json& message) {
    if (!message.contains("op") || !message.at("op").is_number_integer()) {
        throw std::runtime_error("ObsProtocol::extractOpcode: champ 'op' absent ou invalide");
    }

    int op = message.at("op").get<int>();
    switch (op) {
        case 0: case 1: case 2: case 3: case 5: case 6: case 7: case 8: case 9:
            return static_cast<ObsOpcode>(op);
        default:
            throw std::runtime_error("ObsProtocol::extractOpcode: opcode inconnu (" +
                                      std::to_string(op) + ")");
    }
}

ObsHello ObsProtocol::parseHello(const nlohmann::json& message) {
    if (extractOpcode(message) != ObsOpcode::Hello) {
        throw std::runtime_error("ObsProtocol::parseHello: opcode inattendu (Hello attendu)");
    }
    const auto& d = message.at("d");

    ObsHello hello;
    hello.rpcVersion = d.value("rpcVersion", 1);

    if (d.contains("authentication")) {
        hello.authenticationRequired = true;
        const auto& auth = d.at("authentication");
        hello.authChallenge = auth.value("challenge", "");
        hello.authSalt = auth.value("salt", "");
    }

    return hello;
}

ObsIdentified ObsProtocol::parseIdentified(const nlohmann::json& message) {
    if (extractOpcode(message) != ObsOpcode::Identified) {
        throw std::runtime_error("ObsProtocol::parseIdentified: opcode inattendu (Identified attendu)");
    }

    ObsIdentified result;
    result.negotiatedRpcVersion = message.at("d").value("negotiatedRpcVersion", 0);
    return result;
}

ObsRequestResponse ObsProtocol::parseRequestResponse(const nlohmann::json& message) {
    if (extractOpcode(message) != ObsOpcode::RequestResponse) {
        throw std::runtime_error("ObsProtocol::parseRequestResponse: opcode inattendu "
                                  "(RequestResponse attendu)");
    }
    const auto& d = message.at("d");

    ObsRequestResponse r;
    r.requestType = d.value("requestType", "");
    r.requestId = d.value("requestId", "");

    if (d.contains("requestStatus")) {
        const auto& status = d.at("requestStatus");
        r.success = status.value("result", false);
        r.statusCode = status.value("code", 0);
        r.comment = status.value("comment", "");
    }

    if (d.contains("responseData")) {
        r.responseData = d.at("responseData");
    }

    return r;
}

ObsEvent ObsProtocol::parseEvent(const nlohmann::json& message) {
    if (extractOpcode(message) != ObsOpcode::Event) {
        throw std::runtime_error("ObsProtocol::parseEvent: opcode inattendu (Event attendu)");
    }
    const auto& d = message.at("d");

    ObsEvent e;
    e.eventType = d.value("eventType", "");
    e.eventIntent = d.value("eventIntent", 0);

    if (d.contains("eventData")) {
        e.eventData = d.at("eventData");
    }

    return e;
}

std::string ObsProtocol::buildIdentify(int rpcVersion, const std::string& authenticationResponse) {
    nlohmann::json d;
    d["rpcVersion"] = rpcVersion;
    if (!authenticationResponse.empty()) {
        d["authentication"] = authenticationResponse;
    }

    nlohmann::json message;
    message["op"] = static_cast<int>(ObsOpcode::Identify);
    message["d"] = d;
    return message.dump();
}

std::string ObsProtocol::buildRequest(const std::string& requestType, const std::string& requestId,
                                       const nlohmann::json& requestData) {
    nlohmann::json d;
    d["requestType"] = requestType;
    d["requestId"] = requestId;
    if (!requestData.is_null()) {
        d["requestData"] = requestData;
    }

    nlohmann::json message;
    message["op"] = static_cast<int>(ObsOpcode::Request);
    message["d"] = d;
    return message.dump();
}
