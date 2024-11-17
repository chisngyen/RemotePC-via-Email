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

    // Create main panel with dark theme background (Rich dark blue-gray)
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(wxColour(30, 33, 41));

    // Set default font for better readability on dark background
    wxFont defaultFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    panel->SetFont(defaultFont);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Connection section
    wxStaticBoxSizer* connectionSizer = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Server Connection");
    wxStaticBox* connBox = connectionSizer->GetStaticBox();
    connBox->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho tiêu đề
    connBox->SetBackgroundColour(wxColour(37, 41, 51));

    wxBoxSizer* ipPortSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* ipLabel = new wxStaticText(panel, wxID_ANY, "IP Address:");
    ipLabel->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho label
    txtIpAddress = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(150, -1));
    txtIpAddress->SetBackgroundColour(wxColour(45, 48, 58));
    txtIpAddress->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho text input

    wxStaticText* portLabel = new wxStaticText(panel, wxID_ANY, "Port:");
    portLabel->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho label
    txtPort = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(100, -1));
    txtPort->SetBackgroundColour(wxColour(45, 48, 58));
    txtPort->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho text input

    ipPortSizer->Add(ipLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    ipPortSizer->Add(txtIpAddress, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);
    ipPortSizer->Add(portLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    ipPortSizer->Add(txtPort, 0, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* connectionButtonSizer = new wxBoxSizer(wxHORIZONTAL);

    // Create modern-looking connect button (Accent color: Teal)
    btnConnect = new wxButton(panel, ID_CONNECT, "Connect", wxDefaultPosition, wxSize(100, 30));
    btnConnect->SetBackgroundColour(wxColour(0, 150, 136));
    btnConnect->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho button text

    wxButton* btnDisconnect = new wxButton(panel, ID_DISCONNECT, "Disconnect", wxDefaultPosition, wxSize(100, 30));
    btnDisconnect->SetBackgroundColour(wxColour(239, 83, 80));
    btnDisconnect->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho button text
    btnDisconnect->Disable();

    lblConnectionStatus = new wxStaticText(panel, wxID_ANY, "Status: OFF");
    lblConnectionStatus->SetForegroundColour(wxColour(255, 99, 71));  // Tomato red cho status offline

    connectionButtonSizer->Add(btnConnect, 0, wxALL, 5);
    connectionButtonSizer->Add(btnDisconnect, 0, wxALL, 5);
    connectionButtonSizer->Add(lblConnectionStatus, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 20);

    connectionSizer->Add(ipPortSizer, 0, wxEXPAND | wxALL, 10);
    connectionSizer->Add(connectionButtonSizer, 1, wxEXPAND | wxALL, 10);

    // Authentication section
    wxStaticBoxSizer* authSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Gmail Authentication");
    wxStaticBox* authBox = authSizer->GetStaticBox();
    authBox->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho tiêu đề
    authBox->SetBackgroundColour(wxColour(37, 41, 51));

    wxBoxSizer* authButtonSizer = new wxBoxSizer(wxHORIZONTAL);

    btnAuthenticate = new wxButton(panel, ID_AUTHENTICATE, "Start Authentication", wxDefaultPosition, wxSize(-1, 30));
    btnAuthenticate->SetBackgroundColour(wxColour(63, 81, 181));
    btnAuthenticate->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho button text

    btnSubmitAuth = new wxButton(panel, ID_SUBMIT_AUTH, "Submit Auth Code", wxDefaultPosition, wxSize(-1, 30));
    btnSubmitAuth->SetBackgroundColour(wxColour(63, 81, 181));
    btnSubmitAuth->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho button text
    btnSubmitAuth->Disable();

    authButtonSizer->Add(btnAuthenticate, 1, wxALL, 5);
    authButtonSizer->Add(btnSubmitAuth, 1, wxALL, 5);

    txtAuthCode = new wxTextCtrl(panel, wxID_ANY);
    txtAuthCode->SetBackgroundColour(wxColour(45, 48, 58));
    txtAuthCode->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho text input
    txtAuthCode->SetHint("Enter authentication code here");

    authSizer->Add(authButtonSizer, 0, wxEXPAND | wxALL, 5);
    authSizer->Add(txtAuthCode, 0, wxALL | wxEXPAND, 5);

    // Monitoring section
    wxStaticBoxSizer* monitoringSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Email Monitoring");
    wxStaticBox* monitorBox = monitoringSizer->GetStaticBox();
    monitorBox->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho tiêu đề
    monitorBox->SetBackgroundColour(wxColour(37, 41, 51));

    btnStartMonitoring = new wxButton(panel, ID_START_MONITORING, "Start Monitoring", wxDefaultPosition, wxSize(-1, 30));
    btnStartMonitoring->SetBackgroundColour(wxColour(0, 150, 136));
    btnStartMonitoring->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho button text
    btnStartMonitoring->Disable();
    monitoringSizer->Add(btnStartMonitoring, 0, wxALL | wxEXPAND, 5);

    // Status and Commands sections
    wxBoxSizer* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

    // Status section
    wxStaticBoxSizer* statusSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Status");
    wxStaticBox* statusBox = statusSizer->GetStaticBox();
    statusBox->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho tiêu đề
    statusBox->SetBackgroundColour(wxColour(37, 41, 51));

    txtStatus = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 150),
        wxTE_MULTILINE | wxTE_READONLY);
    txtStatus->SetBackgroundColour(wxColour(45, 48, 58));
    txtStatus->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho text area
    statusSizer->Add(txtStatus, 1, wxALL | wxEXPAND, 5);

    // Commands section
    wxStaticBoxSizer* commandsSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Received Commands");
    wxStaticBox* commandsBox = commandsSizer->GetStaticBox();
    commandsBox->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho tiêu đề
    commandsBox->SetBackgroundColour(wxColour(37, 41, 51));

    txtCommands = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 150),
        wxTE_MULTILINE | wxTE_READONLY);
    txtCommands->SetBackgroundColour(wxColour(45, 48, 58));
    txtCommands->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho text area
    commandsSizer->Add(txtCommands, 1, wxALL | wxEXPAND, 5);

    bottomSizer->Add(statusSizer, 1, wxEXPAND | wxRIGHT, 5);
    bottomSizer->Add(commandsSizer, 1, wxEXPAND | wxLEFT, 5);

    // Direct Command buttons
    wxStaticBoxSizer* directCommandsSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Direct Commands");
    wxStaticBox* directBox = directCommandsSizer->GetStaticBox();
    directBox->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho tiêu đề
    directBox->SetBackgroundColour(wxColour(37, 41, 51));

    wxFlexGridSizer* buttonGrid = new wxFlexGridSizer(3, 2, 8, 8);

    // Function to create styled command buttons
    auto createCommandButton = [&](const wxString& label, wxWindowID id) {
        wxButton* btn = new wxButton(panel, id, label, wxDefaultPosition, wxSize(-1, 30));
        btn->SetBackgroundColour(wxColour(96, 125, 139));
        btn->SetForegroundColour(wxColour(255, 255, 255));  // Trắng cho button text
        return btn;
        };

    wxButton* btnListApp = createCommandButton("List Applications", ID_LIST_APP);
    wxButton* btnListProcess = createCommandButton("List Processes", ID_LIST_PROCESS);
    wxButton* btnListService = createCommandButton("List Services", ID_LIST_SERVICE);
    wxButton* btnScreenshot = createCommandButton("Take Screenshot", ID_SCREENSHOT);
    wxButton* btnOpenCam = createCommandButton("Open Camera", ID_OPEN_CAM);
    wxButton* btnHelp = createCommandButton("Help", ID_HELP);

    buttonGrid->Add(btnListApp, 1, wxEXPAND);
    buttonGrid->Add(btnListProcess, 1, wxEXPAND);
    buttonGrid->Add(btnListService, 1, wxEXPAND);
    buttonGrid->Add(btnScreenshot, 1, wxEXPAND);
    buttonGrid->Add(btnOpenCam, 1, wxEXPAND);
    buttonGrid->Add(btnHelp, 1, wxEXPAND);

    directCommandsSizer->Add(buttonGrid, 0, wxALL | wxEXPAND, 10);

    mainSizer->Add(connectionSizer, 0, wxALL | wxEXPAND, 10);
    mainSizer->Add(authSizer, 0, wxALL | wxEXPAND, 10);
    mainSizer->Add(monitoringSizer, 0, wxALL | wxEXPAND, 10);
    mainSizer->Add(directCommandsSizer, 0, wxALL | wxEXPAND, 10);
    mainSizer->Add(bottomSizer, 1, wxALL | wxEXPAND, 10);

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

