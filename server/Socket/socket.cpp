#include "socket.h"

SocketServer::SocketServer(const char* port)
    : m_port(port)
    , m_listenSocket(INVALID_SOCKET)
    , m_clientSocket(INVALID_SOCKET)
    , m_initialized(false)
{
}

SocketServer::~SocketServer() {
    cleanup();
}

bool SocketServer::initialize() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return false;
    }
    m_initialized = true;
    return true;
}

bool SocketServer::createListener() {
    if (!m_initialized) {
        std::cerr << "Winsock not initialized" << std::endl;
        return false;
    }

    struct addrinfo* result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int iResult = getaddrinfo("0.0.0.0", m_port, &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed: " << iResult << std::endl;
        cleanup();
        return false;
    }

    m_listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_listenSocket == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        cleanup();
        return false;
    }

    iResult = bind(m_listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "bind failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(m_listenSocket);
        cleanup();
        return false;
    }

    freeaddrinfo(result);

    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(m_listenSocket);
        cleanup();
        return false;
    }

    return true;
}

bool SocketServer::acceptConnection() {
    m_clientSocket = accept(m_listenSocket, NULL, NULL);
    if (m_clientSocket == INVALID_SOCKET) {
        std::cerr << "accept failed with error: " << WSAGetLastError() << std::endl;
        closesocket(m_listenSocket);
        cleanup();
        return false;
    }
    std::cout << "Client connected." << std::endl;
    return true;
}

bool SocketServer::sendMessage(const std::string& message) {
    int sendResult = send(m_clientSocket, message.c_str(), message.length(), 0);
    if (sendResult == SOCKET_ERROR) {
        std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

std::string SocketServer::receiveMessage() {
    char recvbuf[DEFAULT_BUFLEN];
    int recvResult = recv(m_clientSocket, recvbuf, DEFAULT_BUFLEN, 0);

    if (recvResult > 0) {
        return std::string(recvbuf, recvResult);
    }
    else if (recvResult == 0) {
        std::cout << "Connection closed by client" << std::endl;
        return "";
    }
    else {
        std::cerr << "recv failed with error: " << WSAGetLastError() << std::endl;
        return "";
    }
}

void SocketServer::closeClientConnection() {
    if (m_clientSocket != INVALID_SOCKET) {
        int iResult = shutdown(m_clientSocket, SD_SEND);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "shutdown failed with error: " << WSAGetLastError() << std::endl;
        }
        closesocket(m_clientSocket);
        m_clientSocket = INVALID_SOCKET;
    }
}

void SocketServer::cleanup() {
    if (m_clientSocket != INVALID_SOCKET) {
        closesocket(m_clientSocket);
        m_clientSocket = INVALID_SOCKET;
    }
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }
    if (m_initialized) {
        WSACleanup();
        m_initialized = false;
    }
}