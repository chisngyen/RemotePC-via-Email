#include "GUI.h"
#include <wx/filename.h>
#include <fstream>
#include <json/json.h>
#include "utils.h"
#include <windows.h>

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_BUTTON(ID_CONNECT, MainFrame::OnConnect)
EVT_BUTTON(ID_DISCONNECT, MainFrame::OnDisconnect)
EVT_BUTTON(ID_AUTHENTICATE, MainFrame::OnAuthenticate)
EVT_BUTTON(ID_SUBMIT_AUTH, MainFrame::OnSubmitAuth)
EVT_BUTTON(ID_START_MONITORING, MainFrame::OnStartMonitoring)
EVT_TIMER(ID_CHECK_EMAIL_TIMER, MainFrame::OnCheckEmail)
EVT_BUTTON(ID_LIST_APP, MainFrame::OnListApp)
EVT_BUTTON(ID_LIST_PROCESS, MainFrame::OnListProcess)
EVT_BUTTON(ID_LIST_SERVICE, MainFrame::OnListService)
EVT_BUTTON(ID_SCREENSHOT, MainFrame::OnScreenshot)
EVT_BUTTON(ID_OPEN_CAM, MainFrame::OnOpenCam)
EVT_BUTTON(ID_HELP, MainFrame::OnHelp)
END_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600))
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

    wxBoxSizer* connectionButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    btnConnect = new wxButton(panel, ID_CONNECT, "Connect");
    wxButton* btnDisconnect = new wxButton(panel, ID_DISCONNECT, "Disconnect");
    btnDisconnect->Disable();
    lblConnectionStatus = new wxStaticText(panel, wxID_ANY, "Status: OFF");

    connectionButtonSizer->Add(btnConnect, 0, wxALL, 5);
    connectionButtonSizer->Add(btnDisconnect, 0, wxALL, 5);
    connectionButtonSizer->Add(lblConnectionStatus, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    connectionSizer->Add(ipSizer, 0, wxEXPAND);
    connectionSizer->Add(portSizer, 0, wxEXPAND);
    connectionSizer->Add(connectionButtonSizer, 0, wxEXPAND);

    // Authentication section
    wxStaticBoxSizer* authSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Gmail Authentication");

    wxBoxSizer* authButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    btnAuthenticate = new wxButton(panel, ID_AUTHENTICATE, "Start Authentication");
    btnSubmitAuth = new wxButton(panel, ID_SUBMIT_AUTH, "Submit Auth Code");
    btnSubmitAuth->Disable();

    authButtonSizer->Add(btnAuthenticate, 0, wxALL, 5);
    authButtonSizer->Add(btnSubmitAuth, 0, wxALL, 5);

    txtAuthCode = new wxTextCtrl(panel, wxID_ANY);
    txtAuthCode->SetHint("Enter authentication code here");

    authSizer->Add(authButtonSizer, 0, wxEXPAND);
    authSizer->Add(txtAuthCode, 0, wxALL | wxEXPAND, 5);

    // Monitoring section
    wxStaticBoxSizer* monitoringSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Email Monitoring");
    btnStartMonitoring = new wxButton(panel, ID_START_MONITORING, "Start Monitoring");
    btnStartMonitoring->Disable();
    monitoringSizer->Add(btnStartMonitoring, 0, wxALL | wxEXPAND, 5);

    // Create a horizontal sizer for status and commands
    wxBoxSizer* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

    // Status section
    wxStaticBoxSizer* statusSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Status");
    txtStatus = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 150),
        wxTE_MULTILINE | wxTE_READONLY);
    statusSizer->Add(txtStatus, 1, wxALL | wxEXPAND, 5);

    // Commands section
    wxStaticBoxSizer* commandsSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Received Commands");
    txtCommands = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 150),
        wxTE_MULTILINE | wxTE_READONLY);
    commandsSizer->Add(txtCommands, 1, wxALL | wxEXPAND, 5);

    bottomSizer->Add(statusSizer, 1, wxEXPAND | wxRIGHT, 5);
    bottomSizer->Add(commandsSizer, 1, wxEXPAND | wxLEFT, 5);

    // Direct Command buttons
    wxStaticBoxSizer* directCommandsSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Direct Commands");
    wxFlexGridSizer* buttonGrid = new wxFlexGridSizer(3, 2, 5, 5);

    wxButton* btnListApp = new wxButton(panel, ID_LIST_APP, "List Applications");
    wxButton* btnListProcess = new wxButton(panel, ID_LIST_PROCESS, "List Processes");
    wxButton* btnListService = new wxButton(panel, ID_LIST_SERVICE, "List Services");
    wxButton* btnScreenshot = new wxButton(panel, ID_SCREENSHOT, "Take Screenshot");
    wxButton* btnOpenCam = new wxButton(panel, ID_OPEN_CAM, "Open Camera");
    wxButton* btnHelp = new wxButton(panel, ID_HELP, "Help");

    buttonGrid->Add(btnListApp, 0, wxEXPAND);
    buttonGrid->Add(btnListProcess, 0, wxEXPAND);
    buttonGrid->Add(btnListService, 0, wxEXPAND);
    buttonGrid->Add(btnScreenshot, 0, wxEXPAND);
    buttonGrid->Add(btnOpenCam, 0, wxEXPAND);
    buttonGrid->Add(btnHelp, 0, wxEXPAND);

    directCommandsSizer->Add(buttonGrid, 0, wxALL | wxEXPAND, 5);

    // Add all sections to main sizer
    mainSizer->Add(connectionSizer, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(authSizer, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(monitoringSizer, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(directCommandsSizer, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(bottomSizer, 1, wxALL | wxEXPAND, 5);

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

void MainFrame::UpdateConnectionStatus() {
    if (socketClient->isConnected()) {
        lblConnectionStatus->SetLabel("Status: ON");
        btnConnect->Disable();
        wxWindow* btnDisconnect = FindWindow(ID_DISCONNECT);
        if (btnDisconnect) btnDisconnect->Enable();
    }
    else {
        lblConnectionStatus->SetLabel("Status: OFF");
        btnConnect->Enable();
        wxWindow* btnDisconnect = FindWindow(ID_DISCONNECT);
        if (btnDisconnect) btnDisconnect->Disable();
    }
}

void MainFrame::UpdateCommandsList(const std::string& command) {
    wxDateTime now = wxDateTime::Now();
    txtCommands->AppendText(now.FormatTime() + ": " + command + "\n");
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
        UpdateConnectionStatus();
        btnAuthenticate->Enable();
    }
    else {
        UpdateStatus("Failed to connect to server");
    }
}

void MainFrame::OnDisconnect(wxCommandEvent& event) {
    if (socketClient->disconnect()) {
        UpdateStatus("Disconnected from server");
        UpdateConnectionStatus();
        btnAuthenticate->Disable();
        btnStartMonitoring->Disable();
        if (isMonitoring) {
            isMonitoring = false;
            checkEmailTimer->Stop();
        }
    }
    else {
        UpdateStatus("Failed to disconnect from server");
    }
}

void MainFrame::OnAuthenticate(wxCommandEvent& event) {
    if (oauth == nullptr) {
        oauth = new GoogleOAuth(clientId, clientSecret, REDIRECT_URI);
        std::string authUrl = oauth->getAuthUrl();

        if ((size_t)ShellExecuteA(NULL, "open", authUrl.c_str(), NULL, NULL, SW_SHOWNORMAL) > 32) {
            UpdateStatus("Browser opened for authentication. Please enter the code when redirected.");
            btnAuthenticate->Disable();
            btnSubmitAuth->Enable();
        }
        else {
            UpdateStatus("Failed to open browser. Please visit this URL manually:\n" + authUrl);
        }
    }
}

void MainFrame::OnSubmitAuth(wxCommandEvent& event) {
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
    btnSubmitAuth->Disable();
}

void MainFrame::OnCheckEmail(wxTimerEvent& event) {
    if (!emailHandler) return;

    EmailHandler::EmailInfo emailInfo = emailHandler->readNewestEmail();
    if (!emailInfo.content.empty() && emailInfo.subject == "Mail Control") {
        emailInfo.content = trim(emailInfo.content);
        UpdateStatus("Received new email: " + emailInfo.content);
        UpdateCommandsList(emailInfo.content);

        if (socketClient->sendData(emailInfo.content.c_str(), emailInfo.content.length()) == SOCKET_ERROR) {
            UpdateStatus("Failed to send email content to server");
            return;
        }

        std::string filename = "";
        if (emailInfo.content == "list app" || emailInfo.content == "list service") {
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

    void MainFrame::OnListApp(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        if (socketClient->sendData("list app", 8) == SOCKET_ERROR) {
            UpdateStatus("Failed to send list app command");
            return;
        }

        // Tạo và hiển thị Save File Dialog
        wxFileDialog saveFileDialog(this, "Save Applications List", "", "applications.txt",
            "Text files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL)
            return;

        // Lưu file với tên và đường dẫn đã chọn
        socketClient->receiveAndSaveFile(saveFileDialog.GetPath().ToStdString());
        UpdateStatus("Applications list saved to " + saveFileDialog.GetPath());

        // Tự động mở file sau khi lưu
        ShellExecuteA(NULL, "open", saveFileDialog.GetPath().c_str(), NULL, NULL, SW_SHOW);
    }

    void MainFrame::OnListProcess(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        if (socketClient->sendData("list process", 12) == SOCKET_ERROR) {
            UpdateStatus("Failed to send list process command");
            return;
        }

        wxFileDialog saveFileDialog(this, "Save Process List", "", "processes.txt",
            "Text files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL)
            return;

        socketClient->receiveAndSaveFile(saveFileDialog.GetPath().ToStdString());
        UpdateStatus("Process list saved to " + saveFileDialog.GetPath());

        ShellExecuteA(NULL, "open", saveFileDialog.GetPath().c_str(), NULL, NULL, SW_SHOW);
    }

    void MainFrame::OnListService(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        if (socketClient->sendData("list service", 12) == SOCKET_ERROR) {
            UpdateStatus("Failed to send list service command");
            return;
        }

        wxFileDialog saveFileDialog(this, "Save Services List", "", "services.txt",
            "Text files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL)
            return;

        socketClient->receiveAndSaveFile(saveFileDialog.GetPath().ToStdString());
        UpdateStatus("Services list saved to " + saveFileDialog.GetPath());

        ShellExecuteA(NULL, "open", saveFileDialog.GetPath().c_str(), NULL, NULL, SW_SHOW);
    }

    void MainFrame::OnScreenshot(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        if (socketClient->sendData("screenshot", 10) == SOCKET_ERROR) {
            UpdateStatus("Failed to send screenshot command");
            return;
        }

        wxFileDialog saveFileDialog(this, "Save Screenshot", "", "screenshot.png",
            "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL)
            return;

        socketClient->receiveAndSaveImage(saveFileDialog.GetPath().ToStdString());
        UpdateStatus("Screenshot saved to " + saveFileDialog.GetPath());

        ShellExecuteA(NULL, "open", saveFileDialog.GetPath().c_str(), NULL, NULL, SW_SHOW);
    }

    void MainFrame::OnOpenCam(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        if (socketClient->sendData("open_cam", 8) == SOCKET_ERROR) {
            UpdateStatus("Failed to send open cam command");
            return;
        }

        // Thêm dialog để chọn nơi lưu ảnh
        wxFileDialog saveFileDialog(this, "Save Webcam Image", "", "webcam.png",
            "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL)
            return;

        // Lưu ảnh vào đường dẫn đã chọn
        socketClient->receiveAndSaveImage(saveFileDialog.GetPath().ToStdString());
        UpdateStatus("Webcam image saved to " + saveFileDialog.GetPath());

        // Mở ảnh sau khi lưu
        ShellExecuteA(NULL, "open", saveFileDialog.GetPath().c_str(), NULL, NULL, SW_SHOW);
    }

    void MainFrame::OnHelp(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        if (socketClient->sendData("help", 4) == SOCKET_ERROR) {
            UpdateStatus("Failed to send help command");
            return;
        }

        wxFileDialog saveFileDialog(this, "Save Help Information", "", "help.txt",
            "Text files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL)
            return;

        socketClient->receiveAndSaveFile(saveFileDialog.GetPath().ToStdString());
        UpdateStatus("Help information saved to " + saveFileDialog.GetPath());

        ShellExecuteA(NULL, "open", saveFileDialog.GetPath().c_str(), NULL, NULL, SW_SHOW);
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

    void MainFrame::UpdateStatus(const wxString& message) {
        wxDateTime now = wxDateTime::Now();
        txtStatus->AppendText(now.FormatTime() + ": " + message + "\n");
    }