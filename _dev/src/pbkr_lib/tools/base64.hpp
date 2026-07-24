#pragma once
// base64.hpp — Encodage/décodage Base64 (RFC 4648), header-only, sans dépendance.

#include <string>
#include <vector>
#include <cstdint>
#include <array>
#include <stdexcept>

class Base64 {
public:
    static std::string encode(const uint8_t* data, size_t len) {
        static const char* table =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string out;
        out.reserve(((len + 2) / 3) * 4);

        size_t i = 0;
        while (i + 3 <= len) {
            uint32_t n = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8) | data[i + 2];
            out.push_back(table[(n >> 18) & 0x3F]);
            out.push_back(table[(n >> 12) & 0x3F]);
            out.push_back(table[(n >> 6) & 0x3F]);
            out.push_back(table[n & 0x3F]);
            i += 3;
        }

        size_t rem = len - i;
        if (rem == 1) {
            uint32_t n = uint32_t(data[i]) << 16;
            out.push_back(table[(n >> 18) & 0x3F]);
            out.push_back(table[(n >> 12) & 0x3F]);
            out.push_back('=');
            out.push_back('=');
        } else if (rem == 2) {
            uint32_t n = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8);
            out.push_back(table[(n >> 18) & 0x3F]);
            out.push_back(table[(n >> 12) & 0x3F]);
            out.push_back(table[(n >> 6) & 0x3F]);
            out.push_back('=');
        }

        return out;
    }

    static std::string encode(const std::vector<uint8_t>& data) {
        return data.empty() ? std::string() : encode(data.data(), data.size());
    }

    template <size_t N>
    static std::string encode(const std::array<uint8_t, N>& data) {
        return encode(data.data(), N);
    }

    static std::string encode(const std::string& raw) {
        return encode(reinterpret_cast<const uint8_t*>(raw.data()), raw.size());
    }

    static std::vector<uint8_t> decode(const std::string& in) {
        auto decodeChar = [](char c) -> int {
            if (c >= 'A' && c <= 'Z') return c - 'A';
            if (c >= 'a' && c <= 'z') return c - 'a' + 26;
            if (c >= '0' && c <= '9') return c - '0' + 52;
            if (c == '+') return 62;
            if (c == '/') return 63;
            return -1; // padding '=' ou caractère invalide
        };

        std::vector<uint8_t> out;
        out.reserve((in.size() / 4) * 3);

        int vals[4];
        size_t vi = 0;

        for (char c : in) {
            if (c == '=' || c == '\n' || c == '\r') continue;
            int v = decodeChar(c);
            if (v < 0) throw std::runtime_error("Base64::decode: caractere invalide");
            vals[vi++] = v;
            if (vi == 4) {
                uint32_t n = (uint32_t(vals[0]) << 18) | (uint32_t(vals[1]) << 12) |
                             (uint32_t(vals[2]) << 6) | uint32_t(vals[3]);
                out.push_back(uint8_t(n >> 16));
                out.push_back(uint8_t(n >> 8));
                out.push_back(uint8_t(n));
                vi = 0;
            }
        }

        // Reliquat (cas 2 ou 3 caracteres utiles avant padding)
        if (vi == 2) {
            uint32_t n = (uint32_t(vals[0]) << 18) | (uint32_t(vals[1]) << 12);
            out.push_back(uint8_t(n >> 16));
        } else if (vi == 3) {
            uint32_t n = (uint32_t(vals[0]) << 18) | (uint32_t(vals[1]) << 12) | (uint32_t(vals[2]) << 6);
            out.push_back(uint8_t(n >> 16));
            out.push_back(uint8_t(n >> 8));
        }

        return out;
    }
};