void MainFrame::UpdateCommandsList(const std::string& command) {
    wxDateTime now = wxDateTime::Now();
    txtCommands->AppendText(now.FormatTime() + ": " + command + "\n");
}

void MainFrame::ResetApplicationState() {
    // Reset all input fields
    txtIpAddress->SetValue("");
    txtPort->SetValue("");
    if (txtAuthCode) {
        txtAuthCode->SetValue("");
    }

    UpdateConnectionStatus();

    // Reset all buttons to initial state
    btnConnect->Enable();
    btnAuthenticate->Disable();
    btnStartMonitoring->Disable();
    if (btnSubmitAuth) {
        btnSubmitAuth->Disable();
    }

    wxWindow* btnDisconnect = FindWindow(ID_DISCONNECT);
    if (btnDisconnect) btnDisconnect->Disable();

    // Reset monitoring state
    if (isMonitoring) {
        isMonitoring = false;
        if (checkEmailTimer->IsRunning()) {
            checkEmailTimer->Stop();
        }
    }

    // Reset connection status label
    lblConnectionStatus->SetLabel("Status: OFF");

    // Reset OAuth tokens
    accessToken.clear();
    refreshToken.clear();

    // Reset email handler if exists
    if (emailHandler) {
        delete emailHandler;
        emailHandler = nullptr;
    }

    // Reset oauth object
    if (oauth) {
        delete oauth; // Giải phóng bộ nhớ nếu đã có
        oauth = nullptr; // Đặt lại về nullptr
    }

    // Optional: Clear command list
    if (txtCommands) {
        txtCommands->Clear();
    }

    // Keep the status log by not clearing txtStatus
}

