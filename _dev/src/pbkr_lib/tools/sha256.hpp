#pragma once
// sha256.hpp — Implémentation SHA-256 (FIPS 180-4), header-only, sans dépendance.
// Zero dépendance externe : uniquement <cstdint>, <cstddef>, <array>, <string>, <vector>.

#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

class Sha256 {
public:
    using Digest = std::array<uint8_t, 32>;

    Sha256() { reset(); }

    void reset() {
        state_ = {
            0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
            0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
        };
        bitLen_ = 0;
        bufferLen_ = 0;
        finalized_ = false;
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

    void update(const std::vector<uint8_t>& v) {
        if (!v.empty()) update(v.data(), v.size());
    }

    // Termine le hachage et renvoie le digest binaire (32 octets).
    Digest digest() {
        // On travaille sur une copie pour permettre digest() sans détruire l'état
        // si on voulait continuer à update() ensuite (non standard mais pratique en test).
        uint64_t savedBitLen = bitLen_;
        std::array<uint32_t, 8> savedState = state_;
        std::array<uint8_t, 64> savedBuffer = buffer_;
        size_t savedBufferLen = bufferLen_;

        uint8_t pad = 0x80;
        update(&pad, 1);

        uint8_t zero = 0x00;
        while (bufferLen_ != 56) {
            update(&zero, 1);
        }

        uint64_t bitLenBE = savedBitLen; // longueur AVANT padding, en bits
        uint8_t lenBytes[8];
        for (int i = 0; i < 8; ++i) {
            lenBytes[i] = static_cast<uint8_t>(bitLenBE >> (56 - 8 * i));
        }
        update(lenBytes, 8);

        Digest out;
        for (int i = 0; i < 8; ++i) {
            out[i * 4 + 0] = static_cast<uint8_t>(state_[i] >> 24);
            out[i * 4 + 1] = static_cast<uint8_t>(state_[i] >> 16);
            out[i * 4 + 2] = static_cast<uint8_t>(state_[i] >> 8);
            out[i * 4 + 3] = static_cast<uint8_t>(state_[i]);
        }

        // Restaure l'état pré-finalisation (permet un digest() "const-like" réutilisable en tests)
        bitLen_ = savedBitLen;
        state_ = savedState;
        buffer_ = savedBuffer;
        bufferLen_ = savedBufferLen;

        return out;
    }

    // Hélper statique : hash complet en un appel.
    static Digest hash(const uint8_t* data, size_t len) {
        Sha256 h;
        h.update(data, len);
        return h.digest();
    }

    static Digest hash(const std::string& s) {
        return hash(reinterpret_cast<const uint8_t*>(s.data()), s.size());
    }

    static std::string toHex(const Digest& d) {
        static const char* hexChars = "0123456789abcdef";
        std::string out;
        out.reserve(64);
        for (uint8_t b : d) {
            out.push_back(hexChars[b >> 4]);
            out.push_back(hexChars[b & 0x0F]);
        }
        return out;
    }

private:
    static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }

    void processBlock(const uint8_t* block) {
        static const uint32_t k[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        };

        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (uint32_t(block[i * 4]) << 24) | (uint32_t(block[i * 4 + 1]) << 16) |
                   (uint32_t(block[i * 4 + 2]) << 8) | (uint32_t(block[i * 4 + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
        uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t temp1 = h + S1 + ch + k[i] + w[i];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;

            h = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }

        state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
        state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
    }

    std::array<uint32_t, 8> state_{};
    std::array<uint8_t, 64> buffer_{};
    size_t bufferLen_ = 0;
    uint64_t bitLen_ = 0;
    bool finalized_ = false;
};
