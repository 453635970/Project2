#include <iostream>
#include <string>
#include "websocket_client.h"

int main(int argc, char** argv) 
{
system("chcp 65001"); // 设置控制台为UTF-8编码
    std::string host = "124.222.6.60";
    std::string port = "8800";
    std::string path = "/";
    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = argv[2];
    if (argc >= 4) path = argv[3];

    WebSocketClient ws;
    std::cout << "Connecting to ws://" << host << ":" << port << path << " ...\n";
    if (!ws.connect(host, port, path)) 
    {
        std::cerr << "Failed to connect" << std::endl;
        return 1;
    }

    std::string recv = ws.recvText();
    if (recv.empty()) 
    {
        std::cerr << "Receive failed or connection closed" << std::endl;
    } else 
    {
        std::cout << "Received: " << recv << std::endl;
    }


    {
     std::string msg = "123666666";
    std::cout << "Sending: " << msg << std::endl;
    if (!ws.sendText(msg)) 
    {
        std::cerr << "Send failed" << std::endl;
        ws.close();
        return 1;
    }

    std::cout << "Waiting for a message (blocking)..." << std::endl;
    std::string recv = ws.recvText();
    if (recv.empty()) 
    {
        std::cerr << "Receive failed or connection closed" << std::endl;
    } else 
    {
        std::cout << "Received: " << recv << std::endl;
    }
}
    ws.close();
    
    return 0;
}

