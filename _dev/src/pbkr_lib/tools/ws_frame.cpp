#include "tools/ws_frame.hpp"
#include "tools/random_bytes.hpp"

#include <cstring>

std::vector<uint8_t> WsFrameCodec::encode(const WsFrame& frame, bool masked) {
    std::vector<uint8_t> out;
    size_t payloadLen = frame.payload.size();

    // Octet 0 : bit FIN + 3 bits RSV (toujours 0, pas d'extension negociee) + 4 bits opcode.
    out.push_back(static_cast<uint8_t>((frame.fin ? 0x80 : 0x00) |
                                        (static_cast<uint8_t>(frame.opcode) & 0x0F)));

    uint8_t maskBit = masked ? 0x80 : 0x00;

    if (payloadLen < 126) {
        out.push_back(static_cast<uint8_t>(maskBit | payloadLen));
    } else if (payloadLen <= 0xFFFF) {
        out.push_back(static_cast<uint8_t>(maskBit | 126));
        out.push_back(static_cast<uint8_t>((payloadLen >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>(payloadLen & 0xFF));
    } else {
        out.push_back(static_cast<uint8_t>(maskBit | 127));
        for (int i = 7; i >= 0; --i) {
            out.push_back(static_cast<uint8_t>((static_cast<uint64_t>(payloadLen) >> (8 * i)) & 0xFF));
        }
    }

    if (masked) {
        auto maskKey = RandomBytes::generate(4);
        out.insert(out.end(), maskKey.begin(), maskKey.end());

        size_t start = out.size();
        out.resize(start + payloadLen);
        for (size_t i = 0; i < payloadLen; ++i) {
            out[start + i] = static_cast<uint8_t>(frame.payload[i] ^ maskKey[i % 4]);
        }
    } else {
        out.insert(out.end(), frame.payload.begin(), frame.payload.end());
    }

    return out;
}

WsDecodeResult WsFrameCodec::decode(const uint8_t* data, size_t len) {
    WsDecodeResult result;

    if (len < 2) {
        return result; // NeedMoreData
    }

    uint8_t byte0 = data[0];
    uint8_t byte1 = data[1];

    uint8_t rsv = (byte0 >> 4) & 0x07;
    if (rsv != 0) {
        // RSV1/2/3 reserves aux extensions (ex. permessage-deflate) : on ne negocie
        // aucune extension, donc un pair correct ne doit jamais les positionner.
        result.status = WsDecodeStatus::ProtocolError;
        return result;
    }

    bool fin = (byte0 & 0x80) != 0;
    uint8_t opcodeRaw = byte0 & 0x0F;

    switch (opcodeRaw) {
        case 0x0: case 0x1: case 0x2: case 0x8: case 0x9: case 0xA:
            break;
        default:
            result.status = WsDecodeStatus::ProtocolError;
            return result;
    }

    bool masked = (byte1 & 0x80) != 0;
    uint8_t len7 = byte1 & 0x7F;

    size_t headerSize = 2;
    uint64_t payloadLen = 0;

    if (len7 < 126) {
        payloadLen = len7;
    } else if (len7 == 126) {
        headerSize += 2;
        if (len < headerSize) return result; // NeedMoreData
        payloadLen = (static_cast<uint64_t>(data[2]) << 8) | static_cast<uint64_t>(data[3]);
    } else { // len7 == 127
        headerSize += 8;
        if (len < headerSize) return result; // NeedMoreData
        payloadLen = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLen = (payloadLen << 8) | static_cast<uint64_t>(data[2 + i]);
        }
        if (payloadLen & (1ull << 63)) {
            // RFC 6455 5.2 : le bit de poids fort doit toujours etre a 0.
            result.status = WsDecodeStatus::ProtocolError;
            return result;
        }
    }

    if (payloadLen > kMaxPayloadLen) {
        result.status = WsDecodeStatus::ProtocolError;
        return result;
    }

    size_t maskOffset = headerSize;
    if (masked) {
        headerSize += 4;
    }

    size_t totalSize = headerSize + static_cast<size_t>(payloadLen);
    if (len < totalSize) {
        return result; // NeedMoreData
    }

    result.frame.fin = fin;
    result.frame.opcode = static_cast<WsOpcode>(opcodeRaw);
    result.frame.payload.resize(static_cast<size_t>(payloadLen));

    size_t payloadOffset = headerSize;

    if (masked) {
        uint8_t maskKey[4] = {
            data[maskOffset], data[maskOffset + 1], data[maskOffset + 2], data[maskOffset + 3]
        };
        for (size_t i = 0; i < payloadLen; ++i) {
            result.frame.payload[i] = static_cast<uint8_t>(data[payloadOffset + i] ^ maskKey[i % 4]);
        }
    } else if (payloadLen > 0) {
        std::memcpy(result.frame.payload.data(), data + payloadOffset, static_cast<size_t>(payloadLen));
    }

    result.status = WsDecodeStatus::Ok;
    result.consumed = totalSize;
    return result;
}
