#include "GUI.h"
#include <wx/wx.h>
#include <wx/stattext.h>
#include <sstream>
#include <fstream>
#include <ctime>
#include <iomanip>

wxDEFINE_EVENT(wxEVT_SERVER_LOG, wxCommandEvent);

BEGIN_EVENT_TABLE(ServerFrame, wxFrame)
EVT_BUTTON(1001, ServerFrame::OnStart)
EVT_BUTTON(1002, ServerFrame::OnStop)
EVT_CLOSE(ServerFrame::OnClose)
EVT_COMMAND(wxID_ANY, wxEVT_SERVER_LOG, ServerFrame::OnLogEvent)
EVT_LIST_ITEM_ACTIVATED(wxID_ANY, ServerFrame::OnLogItemDblClick)
END_EVENT_TABLE()

ServerFrame::ServerFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600))
{
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(wxColour(30, 33, 41));

    wxFont defaultFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    panel->SetFont(defaultFont);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Server Control Section
    wxStaticBoxSizer* controlSizer = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Server Control");
    wxStaticBox* controlBox = controlSizer->GetStaticBox();
    controlBox->SetForegroundColour(wxColour(255, 255, 255));
    controlBox->SetBackgroundColour(wxColour(37, 41, 51));

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    startButton = new wxButton(panel, 1001, "Start Server", wxDefaultPosition, wxSize(100, 30));
    startButton->SetBackgroundColour(wxColour(0, 150, 136));
    startButton->SetForegroundColour(wxColour(255, 255, 255));

    stopButton = new wxButton(panel, 1002, "Stop Server", wxDefaultPosition, wxSize(100, 30));
    stopButton->SetBackgroundColour(wxColour(239, 83, 80));
    stopButton->SetForegroundColour(wxColour(255, 255, 255));
    stopButton->Disable();

    buttonSizer->Add(startButton, 0, wxALL, 5);
    buttonSizer->Add(stopButton, 0, wxALL, 5);

    wxBoxSizer* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* statusLabel = new wxStaticText(panel, wxID_ANY, "Status:");
    statusLabel->SetForegroundColour(wxColour(255, 255, 255));

    statusText = new wxStaticText(panel, wxID_ANY, "OFF");
    statusText->SetForegroundColour(*wxRED);
    statusText->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    statusSizer->Add(statusLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    statusSizer->Add(statusText, 0, wxALIGN_CENTER_VERTICAL);

    controlSizer->Add(buttonSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    controlSizer->AddStretchSpacer();
    controlSizer->Add(statusSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 10);

    mainSizer->Add(controlSizer, 0, wxEXPAND | wxALL, 10);

    // Client Commands Log Section
    wxStaticBoxSizer* commandLogSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Client Commands");
    wxStaticBox* commandLogBox = commandLogSizer->GetStaticBox();
    commandLogBox->SetForegroundColour(wxColour(255, 255, 255));
    commandLogBox->SetBackgroundColour(wxColour(37, 41, 51));

    logList = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(-1, 200),
        wxLC_REPORT | wxLC_SINGLE_SEL);
    logList->SetBackgroundColour(wxColour(45, 48, 58));
    logList->SetForegroundColour(wxColour(255, 255, 255));

    logList->InsertColumn(0, "Time", wxLIST_FORMAT_LEFT, 150);
    logList->InsertColumn(1, "Command", wxLIST_FORMAT_LEFT, 500);

    commandLogSizer->Add(logList, 0, wxEXPAND | wxALL, 5);

    mainSizer->Add(commandLogSizer, 0, wxEXPAND | wxALL, 10);

    // Server Messages Log Section
    wxStaticBoxSizer* messageLogSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Server Messages");
    wxStaticBox* messageLogBox = messageLogSizer->GetStaticBox();
    messageLogBox->SetForegroundColour(wxColour(255, 255, 255));
    messageLogBox->SetBackgroundColour(wxColour(37, 41, 51));

    messageLog = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
    messageLog->SetBackgroundColour(wxColour(45, 48, 58));
    messageLog->SetForegroundColour(wxColour(255, 255, 255));

    messageLogSizer->Add(messageLog, 1, wxEXPAND | wxALL, 5);

    mainSizer->Add(messageLogSizer, 1, wxEXPAND | wxALL, 10);

    panel->SetSizer(mainSizer);

    server = nullptr;
    serverThread = nullptr;
    isRunning = false;

    Center();
}

ServerFrame::~ServerFrame() {
    StopServer();
}

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
    LogEntry* entry = static_cast<LogEntry*>(event.GetClientData());
    if (!entry) return;

    if (entry->isCommand) {
        // Get current time
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::ostringstream timeStr;
        timeStr << std::put_time(&tm, "%H:%M:%S");

        // Add to list control
        long itemIndex = logList->InsertItem(logList->GetItemCount(), timeStr.str());
        logList->SetItem(itemIndex, 1, entry->message);

        // Store the log entry
        logEntries.push_back(*entry);
    }
    else {
        // Add to message log
        messageLog->AppendText(entry->message + "\n");
    }

    delete entry;
}

