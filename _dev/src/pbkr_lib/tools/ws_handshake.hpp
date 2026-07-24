#pragma once
// ws/ws_handshake.hpp — Handshake HTTP Upgrade du protocole WebSocket (RFC 6455, section 4).
//
// Cette classe ne touche a aucune socket : elle construit une requete texte a envoyer
// telle quelle, et valide une reponse texte telle que recue. La couche reseau
// (TcpSocket / WebSocketClient) se charge de l'envoi/reception des octets.

#include <string>

class WsHandshake {
public:
    // Genere une cle Sec-WebSocket-Key : 16 octets aleatoires, encodes en base64.
    static std::string generateKey();

    // Construit la requete HTTP GET d'upgrade websocket.
    //   host : nom d'hote + port eventuel, ex. "127.0.0.1:4455"
    //   path : chemin de la ressource, ex. "/"
    //   key  : valeur de Sec-WebSocket-Key (voir generateKey())
    // Retourne la requete complete, terminee par la ligne vide (\r\n\r\n), prete a envoyer.
    static std::string buildRequest(const std::string& host,
                                     const std::string& path,
                                     const std::string& key);

    // Calcule la valeur attendue de Sec-WebSocket-Accept a partir de la cle envoyee
    // (base64(sha1(key + GUID RFC 6455))).
    static std::string computeExpectedAccept(const std::string& key);

    // Valide une reponse HTTP brute (status line + en-tetes, telle que recue sur le socket) :
    // renvoie true si le statut est 101 Switching Protocols et si Sec-WebSocket-Accept
    // correspond a la cle envoyee.
    static bool validateResponse(const std::string& rawResponse, const std::string& key);

    // Extrait le code de statut HTTP de la ligne de statut ("HTTP/1.1 101 ..." -> 101).
    // Renvoie -1 si la ligne de statut est absente ou malformee.
    static int extractStatusCode(const std::string& rawResponse);

    // Extrait la valeur d'un en-tete HTTP (recherche insensible a la casse du nom).
    // Renvoie une chaine vide si l'en-tete est absent.
    static std::string extractHeader(const std::string& rawResponse, const std::string& headerName);
};