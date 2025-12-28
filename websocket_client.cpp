#include "websocket_client.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <sstream>
#include <random>
#include <ctime>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

// Minimal SHA1 implementation and Base64 required for WebSocket handshake
namespace {
    // --- Base64 encode ---
    static const char* b64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string base64_encode(const unsigned char* data, size_t len) {
        std::string out;
        out.reserve(((len + 2) / 3) * 4);
        unsigned int val = 0;
        int valb = -6;
        for (size_t i = 0; i < len; ++i) {
            val = (val << 8) + data[i];
            valb += 8;
            while (valb >= 0) {
                out.push_back(b64_table[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(b64_table[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    // --- SHA1 ---
    // Very small SHA1 implementation adapted for single-shot use
    typedef unsigned int uint32;
    void sha1(const unsigned char* data, size_t len, unsigned char out[20]) {
        uint32 h0 = 0x67452301;
        uint32 h1 = 0xEFCDAB89;
        uint32 h2 = 0x98BADCFE;
        uint32 h3 = 0x10325476;
        uint32 h4 = 0xC3D2E1F0;

        // Pre-processing
        size_t new_len = len + 1;
        while (new_len % 64 != 56) ++new_len;
        std::vector<unsigned char> msg(new_len + 8);
        memcpy(msg.data(), data, len);
        msg[len] = 0x80;
        uint64_t bits = static_cast<uint64_t>(len) * 8;
        for (size_t i = 0; i < 8; ++i) msg[new_len + i] = (bits >> ((7 - i) * 8)) & 0xFF;

        // Process the message in successive 512-bit chunks
        for (size_t offset = 0; offset < msg.size(); offset += 64) {
            uint32 w[80];
            for (int i = 0; i < 16; ++i) {
                w[i] = (msg[offset + i*4] << 24) | (msg[offset + i*4 + 1] << 16) | (msg[offset + i*4 + 2] << 8) | (msg[offset + i*4 + 3]);
            }
            for (int i = 16; i < 80; ++i) {
                uint32 temp = w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16];
                w[i] = (temp << 1) | (temp >> 31);
            }

            uint32 a = h0;
            uint32 b = h1;
            uint32 c = h2;
            uint32 d = h3;
            uint32 e = h4;

            for (int i = 0; i < 80; ++i) {
                uint32 f, k;
                if (i < 20) {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999;
                } else if (i < 40) {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                } else if (i < 60) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                } else {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }
                uint32 temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
                e = d;
                d = c;
                c = (b << 30) | (b >> 2);
                b = a;
                a = temp;
            }

            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }

        uint32 hs[5] = {h0, h1, h2, h3, h4};
        for (int i = 0; i < 5; ++i) {
            out[i*4 + 0] = (hs[i] >> 24) & 0xFF;
            out[i*4 + 1] = (hs[i] >> 16) & 0xFF;
            out[i*4 + 2] = (hs[i] >> 8) & 0xFF;
            out[i*4 + 3] = (hs[i]) & 0xFF;
        }
    }

    bool isValidUTF8(const std::string& str) {
    // 验证是否为有效的UTF-8字符串
        int i = 0;
        while (i < str.length()) {
            unsigned char c = static_cast<unsigned char>(str[i]);
            if (c <= 0x7F) {
                i++;
            } else if ((c & 0xE0) == 0xC0) {
                if (i + 1 >= str.length() || (str[i + 1] & 0xC0) != 0x80) return false;
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                if (i + 2 >= str.length() || (str[i + 1] & 0xC0) != 0x80 || (str[i + 2] & 0xC0) != 0x80) return false;
                i += 3;
            } else if ((c & 0xF8) == 0xF0) {
                if (i + 3 >= str.length() || (str[i + 1] & 0xC0) != 0x80 || 
                    (str[i + 2] & 0xC0) != 0x80 || (str[i + 3] & 0xC0) != 0x80) return false;
                i += 4;
            } else {
                return false;
            }
        }
        return true;
    }
}

// ---------------- WebSocketClient implementation ----------------
WebSocketClient::WebSocketClient() : sock_(-1), connected_(false) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
}

WebSocketClient::~WebSocketClient() {
    close();
    WSACleanup();
}

bool WebSocketClient::connect(const std::string& host, const std::string& port, const std::string& path) 
{
    if (connected_) return true;

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* result = nullptr;
    int rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
    if (rc != 0 || !result) return false;

    SOCKET s = INVALID_SOCKET;
    for (addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) 
    {
        s = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (s == INVALID_SOCKET) 
        continue;
        if (::connect(s, rp->ai_addr, (int)rp->ai_addrlen) == 0) 
        break;
        closesocket(s);
        s = INVALID_SOCKET;
    }
    freeaddrinfo(result);
    if (s == INVALID_SOCKET) return false;

    sock_ = (int)s;

    // generate Sec-WebSocket-Key
    std::string keyRaw(16, 0);
    std::mt19937 rng((unsigned)time(nullptr));
    for (size_t i = 0; i < keyRaw.size(); ++i) keyRaw[i] = static_cast<char>(rng() & 0xFF);
    std::string key = base64_encode((const unsigned char*)keyRaw.data(), keyRaw.size());

    if (!doHandshake(host, path, key)) {
        closesocket(s);
        sock_ = -1;
        return false;
    }

    connected_ = true;
    return true;
}

bool WebSocketClient::doHandshake(const std::string& host, const std::string& path, const std::string& key) 
{
    std::ostringstream req;
    req << "GET " << path << " HTTP/1.1\r\n"
        << "Host: " << host << "\r\n"
        << "Upgrade: websocket\r\n"
        << "Connection: Upgrade\r\n"
        << "Sec-WebSocket-Version: 13\r\n"
        << "Sec-WebSocket-Key: " << key << "\r\n"
        << "\r\n";
    std::string rq = req.str();
    if (!sendAll(rq.data(), rq.size())) return false;

    // read response header (simple read until \r\n\r\n with timeout)
    std::string header;
    char buf[1024];
    int ret;
    while (true) {
        ret = ::recv((SOCKET)sock_, buf, sizeof(buf), 0);
        if (ret <= 0) return false;
        header.append(buf, buf + ret);
        if (header.find("\r\n\r\n") != std::string::npos) break;
        // safety: if header too large, abort
        if (header.size() > 64 * 1024) return false;
    }

    // check for 101 Switching Protocols
    if (header.find("HTTP/1.1 101") == std::string::npos) 
    return false;

    // compute expected accept value
    std::string acceptSource = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char sha[20];
    sha1((const unsigned char*)acceptSource.data(), acceptSource.size(), sha);
    std::string expected = base64_encode(sha, 20);

    // find Sec-WebSocket-Accept header
    std::string::size_type pos = header.find("Sec-WebSocket-Accept:");
    if (pos == std::string::npos) 
    return false;
    pos += strlen("Sec-WebSocket-Accept:");
    // read line
    std::string::size_type eol = header.find("\r\n", pos);
    if (eol == std::string::npos) return false;
    std::string acceptVal = header.substr(pos, eol - pos);
    // trim
    while (!acceptVal.empty() && (acceptVal.front() == ' ' || acceptVal.front() == '\t')) acceptVal.erase(acceptVal.begin());
    while (!acceptVal.empty() && (acceptVal.back() == '\r' || acceptVal.back() == '\n' || acceptVal.back() == ' ')) acceptVal.pop_back();

    return acceptVal == expected;
}

bool WebSocketClient::sendAll(const void* data, size_t len) {
    const char* p = (const char*)data;
    size_t left = len;
    while (left > 0) {
        int sent = ::send((SOCKET)sock_, p, (int)left, 0);
        if (sent == SOCKET_ERROR) return false;
        left -= sent;
        p += sent;
    }
    return true;
}

bool WebSocketClient::recvAll(void* data, size_t len) {
    char* p = (char*)data;
    size_t left = len;
    while (left > 0) {
        int r = ::recv((SOCKET)sock_, p, (int)left, 0);
        if (r <= 0) return false;
        left -= r;
        p += r;
    }
    return true;
}

bool WebSocketClient::sendFrame(const std::vector<unsigned char>& payload, unsigned char opcode, bool maskPayload) {
    std::vector<unsigned char> frame;
    unsigned char finOp = 0x80 | (opcode & 0x0F);
    frame.push_back(finOp);
    size_t plen = payload.size();
    if (plen <= 125) {
        unsigned char b = (unsigned char)plen;
        if (maskPayload) b |= 0x80;
        frame.push_back(b);
    } else if (plen <= 0xFFFF) {
        unsigned char b = 126;
        if (maskPayload) b |= 0x80;
        frame.push_back(b);
        frame.push_back((plen >> 8) & 0xFF);
        frame.push_back(plen & 0xFF);
    } else {
        unsigned char b = 127;
        if (maskPayload) b |= 0x80;
        frame.push_back(b);
        for (int i = 7; i >= 0; --i) frame.push_back((plen >> (8*i)) & 0xFF);
    }

    std::vector<unsigned char> mask(4);
    if (maskPayload) {
        std::mt19937 rng((unsigned)time(nullptr));
        for (int i = 0; i < 4; ++i) mask[i] = (unsigned char)(rng() & 0xFF);
        frame.insert(frame.end(), mask.begin(), mask.end());
        for (size_t i = 0; i < payload.size(); ++i) {
            unsigned char c = payload[i] ^ mask[i % 4];
            frame.push_back(c);
        }
    } else {
        frame.insert(frame.end(), payload.begin(), payload.end());
    }

    return sendAll(frame.data(), frame.size());
}

bool WebSocketClient::recvFrame(std::vector<unsigned char>& outPayload, unsigned char& outOpcode) {
    unsigned char hdr[2];
    if (!recvAll(hdr, 2)) return false;
    unsigned char b0 = hdr[0];
    unsigned char b1 = hdr[1];
    bool fin = (b0 & 0x80) != 0;
    unsigned char opcode = b0 & 0x0F;
    bool mask = (b1 & 0x80) != 0;
    uint64_t plen = b1 & 0x7F;
    if (plen == 126) {
        unsigned char ext[2]; if (!recvAll(ext,2)) return false;
        plen = (ext[0] << 8) | ext[1];
    } else if (plen == 127) {
        unsigned char ext[8]; if (!recvAll(ext,8)) return false;
        plen = 0;
        for (int i = 0; i < 8; ++i) plen = (plen << 8) | ext[i];
    }

    std::vector<unsigned char> maskKey(4);
    if (mask) {
        if (!recvAll(maskKey.data(), 4)) return false;
    }

    outPayload.resize(plen);
    if (plen > 0) {
        if (!recvAll(outPayload.data(), (size_t)plen)) return false;
        if (mask) {
            for (size_t i = 0; i < plen; ++i) outPayload[i] ^= maskKey[i % 4];
        }
    }

    outOpcode = opcode;
    return true;
}

bool WebSocketClient::sendText(const std::string& message) {
    if (!connected_) return false;
    std::vector<unsigned char> payload(message.begin(), message.end());
    return sendFrame(payload, 0x1, true); // mask payload (client -> server)
}

std::string WebSocketClient::recvText() {
    if (!connected_) return std::string();
    std::vector<unsigned char> payload;
    unsigned char opcode = 0;
    if (!recvFrame(payload, opcode)) {
        return std::string();
    }
    
    if (opcode == 0x1) {  // 文本帧
        std::string result(payload.begin(), payload.end());
        // 验证是否为有效的UTF-8
        if (isValidUTF8(result)) {
            return result;
        } else {
            // 如果不是有效的UTF-8，可以选择转换或返回错误
            // 这里简单返回，但你可能想要记录日志
            return result;
        }
    }
    else if (opcode == 0x2) {  // 二进制帧
        // 二进制帧不应该作为文本处理，这里转换为字符串但可能包含乱码
        return std::string(payload.begin(), payload.end());
    }
    // handle close (0x8) or ping/pong (0x9/0xA) 
    else if (opcode == 0x8) {
        close();
        return std::string();
    }
    return std::string();
}

void WebSocketClient::close() {
    if (connected_) {
        // send close frame
        std::vector<unsigned char> empty;
        sendFrame(empty, 0x8, true);
    }
    if (sock_ != -1) {
        closesocket((SOCKET)sock_);
        sock_ = -1;
    }
    connected_ = false;
}