void MainFrame::OnConnect(wxCommandEvent& event) {
    wxString ipAddress = txtIpAddress->GetValue();
    long port;
    if (!txtPort->GetValue().ToLong(&port)) {
        UpdateStatus("Error: Invalid port number");
        return;
    }

    // Kiểm tra nếu đã kết nối thì return
    if (socketClient->isConnected()) {
        UpdateStatus("Already connected to server");
        return;
    }

    if (socketClient->connect(ipAddress.ToStdString(), static_cast<int>(port))) {
        UpdateStatus("Successfully connected to server");
        UpdateConnectionStatus(); // Cập nhật UI để show trạng thái connected
        btnAuthenticate->Enable();
    }
    else {
        UpdateStatus("Failed to connect to server");
        ResetApplicationState(); // Reset UI nếu kết nối thất bại
    }
}

void MainFrame::OnDisconnect(wxCommandEvent& event) {
    // Kiểm tra xem có đang kết nối không
    if (!socketClient->isConnected()) {
        UpdateStatus("Not connected to any server");
        ResetApplicationState();
        return;
    }

    if (socketClient->disconnect()) {
        UpdateStatus("Disconnected from server");
        ResetApplicationState(); // Reset UI sau khi disconnect thành công
    }
    else {
        UpdateStatus("Failed to disconnect from server");
        // Có thể thêm logic retry disconnect ở đây nếu cần
    }
}

// Và sửa lại hàm UpdateConnectionStatus để sử dụng isConnected
void MainFrame::UpdateConnectionStatus() {
    if (socketClient && socketClient->isConnected()) {
        lblConnectionStatus->SetLabel("Status: ON");
        lblConnectionStatus->SetForegroundColour(*wxGREEN);
        btnConnect->Disable();
        wxWindow* btnDisconnect = FindWindow(ID_DISCONNECT);
        if (btnDisconnect) btnDisconnect->Enable();
    }
    else {
        lblConnectionStatus->SetLabel("Status: OFF");
        lblConnectionStatus->SetForegroundColour(wxColour(255, 99, 71));
        btnConnect->Enable();
        wxWindow* btnDisconnect = FindWindow(ID_DISCONNECT);
        if (btnDisconnect) btnDisconnect->Disable();
    }
    lblConnectionStatus->Refresh();
    lblConnectionStatus->Update();
}

