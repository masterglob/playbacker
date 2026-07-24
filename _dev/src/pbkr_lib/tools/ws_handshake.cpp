#include "tools/ws_handshake.hpp"

#include "tools/sha1.hpp"
#include "tools/base64.hpp"
#include "tools/random_bytes.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

// GUID magique impose par la RFC 6455, section 1.3.
const char* kWebSocketGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

} // namespace

std::string WsHandshake::generateKey() {
    auto raw = RandomBytes::generate(16);
    return Base64::encode(raw.data(), raw.size());
}

std::string WsHandshake::buildRequest(const std::string& host,
                                       const std::string& path,
                                       const std::string& key) {
    std::ostringstream req;
    req << "GET " << path << " HTTP/1.1\r\n"
        << "Host: " << host << "\r\n"
        << "Upgrade: websocket\r\n"
        << "Connection: Upgrade\r\n"
        << "Sec-WebSocket-Key: " << key << "\r\n"
        << "Sec-WebSocket-Version: 13\r\n"
        << "\r\n";
    return req.str();
}

std::string WsHandshake::computeExpectedAccept(const std::string& key) {
    auto digest = Sha1::hash(key + kWebSocketGuid);
    return Base64::encode(digest.data(), digest.size());
}

int WsHandshake::extractStatusCode(const std::string& rawResponse) {
    size_t lineEnd = rawResponse.find("\r\n");
    std::string statusLine = (lineEnd == std::string::npos) ? rawResponse
                                                              : rawResponse.substr(0, lineEnd);

    // Format attendu : "HTTP/1.1 101 Switching Protocols"
    size_t firstSpace = statusLine.find(' ');
    if (firstSpace == std::string::npos) return -1;

    size_t secondSpace = statusLine.find(' ', firstSpace + 1);
    std::string codeStr = (secondSpace == std::string::npos)
                               ? statusLine.substr(firstSpace + 1)
                               : statusLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);

    if (codeStr.empty() || !std::all_of(codeStr.begin(), codeStr.end(),
                                        [](unsigned char c) { return std::isdigit(c); })) {
        return -1;
    }

    try {
        return std::stoi(codeStr);
    } catch (...) {
        return -1;
    }
}

std::string WsHandshake::extractHeader(const std::string& rawResponse, const std::string& headerName) {
    std::string needle = toLower(headerName);
    size_t pos = 0;
    bool firstLine = true;

    while (pos <= rawResponse.size()) {
        size_t lineEnd = rawResponse.find("\r\n", pos);
        std::string line = (lineEnd == std::string::npos) ? rawResponse.substr(pos)
                                                            : rawResponse.substr(pos, lineEnd - pos);

        if (firstLine) {
            // On saute la status line.
            firstLine = false;
        } else if (line.empty()) {
            // Ligne vide = fin des en-tetes (debut du corps eventuel) : on arrete la recherche.
            break;
        } else {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string name = toLower(trim(line.substr(0, colon)));
                if (name == needle) {
                    return trim(line.substr(colon + 1));
                }
            }
        }

        if (lineEnd == std::string::npos) break;
        pos = lineEnd + 2;
    }

    return "";
}

bool WsHandshake::validateResponse(const std::string& rawResponse, const std::string& key) {
    if (extractStatusCode(rawResponse) != 101) {
        return false;
    }

    std::string accept = extractHeader(rawResponse, "Sec-WebSocket-Accept");
    if (accept.empty()) {
        return false;
    }

    return accept == computeExpectedAccept(key);
}