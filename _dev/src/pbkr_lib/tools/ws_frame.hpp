#pragma once
// tools/ws_frame.hpp — Framing WebSocket bas niveau (RFC 6455, section 5.2).
//
// WsFrameCodec encode/decode UNE trame a la fois. Il ne connait rien a la reassemblage
// de messages fragmentes sur plusieurs trames (opcode Continuation) : cette logique
// applicative appartient a la couche superieure (WebSocketClient), qui appelle decode()
// en boucle sur son buffer de reception et accumule les trames Continuation jusqu'a fin==true.

#include <cstdint>
#include <cstddef>
#include <vector>

enum class WsOpcode : uint8_t {
    Continuation = 0x0,
    Text         = 0x1,
    Binary       = 0x2,
    Close        = 0x8,
    Ping         = 0x9,
    Pong         = 0xA,
};

struct WsFrame {
    bool fin = true;
    WsOpcode opcode = WsOpcode::Text;
    std::vector<uint8_t> payload;
};

enum class WsDecodeStatus {
    NeedMoreData,  // buffer incomplet : reessayer une fois davantage d'octets recus
    Ok,            // trame decodee avec succes
    ProtocolError, // trame malformee ou hors des limites acceptees (RSV bits, opcode, taille...)
};

struct WsDecodeResult {
    WsDecodeStatus status = WsDecodeStatus::NeedMoreData;
    WsFrame frame;
    size_t consumed = 0; // octets consommes dans le buffer d'entree ; valide seulement si status == Ok
};

class WsFrameCodec {
public:
    // Taille maximale de payload acceptee en decode : protection contre un champ de longueur
    // demesure envoye par un pair malveillant ou defaillant. 64 MiB est largement suffisant
    // pour les messages obs-websocket (captures d'ecran base64 comprises).
    static constexpr size_t kMaxPayloadLen = 64ull * 1024 * 1024;

    // Encode une trame complete en octets prets a etre envoyes sur le socket.
    // masked=true genere une cle de masquage aleatoire a 4 octets (obligatoire cote client,
    // RFC 6455 5.1 : "a client MUST mask all frames it sends"). masked=false pour un serveur
    // (non utilise ici puisqu'on implemente un client, mais garde pour symetrie/tests).
    static std::vector<uint8_t> encode(const WsFrame& frame, bool masked);

    // Tente de decoder une trame depuis le debut du buffer [data, data+len).
    // Ne modifie jamais le buffer d'entree. Si status == NeedMoreData ou ProtocolError,
    // consumed reste a 0 : l'appelant ne doit rien retirer de son buffer de reception.
    static WsDecodeResult decode(const uint8_t* data, size_t len);
};
