#include "socket.h"
#include <iostream>
#include <fstream>

SocketClient::SocketClient() : clientSocket(INVALID_SOCKET), isInitialized(false) {
    WSADATA wsaData;
    isInitialized = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
    if (!isInitialized) {
        cerr << "Failed to initialize Winsock" << endl;
    }
}

SocketClient::~SocketClient() {
    cleanup();
}

void SocketClient::saveToFile(const string& filename, const string& data) {
    ofstream outFile(filename, ios::binary);
    if (outFile.is_open()) {
        outFile << data;
        outFile.close();
        cout << "Data saved to " << filename << endl;
    }
    else {
        cerr << "Unable to open file for writing: " << filename << endl;
    }
}

bool SocketClient::connect(const string& serverIP, int port) {
    if (!isInitialized) return false;

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Error creating socket: " << WSAGetLastError() << endl;
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

    if (::connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Failed to connect: " << WSAGetLastError() << endl;
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
    }

    return true;
}

bool SocketClient::disconnect() {
    if (!connected) {
        return true;  // Đã disconnect rồi
    }

    if (closesocket(clientSocket) == SOCKET_ERROR) {
        return false;
    }

    connected = false;
    clientSocket = INVALID_SOCKET;
    return true;
}

int SocketClient::sendData(const char* data, int dataSize) {
    if (clientSocket == INVALID_SOCKET) return SOCKET_ERROR;
    return send(clientSocket, data, dataSize, 0);
}

int SocketClient::receiveData(char* buffer, int bufferSize) {
    if (clientSocket == INVALID_SOCKET) return SOCKET_ERROR;
    return recv(clientSocket, buffer, bufferSize, 0);
}

void SocketClient::sendCommand(const string& command) {
    if (clientSocket == INVALID_SOCKET) return;
    send(clientSocket, command.c_str(), command.size(), 0);
}

void SocketClient::receiveAndSaveFile(const string& filename) {
    if (clientSocket == INVALID_SOCKET) return;

    string data;
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    do {
        bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            data.append(buffer, bytesReceived);
        }
    } while (bytesReceived == BUFFER_SIZE);

    saveToFile(filename, data);
}

void SocketClient::receiveAndSaveImage(const string& filename) {
    if (clientSocket == INVALID_SOCKET) return;

    int imageSize;
    recv(clientSocket, (char*)&imageSize, sizeof(imageSize), 0);

    vector<char> imageData(imageSize);
    int totalReceived = 0;

    while (totalReceived < imageSize) {
        int bytesReceived = recv(clientSocket, imageData.data() + totalReceived,
            imageSize - totalReceived, 0);
        if (bytesReceived <= 0) break;
        totalReceived += bytesReceived;
    }

    saveToFile(filename, string(imageData.begin(), imageData.end()));
}

void SocketClient::cleanup() {
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
    if (isInitialized) {
        WSACleanup();
        isInitialized = false;
    }
}

bool SocketClient::isConnected() const {
    return clientSocket != INVALID_SOCKET;
}