// main.cpp — exemple minimal : connexion a OBS, recuperation de la version,
// changement de scene, et ecoute des venements.

#include "obs/obs_client.hpp"

#include <iostream>
#include <csignal>

static volatile std::sig_atomic_t g_running = 1;

// static const std::string hostname = "127.0.0.1";
static const std::string hostname = "172.23.112.1";
//static const std::string hostname = "192.168.1.123";
static const std::string password = "EX7sbbLpXyQWC5lc";
static const unsigned short port = 4455;

void onSigint(int) {
    std::cout << "[onSigint) " << "\n";
    if (!g_running)
        exit(1);
    g_running = 0;
}

int main() {
    std::signal(SIGINT, onSigint);

    ObsClient client;

    client.onError([](const std::string& message) {
        std::cerr << "[erreur] " << message << "\n";
    });

    client.onEvent([](const std::string& eventType, const nlohmann::json& data) {
        std::cout << "[evenement] " << eventType;
        if (!data.is_null()) {
            std::cout << " : " << data.dump();
        }
        std::cout << "\n";
    });

    client.onConnected([&client]() {
        std::cout << "Connecte et identifie aupres d'OBS.\n";

        client.call("GetVersion", [](bool success, int code, const std::string& comment,
                                      const nlohmann::json& data) {
            if (!success) {
                std::cerr << "GetVersion a echoue (code " << code << "): " << comment << "\n";
                return;
            }
            std::cout << "Version OBS : " << data.at("obsVersion").get<std::string>() << "\n";
        });

        nlohmann::json params;
        params["sceneName"] = "S1";
        client.call("SetCurrentProgramScene", params,
                    [](bool success, int code, const std::string& comment, const nlohmann::json&) {
                        if (!success) {
                            std::cerr << "SetCurrentProgramScene a echoue (code " << code
                                      << "): " << comment << "\n";
                        } else {
                            std::cout << "Scene changee avec succes.\n";
                        }
                    });
    });

    try {
        client.connect(hostname, port, password);
    } catch (const std::exception& e) {
        std::cerr << "Impossible de se connecter a OBS : " << e.what() << "\n";
        return 1;
    }

    while (g_running) {
        client.poll(200);
    }

    client.closeConnection();

    return 0;
}