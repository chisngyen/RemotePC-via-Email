#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "socket.h"
#include "CommandExecutor.h"
#include <thread>

#define DEFAULT_PORT "27015"

// Custom event for logging
wxDECLARE_EVENT(wxEVT_SERVER_LOG, wxCommandEvent);

class ServerFrame : public wxFrame {
public:
    ServerFrame(const wxString& title);
    virtual ~ServerFrame();

private:
    // GUI Controls
    wxButton* startButton;
    wxButton* stopButton;
    wxStaticText* statusText;
    wxListCtrl* clientList;
    wxTextCtrl* logArea;

    // Server components
    SocketServer* server;
    std::thread* serverThread;
    bool isRunning;
    Command cmd;

    // Event handling methods
    void OnStart(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnLogEvent(wxCommandEvent& event);

    // Server methods
    void StartServer();
    void StopServer();
    void ServerLoop();
    void UpdateClientList();
    void LogMessage(const wxString& message);

    DECLARE_EVENT_TABLE()
};
