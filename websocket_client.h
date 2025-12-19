#pragma once

#include <string>
#include <vector>

class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();

    // Connect to websocket server at host:port and request the given path (e.g. "/")
    // Returns true on success.
    bool connect(const std::string& host, const std::string& port, const std::string& path = "/");

    // Send a text message (blocking). Returns true on success.
    bool sendText(const std::string& message);

    // Receive a text message (blocking). Returns empty string on error/close.
    std::string recvText();

    // Close the connection.
    void close();

    // Check whether the socket is connected
    bool isConnected() const { return connected_; }

private:
    int sock_;
    bool connected_;

    bool doHandshake(const std::string& host, const std::string& path, const std::string& key);

    // low-level send/recv helpers
    bool sendAll(const void* data, size_t len);
    bool recvAll(void* data, size_t len);

    // framing helpers
    bool sendFrame(const std::vector<unsigned char>& payload, unsigned char opcode, bool maskPayload);
    bool recvFrame(std::vector<unsigned char>& outPayload, unsigned char& outOpcode);
};
