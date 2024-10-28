#pragma once

#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

class SocketServer {
public:
    SocketServer(const char* port = "27015");
    ~SocketServer();

    bool initialize();
    bool createListener();
    bool acceptConnection();
    bool sendMessage(const std::string& message);
    std::string receiveMessage();
    void closeClientConnection();
    void cleanup();

    // Add this new method
    SOCKET getClientSocket() const { return m_clientSocket; }

private:
    static const int DEFAULT_BUFLEN = 4096;
    const char* m_port;
    SOCKET m_listenSocket;
    SOCKET m_clientSocket;
    bool m_initialized;
};
