#pragma once
#include <string>
#include <functional>
#include <thread>
#include <WinSock2.h>
#include <wx/event.h>

// Định nghĩa custom event cho OAuth callback
wxDECLARE_EVENT(wxEVT_OAUTH_CODE, wxCommandEvent);

class OAuthCallbackServer {
public:
    OAuthCallbackServer(wxEvtHandler* eventHandler, int port = 8080);
    ~OAuthCallbackServer();

    bool startListening();
    void stop();

private:
    bool running;
    SOCKET serverSocket;
    std::thread listenerThread;
    int port;
    wxEvtHandler* eventHandler;

    void listenForCode();
    bool initializeServer();
    std::string extractCode(const std::string& request);
    void sendResponse(SOCKET clientSocket);
};