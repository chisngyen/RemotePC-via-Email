#pragma once
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
using namespace std;
#define BUFFER_SIZE 4096

class SocketClient {
private:
    SOCKET clientSocket;
    bool isInitialized;
    void saveToFile(const string& filename, const string& data);

public:
    SocketClient();
    ~SocketClient();

    bool connect(const string& serverIP, int port);
    bool disconnect();
    int sendData(const char* data, int dataSize);
    int receiveData(char* buffer, int bufferSize);
    void sendCommand(const string& command);
    void receiveAndSaveFile(const string& filename);
    void receiveVideoData(const string& filename);
    void receiveAndSaveImage(const string& filename);
    void cleanup();
    bool isConnected() const;

    bool connected;
};