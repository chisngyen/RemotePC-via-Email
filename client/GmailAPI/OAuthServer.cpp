#include "OAuthServer.h"
#include <regex>
#include <iostream>

wxDEFINE_EVENT(wxEVT_OAUTH_CODE, wxCommandEvent);

OAuthCallbackServer::OAuthCallbackServer(wxEvtHandler* handler, int port)
    : port(port), running(false), serverSocket(INVALID_SOCKET), eventHandler(handler) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

OAuthCallbackServer::~OAuthCallbackServer() {
    stop();
    WSACleanup();
}

bool OAuthCallbackServer::startListening() {
    if (running) return false;

    running = true;
    listenerThread = std::thread(&OAuthCallbackServer::listenForCode, this);
    return true;
}

void OAuthCallbackServer::listenForCode() {
    if (!initializeServer()) {
        wxCommandEvent event(wxEVT_OAUTH_CODE);
        event.SetString("SERVER_ERROR");
        wxPostEvent(eventHandler, event);
        return;
    }

    char buffer[4096];
    fd_set readSet;
    timeval timeout;

    while (running) {
        FD_ZERO(&readSet);
        FD_SET(serverSocket, &readSet);

        timeout.tv_sec = 1;  // 1 second timeout for checking running flag
        timeout.tv_usec = 0;

        int result = select(0, &readSet, nullptr, nullptr, &timeout);

        if (!running) break;

        if (result > 0 && FD_ISSET(serverSocket, &readSet)) {
            SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
            if (clientSocket != INVALID_SOCKET) {
                int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (bytesReceived > 0) {
                    buffer[bytesReceived] = '\0';
                    std::string request(buffer);
                    std::string code = extractCode(request);

                    if (!code.empty()) {
                        sendResponse(clientSocket);

                        // Post event to main thread with the auth code
                        wxCommandEvent event(wxEVT_OAUTH_CODE);
                        event.SetString(code);
                        wxPostEvent(eventHandler, event);

                        closesocket(clientSocket);
                        break;  // Exit after receiving the code
                    }
                }
                closesocket(clientSocket);
            }
        }
    }

    closesocket(serverSocket);
    serverSocket = INVALID_SOCKET;
}

bool OAuthCallbackServer::initializeServer() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        return false;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        return false;
    }

    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        closesocket(serverSocket);
        return false;
    }

    return true;
}

std::string OAuthCallbackServer::extractCode(const std::string& request) {
    std::regex codeRegex("code=([^&\\s]+)");
    std::smatch matches;
    if (std::regex_search(request, matches, codeRegex)) {
        return matches[1];
    }
    return "";
}

void OAuthCallbackServer::sendResponse(SOCKET clientSocket) {
    const char* response = "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html><body><h1>Authorization successful!</h1>"
        "<p>You can close this window now.</p></body></html>";
    send(clientSocket, response, strlen(response), 0);
}

void OAuthCallbackServer::stop() {
    running = false;
    if (listenerThread.joinable()) {
        listenerThread.join();
    }
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
    }
}