void ServerFrame::OnLogItemDblClick(wxListEvent& event) {
    long index = event.GetIndex();
    if (index >= 0 && static_cast<size_t>(index) < logEntries.size()) {
        const LogEntry& entry = logEntries[index];

        // N?u có ???ng d?n ?ã l?u thì m? file
        if (!entry.savedPath.IsEmpty()) {
            wxLaunchDefaultApplication(entry.savedPath);
        }
    }
}

void ServerFrame::StartServer() {
    server = new SocketServer(DEFAULT_PORT);

    if (!server->initialize()) {
        LogMessage("Failed to initialize Winsock", "", false);
        return;
    }

    if (!server->createListener()) {
        LogMessage("Failed to create listening socket", "", false);
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
    }
}

void ServerFrame::ServerLoop() {
    while (isRunning) {
        if (!server->acceptConnection()) {
            continue;
        }

        LogMessage("New client connected", "", false);

        while (isRunning) {
            string command = server->receiveMessage();

            if (!isRunning) break;

            if (command.empty()) {
                LogMessage("Client disconnected", "", false);
                break;
            }

            if (command.substr(0, 10) == "save_path:") {
                string path = command.substr(10);
                // Tìm và c?p nh?t LogEntry cu?i cùng v?i ???ng d?n file
                if (!logEntries.empty()) {
                    logEntries.back().savedPath = wxString::FromUTF8(path);
                }
                continue;
            }

            string response;
            vector<BYTE> imageData;

            // Log command t? client
            LogMessage(command, "", true);

            if (command == "list app") {
                response = cmd.Applist();
                server->sendMessage(response);
                LogMessage("Server: Sent application list", response, false);
            }
            else if (command == "list service") {
                response = cmd.Listservice();
                server->sendMessage(response);
                LogMessage("Server: Sent service list", response, false);
            }
            else if (command == "list process") {
                response = cmd.Listprocess();
                server->sendMessage(response);
                LogMessage("Server: Sent process list", response, false);
            }
            else if (command == "help") {
                response = cmd.help();
                server->sendMessage(response);
                LogMessage("Server: Sent help information", response, false);
            }
            else if (command == "screenshot") {
                int width, height;
                imageData = cmd.captureScreenWithGDIPlus(width, height);
                cmd.sendImage(server->getClientSocket(), imageData);
                LogMessage("Server: Sent screenshot", "Screenshot taken", false);
            }
            else if (command == "shutdown") {
                LogMessage("Server: Executing shutdown command", "", false);
                cmd.shutdownComputer();
            }
            else if (command == "open_cam") {
                cmd.openCamera();
                Sleep(2000);
                int width, height;
                imageData = cmd.captureScreenWithGDIPlus(width, height);
                cmd.sendImage(server->getClientSocket(), imageData);
                LogMessage("Server: Camera capture taken", "", false);
            }
        }

        server->closeClientConnection();
    }
}

void ServerFrame::LogMessage(const wxString& message, const wxString& details, bool isCommand) {
    LogEntry* entry = new LogEntry{
        message,
        details,
        isCommand
    };

    wxCommandEvent* event = new wxCommandEvent(wxEVT_SERVER_LOG);
    event->SetClientData(entry);
    wxQueueEvent(this, event);
}