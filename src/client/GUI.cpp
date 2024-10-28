#include "GUI.h"
#include <wx/filename.h>
#include <fstream>
#include <json/json.h>
#include "utils.h"
#include <windows.h>

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_BUTTON(ID_CONNECT, MainFrame::OnConnect)
EVT_BUTTON(ID_AUTHENTICATE, MainFrame::OnAuthenticate)
EVT_BUTTON(ID_START_MONITORING, MainFrame::OnStartMonitoring)
EVT_TIMER(ID_CHECK_EMAIL_TIMER, MainFrame::OnCheckEmail)
END_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(600, 400))
{
    // Initialize members
    socketClient = new SocketClient();
    oauth = nullptr;
    emailHandler = nullptr;
    isMonitoring = false;
    checkEmailTimer = new wxTimer(this, ID_CHECK_EMAIL_TIMER);

    // Create main panel
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Connection section
    wxStaticBoxSizer* connectionSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Server Connection");

    wxBoxSizer* ipSizer = new wxBoxSizer(wxHORIZONTAL);
    ipSizer->Add(new wxStaticText(panel, wxID_ANY, "IP Address:"), 0, wxALL, 5);
    txtIpAddress = new wxTextCtrl(panel, wxID_ANY);
    ipSizer->Add(txtIpAddress, 1, wxALL, 5);

    wxBoxSizer* portSizer = new wxBoxSizer(wxHORIZONTAL);
    portSizer->Add(new wxStaticText(panel, wxID_ANY, "Port:"), 0, wxALL, 5);
    txtPort = new wxTextCtrl(panel, wxID_ANY);
    portSizer->Add(txtPort, 1, wxALL, 5);

    btnConnect = new wxButton(panel, ID_CONNECT, "Connect");

    connectionSizer->Add(ipSizer, 0, wxEXPAND);
    connectionSizer->Add(portSizer, 0, wxEXPAND);
    connectionSizer->Add(btnConnect, 0, wxALL | wxALIGN_RIGHT, 5);

    // Authentication section
    wxStaticBoxSizer* authSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Gmail Authentication");

    btnAuthenticate = new wxButton(panel, ID_AUTHENTICATE, "Start Authentication");
    txtAuthCode = new wxTextCtrl(panel, wxID_ANY);
    txtAuthCode->SetHint("Enter authentication code here");

    authSizer->Add(btnAuthenticate, 0, wxALL | wxEXPAND, 5);
    authSizer->Add(txtAuthCode, 0, wxALL | wxEXPAND, 5);

    // Monitoring section
    wxStaticBoxSizer* monitoringSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Email Monitoring");
    btnStartMonitoring = new wxButton(panel, ID_START_MONITORING, "Start Monitoring");
    btnStartMonitoring->Disable();
    monitoringSizer->Add(btnStartMonitoring, 0, wxALL | wxEXPAND, 5);

    // Status section
    wxStaticBoxSizer* statusSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Status");
    txtStatus = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 100),
        wxTE_MULTILINE | wxTE_READONLY);
    statusSizer->Add(txtStatus, 1, wxALL | wxEXPAND, 5);

    // Add all sections to main sizer
    mainSizer->Add(connectionSizer, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(authSizer, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(monitoringSizer, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(statusSizer, 1, wxALL | wxEXPAND, 5);

    panel->SetSizer(mainSizer);

    // Load client secrets
    LoadClientSecrets();
}

MainFrame::~MainFrame() {
    delete socketClient;
    delete oauth;
    delete emailHandler;
    delete checkEmailTimer;
}

void MainFrame::LoadClientSecrets() {
    try {
        std::ifstream file("client_secret.json");
        if (!file.is_open()) {
            UpdateStatus("Error: Unable to open client_secret.json");
            return;
        }

        Json::Value root;
        file >> root;
        file.close();

        clientId = root["web"]["client_id"].asString();
        clientSecret = root["web"]["client_secret"].asString();

        if (clientId.empty() || clientSecret.empty()) {
            UpdateStatus("Error: Invalid client_secret.json format");
            return;
        }

        UpdateStatus("Client secrets loaded successfully");
    }
    catch (const std::exception& e) {
        UpdateStatus(wxString::Format("Error loading client secrets: %s", e.what()));
    }
}

void MainFrame::OnConnect(wxCommandEvent& event) {
    wxString ipAddress = txtIpAddress->GetValue();
    long port;
    if (!txtPort->GetValue().ToLong(&port)) {
        UpdateStatus("Error: Invalid port number");
        return;
    }

    if (socketClient->connect(ipAddress.ToStdString(), static_cast<int>(port))) {
        UpdateStatus("Successfully connected to server");
        btnAuthenticate->Enable();
    }
    else {
        UpdateStatus("Failed to connect to server");
    }
}

void MainFrame::OnAuthenticate(wxCommandEvent& event) {
    if (oauth == nullptr) {
        oauth = new GoogleOAuth(clientId, clientSecret, REDIRECT_URI);
        std::string authUrl = oauth->getAuthUrl();

        // Open browser with auth URL
        if ((size_t)ShellExecuteA(NULL, "open", authUrl.c_str(), NULL, NULL, SW_SHOWNORMAL) > 32) {
            UpdateStatus("Browser opened for authentication. Please enter the code when redirected.");
        }
        else {
            UpdateStatus("Failed to open browser. Please visit this URL manually:\n" + authUrl);
        }
    }
    else {
        // Get authentication code and request tokens
        std::string authCode = txtAuthCode->GetValue().ToStdString();
        if (authCode.empty()) {
            UpdateStatus("Please enter the authentication code");
            return;
        }

        refreshToken = oauth->getRefreshToken(authCode);
        if (refreshToken.empty()) {
            UpdateStatus("Failed to get refresh token");
            return;
        }

        accessToken = oauth->getAccessToken(refreshToken);
        if (accessToken.empty()) {
            UpdateStatus("Failed to get access token");
            return;
        }

        emailHandler = new EmailHandler(accessToken);
        UpdateStatus("Authentication successful");
        btnStartMonitoring->Enable();
    }
}

void MainFrame::OnStartMonitoring(wxCommandEvent& event) {
    if (!isMonitoring) {
        isMonitoring = true;
        btnStartMonitoring->SetLabel("Stop Monitoring");
        checkEmailTimer->Start(2000); // Check every 2 seconds
        UpdateStatus("Started monitoring emails");
    }
    else {
        isMonitoring = false;
        btnStartMonitoring->SetLabel("Start Monitoring");
        checkEmailTimer->Stop();
        UpdateStatus("Stopped monitoring emails");
    }
}

void MainFrame::OnCheckEmail(wxTimerEvent& event) {
    if (!emailHandler) return;

    EmailHandler::EmailInfo emailInfo = emailHandler->readNewestEmail();
    if (!emailInfo.content.empty() && emailInfo.subject == "Mail Control") {
        emailInfo.content = trim(emailInfo.content);
        UpdateStatus("Received new email: " + emailInfo.content);

        // Send email content to server
        if (socketClient->sendData(emailInfo.content.c_str(), emailInfo.content.length()) == SOCKET_ERROR) {
            UpdateStatus("Failed to send email content to server");
            return;
        }

        // Handle different commands
        std::string filename = "";
        if (emailInfo.content == "list app" || emailInfo.content == "list service") {
            filename = (emailInfo.content == "list app") ? "applications.txt" : "services.txt";
            socketClient->receiveAndSaveFile(filename);
        }
        else if (emailInfo.content == "list process" || emailInfo.content == "help") {
            filename = (emailInfo.content == "list process") ? "processes.txt" : "help.txt";
            socketClient->receiveAndSaveFile(filename);
        }
        else if (emailInfo.content == "screenshot" || emailInfo.content == "open_cam") {
            filename = (emailInfo.content == "screenshot") ? "screenshot.png" : "webcam_screenshot.png";
            socketClient->receiveAndSaveImage(filename);
        }
        else if (emailInfo.content.substr(0, 4) == "view") {
            filename = "received_file.txt";
            socketClient->receiveAndSaveFile(filename);
        }

        // Send reply email
        std::string replyMessage = "This is an automated reply to your email: " + emailInfo.content;
        bool success = emailHandler->sendReplyEmail(
            emailInfo.from,
            emailInfo.subject,
            replyMessage,
            emailInfo.threadId,
            filename
        );

        if (success) {
            UpdateStatus("Reply sent successfully with attachment");
        }
        else {
            UpdateStatus("Failed to send reply with attachment");
        }
    }
}

void MainFrame::UpdateStatus(const wxString& message) {
    wxDateTime now = wxDateTime::Now();
    txtStatus->AppendText(now.FormatTime() + ": " + message + "\n");
}