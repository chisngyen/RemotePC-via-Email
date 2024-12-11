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
    ~ServerFrame();

private:
    void FormatCommand(wxString& command);
    void OnClearHistory(wxCommandEvent& event);

    void OnButtonHover(wxMouseEvent& event);
    void OnButtonLeave(wxMouseEvent& event);

    // GUI Controls
    wxButton* startButton;
    wxButton* stopButton;
    wxStaticText* statusText;
    wxListCtrl* logList;
    wxTextCtrl* messageLog;  // For normal messages

    // Server components
    SocketServer* server;
    std::thread* serverThread;
    bool isRunning;
    Command cmd;

    // Event handlers
    void OnStart(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnLogEvent(wxCommandEvent& event);
    void OnLogItemDblClick(wxListEvent& event);

    // Server control methods
    void StartServer();
    void StopServer();
    void ServerLoop();

    // Logging methods
    void LogMessage(const wxString& message, const wxString& details = "", bool isCommand = false);

    // Store command responses for double-click viewing
    struct LogEntry {
        wxString message;
        wxString details;
        bool isCommand;
        wxString savedPath;  
    };
    std::vector<LogEntry> logEntries;

    DECLARE_EVENT_TABLE()
};