void MainFrame::OnAuthenticate(wxCommandEvent& event) {
    // Khởi tạo lại đối tượng oauth mỗi khi xác thực
    if (oauth == nullptr) {
        oauth = new GoogleOAuth(clientId, clientSecret, REDIRECT_URI);
    }

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
            filename = (emailInfo.content == "list app") ? "applications.txt" : "services.txt";
            socketClient->receiveAndSaveFile(filename);
        }
        else if (emailInfo.content == "list process" || emailInfo.content == "help") {
            filename = (emailInfo.content == "list process") ? "processes.txt" : "help.txt";
            socketClient->receiveAndSaveFile(filename);
        }
        else if (emailInfo.content == "screenshot" || emailInfo.content == "open_cam") {
            filename = (emailInfo.content == "screenshot") ? "screenshot.png" : "webcam.png";
            socketClient->receiveAndSaveImage(filename);
        }
        else if (emailInfo.content.substr(0, 4) == "view") {
            filename = "received_file.txt";
            socketClient->receiveAndSaveFile(filename);
        }

        // Lấy thư mục hiện tại nơi ứng dụng đang chạy
        wxString currentDir = wxGetCwd(); // Lấy working directory hiện tại
        wxString filePath = wxFileName(currentDir, filename).GetFullPath();
        std::string path_command = "save_path:" + filePath.ToStdString();
        socketClient->sendData(path_command.c_str(), path_command.length());

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

        wxString filePath = saveFileDialog.GetPath();
        // Lưu file với tên và đường dẫn đã chọn
        socketClient->receiveAndSaveFile(saveFileDialog.GetPath().ToStdString());
        UpdateStatus("Applications list saved to " + saveFileDialog.GetPath());

        // Gửi đường dẫn đến file cho server
        std::string path_command = "save_path:" + filePath.ToStdString();
        socketClient->sendData(path_command.c_str(), path_command.length());

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

        wxString filePath = saveFileDialog.GetPath();
        socketClient->receiveAndSaveFile(saveFileDialog.GetPath().ToStdString());
        UpdateStatus("Process list saved to " + saveFileDialog.GetPath());

        // Gửi đường dẫn đến file cho server
        std::string path_command = "save_path:" + filePath.ToStdString();
        socketClient->sendData(path_command.c_str(), path_command.length());

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

        wxString filePath = saveFileDialog.GetPath();
        socketClient->receiveAndSaveFile(saveFileDialog.GetPath().ToStdString());
        UpdateStatus("Services list saved to " + saveFileDialog.GetPath());

        // Gửi đường dẫn đến file cho server
        std::string path_command = "save_path:" + filePath.ToStdString();
        socketClient->sendData(path_command.c_str(), path_command.length());

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

        wxString filePath = saveFileDialog.GetPath();
        socketClient->receiveAndSaveImage(saveFileDialog.GetPath().ToStdString());
        UpdateStatus("Screenshot saved to " + saveFileDialog.GetPath());

        // Gửi đường dẫn đến file cho server
        std::string path_command = "save_path:" + filePath.ToStdString();
        socketClient->sendData(path_command.c_str(), path_command.length());

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
        wxString filePath = saveFileDialog.GetPath();
        socketClient->receiveAndSaveImage(saveFileDialog.GetPath().ToStdString());
        UpdateStatus("Webcam image saved to " + saveFileDialog.GetPath());

        // Gửi đường dẫn đến file cho server
        std::string path_command = "save_path:" + filePath.ToStdString();
        socketClient->sendData(path_command.c_str(), path_command.length());

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

        wxString filePath = saveFileDialog.GetPath();
        socketClient->receiveAndSaveFile(saveFileDialog.GetPath().ToStdString());
        UpdateStatus("Help information saved to " + saveFileDialog.GetPath());

        // Gửi đường dẫn đến file cho server
        std::string path_command = "save_path:" + filePath.ToStdString();
        socketClient->sendData(path_command.c_str(), path_command.length());

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