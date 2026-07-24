#pragma once
// sha1.hpp — Implémentation SHA-1 (RFC 3174), header-only, sans dépendance.
// Utilisé uniquement pour Sec-WebSocket-Accept (RFC 6455), PAS pour l'auth OBS (qui utilise SHA-256).

#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <algorithm>

class Sha1 {
public:
    using Digest = std::array<uint8_t, 20>;

    Sha1() { reset(); }

    void reset() {
        state_ = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u};
        bitLen_ = 0;
        bufferLen_ = 0;
    }

    void update(const uint8_t* data, size_t len) {
        size_t i = 0;
        while (i < len) {
            size_t take = std::min(len - i, size_t(64) - bufferLen_);
            std::memcpy(buffer_.data() + bufferLen_, data + i, take);
            bufferLen_ += take;
            i += take;
            bitLen_ += take * 8;
            if (bufferLen_ == 64) {
                processBlock(buffer_.data());
                bufferLen_ = 0;
            }
        }
    }

    void update(const std::string& s) {
        update(reinterpret_cast<const uint8_t*>(s.data()), s.size());
    }

    Digest digest() {
        uint64_t savedBitLen = bitLen_;
        std::array<uint32_t, 5> savedState = state_;
        std::array<uint8_t, 64> savedBuffer = buffer_;
        size_t savedBufferLen = bufferLen_;

        uint8_t pad = 0x80;
        update(&pad, 1);
        uint8_t zero = 0x00;
        while (bufferLen_ != 56) update(&zero, 1);

        uint8_t lenBytes[8];
        for (int i = 0; i < 8; ++i) {
            lenBytes[i] = static_cast<uint8_t>(savedBitLen >> (56 - 8 * i));
        }
        update(lenBytes, 8);

        Digest out;
        for (int i = 0; i < 5; ++i) {
            out[i * 4 + 0] = static_cast<uint8_t>(state_[i] >> 24);
            out[i * 4 + 1] = static_cast<uint8_t>(state_[i] >> 16);
            out[i * 4 + 2] = static_cast<uint8_t>(state_[i] >> 8);
            out[i * 4 + 3] = static_cast<uint8_t>(state_[i]);
        }

        bitLen_ = savedBitLen;
        state_ = savedState;
        buffer_ = savedBuffer;
        bufferLen_ = savedBufferLen;

        return out;
    }

    static Digest hash(const uint8_t* data, size_t len) {
        Sha1 h;
        h.update(data, len);
        return h.digest();
    }

    static Digest hash(const std::string& s) {
        return hash(reinterpret_cast<const uint8_t*>(s.data()), s.size());
    }

    static std::string toHex(const Digest& d) {
        static const char* hexChars = "0123456789abcdef";
        std::string out;
        out.reserve(40);
        for (uint8_t b : d) {
            out.push_back(hexChars[b >> 4]);
            out.push_back(hexChars[b & 0x0F]);
        }
        return out;
    }

private:
    static uint32_t rol(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }

    void processBlock(const uint8_t* block) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            w[i] = (uint32_t(block[i * 4]) << 24) | (uint32_t(block[i * 4 + 1]) << 16) |
                   (uint32_t(block[i * 4 + 2]) << 8) | (uint32_t(block[i * 4 + 3]));
        }
        for (int i = 16; i < 80; ++i) {
            w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3], e = state_[4];

        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | ((~b) & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }

            uint32_t temp = rol(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rol(b, 30); b = a; a = temp;
        }

        state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d; state_[4] += e;
    }

    std::array<uint32_t, 5> state_{};
    std::array<uint8_t, 64> buffer_{};
    size_t bufferLen_ = 0;
    uint64_t bitLen_ = 0;
};
