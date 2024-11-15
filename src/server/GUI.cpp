#include "GUI.h"
#include <wx/wx.h>
#include <wx/stattext.h>
#include <sstream>
#include <thread>
#include <vector>
#include <string>

// Define custom event
wxDEFINE_EVENT(wxEVT_SERVER_LOG, wxCommandEvent);

// Event table
BEGIN_EVENT_TABLE(ServerFrame, wxFrame)
EVT_BUTTON(1001, ServerFrame::OnStart)
EVT_BUTTON(1002, ServerFrame::OnStop)
EVT_CLOSE(ServerFrame::OnClose)
EVT_COMMAND(wxID_ANY, wxEVT_SERVER_LOG, ServerFrame::OnLogEvent)
END_EVENT_TABLE()

// Constructor implementation
ServerFrame::ServerFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600))
{
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Top bar with status
    wxBoxSizer* topBarSizer = new wxBoxSizer(wxHORIZONTAL);

    // Button group
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    startButton = new wxButton(panel, 1001, "Start Server");
    stopButton = new wxButton(panel, 1002, "Stop Server");
    stopButton->Disable();
    buttonSizer->Add(startButton, 0, wxALL, 5);
    buttonSizer->Add(stopButton, 0, wxALL, 5);

    // Status indicator
    wxBoxSizer* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* statusLabel = new wxStaticText(panel, wxID_ANY, "Status:");
    statusText = new wxStaticText(panel, wxID_ANY, "OFF");
    statusText->SetForegroundColour(*wxRED);

    statusSizer->Add(statusLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    statusSizer->Add(statusText, 0, wxALIGN_CENTER_VERTICAL);

    // Add button group and status to top bar
    topBarSizer->Add(buttonSizer, 0, wxALIGN_CENTER_VERTICAL);
    topBarSizer->AddStretchSpacer();
    topBarSizer->Add(statusSizer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    mainSizer->Add(topBarSizer, 0, wxEXPAND | wxALL, 5);

    // Client list
    clientList = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(-1, 150),
        wxLC_REPORT | wxLC_SINGLE_SEL);
    clientList->InsertColumn(0, "Client IP", wxLIST_FORMAT_LEFT, 150);
    clientList->InsertColumn(1, "Status", wxLIST_FORMAT_LEFT, 100);
    mainSizer->Add(clientList, 0, wxEXPAND | wxALL, 5);

    // Communication log area
    wxStaticText* logLabel = new wxStaticText(panel, wxID_ANY, "Client Communication Log:");
    mainSizer->Add(logLabel, 0, wxLEFT | wxRIGHT | wxTOP, 5);

    logArea = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
    mainSizer->Add(logArea, 1, wxEXPAND | wxALL, 5);

    panel->SetSizer(mainSizer);

    server = nullptr;
    serverThread = nullptr;
    isRunning = false;

    Center();
}

// Destructor implementation
ServerFrame::~ServerFrame() {
    StopServer();
}

// Event handlers
void ServerFrame::OnStart(wxCommandEvent& event) {
    if (!isRunning) {
        StartServer();
    }
}

void ServerFrame::OnStop(wxCommandEvent& event) {
    if (isRunning) {
        StopServer();
    }
}

void ServerFrame::OnClose(wxCloseEvent& event) {
    StopServer();
    event.Skip();
}

void ServerFrame::OnLogEvent(wxCommandEvent& event) {
    logArea->AppendText(event.GetString() + "\n");
}

// Server control methods
void ServerFrame::StartServer() {
    server = new SocketServer(DEFAULT_PORT);

    if (!server->initialize()) {
        LogMessage("Failed to initialize Winsock.");
        return;
    }

    if (!server->createListener()) {
        LogMessage("Failed to create listening socket.");
        delete server;
        server = nullptr;
        return;
    }

    isRunning = true;
    statusText->SetLabel("ON");
    statusText->SetForegroundColour(*wxGREEN);
    startButton->Disable();
    stopButton->Enable();

    serverThread = new std::thread(&ServerFrame::ServerLoop, this);
}

void ServerFrame::StopServer() {
    if (isRunning) {
        isRunning = false;

        if (server) {
            server->closeClientConnection();
            delete server;
            server = nullptr;
        }

        if (serverThread) {
            serverThread->join();
            delete serverThread;
            serverThread = nullptr;
        }

        statusText->SetLabel("OFF");
        statusText->SetForegroundColour(*wxRED);
        startButton->Enable();
        stopButton->Disable();
        clientList->DeleteAllItems();
    }
}

void ServerFrame::ServerLoop() {
    while (isRunning) {
        if (!server->acceptConnection()) {
            continue;
        }

        LogMessage("New client connected");
        UpdateClientList();

        while (isRunning) {
            string command = server->receiveMessage();

            if (!isRunning) break;

            if (command.empty()) {
                LogMessage("Client disconnected");
                break;
            }

            // Log client request
            LogMessage("Client: " + command);

            // Process commands and log responses
            if (command == "list app") {
                string applist = cmd.Applist();
                if (server->sendMessage(applist)) {
                    LogMessage("Server: Sent application list");
                }
            }
            else if (command == "list service") {
                string servicelist = cmd.Listservice();
                if (server->sendMessage(servicelist)) {
                    LogMessage("Server: Sent service list");
                }
            }
            else if (command == "list process") {
                string processList = cmd.Listprocess();
                if (server->sendMessage(processList)) {
                    LogMessage("Server: Sent process list");
                }
            }
            else if (command == "help") {
                string helps = cmd.help();
                if (server->sendMessage(helps)) {
                    LogMessage("Server: Sent help information");
                }
            }
            else if (command == "screenshot") {
                int width, height;
                vector<BYTE> image = cmd.captureScreenWithGDIPlus(width, height);
                cmd.sendImage(server->getClientSocket(), image);
                LogMessage("Server: Sent screenshot");
            }
            else if (command == "shutdown") {
                LogMessage("Server: Executing shutdown command");
                cmd.shutdownComputer();
            }
            else if (command == "open_cam") {
                cmd.openCamera();
                Sleep(2000);
                int width, height;
                vector<BYTE> image = cmd.captureScreenWithGDIPlus(width, height);
                cmd.sendImage(server->getClientSocket(), image);
                LogMessage("Server: Camera has been opened");
            }
        }

        server->closeClientConnection();
        clientList->DeleteAllItems();
    }
}
// GUI update methods
void ServerFrame::UpdateClientList() {
    wxCommandEvent* event = new wxCommandEvent(wxEVT_SERVER_LOG);
    event->SetString("Updating client list...");
    wxQueueEvent(this, event);

    clientList->DeleteAllItems();
    if (server && server->getClientSocket() != INVALID_SOCKET) {
        sockaddr_in addr;
        int addrlen = sizeof(addr);
        getpeername(server->getClientSocket(), (sockaddr*)&addr, &addrlen);
        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ipstr, sizeof(ipstr));

        long itemIndex = clientList->InsertItem(0, ipstr);
        clientList->SetItem(itemIndex, 1, "Connected");
    }
}

void ServerFrame::LogMessage(const wxString& message) {
    wxCommandEvent* event = new wxCommandEvent(wxEVT_SERVER_LOG);
    event->SetString(message);
    wxQueueEvent(this, event);
}