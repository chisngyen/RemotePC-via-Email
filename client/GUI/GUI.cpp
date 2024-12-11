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
EVT_BUTTON(ID_START_MONITORING, MainFrame::OnStartMonitoring)
EVT_TIMER(ID_CHECK_EMAIL_TIMER, MainFrame::OnCheckEmail)
EVT_BUTTON(ID_LIST_APP, MainFrame::OnListApp)
EVT_BUTTON(ID_LIST_PROCESS, MainFrame::OnListProcess)
EVT_BUTTON(ID_LIST_SERVICE, MainFrame::OnListService)
EVT_BUTTON(ID_SCREENSHOT, MainFrame::OnScreenshot)
EVT_BUTTON(ID_OPEN_CAM, MainFrame::OnOpenCam)
EVT_BUTTON(ID_HELP, MainFrame::OnHelp)
EVT_BUTTON(ID_SHUTDOWN, MainFrame::OnShutdown)
EVT_BUTTON(ID_RESTART, MainFrame::OnRestart)
EVT_BUTTON(ID_LOCKSCREEN, MainFrame::OnLockScreen)
EVT_BUTTON(ID_TOGGLE_APP, MainFrame::OnToggleApp)
END_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1200, 1000))
{
    // Initialize members
    socketClient = new SocketClient();
    oauth = nullptr;
    emailHandler = nullptr;
    isMonitoring = false;
    checkEmailTimer = new wxTimer(this, ID_CHECK_EMAIL_TIMER);
    gmailAutomation = nullptr;
    callbackServer = nullptr;
    currentProcessId = 0;
    isAppRunning = false;

    isCameraOpen = false;

    // Create main panel with modern dark theme
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(wxColour(18, 18, 18));

    // Set modern font
    wxFont defaultFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    panel->SetFont(defaultFont);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Helper function to create modern sections
    auto createSection = [](wxPanel* parent, const wxString& title) -> wxPanel* {
        wxPanel* section = new wxPanel(parent);
        section->SetBackgroundColour(wxColour(30, 30, 30));

        wxBoxSizer* sectionSizer = new wxBoxSizer(wxVERTICAL);

        wxStaticText* headerText = new wxStaticText(section, wxID_ANY, title);
        headerText->SetForegroundColour(wxColour(200, 200, 200));
        wxFont headerFont = headerText->GetFont();
        headerFont.SetPointSize(11);
        headerFont.SetWeight(wxFONTWEIGHT_BOLD);
        headerText->SetFont(headerFont);

        sectionSizer->Add(headerText, 0, wxALL, 15);
        section->SetSizer(sectionSizer);

        return section;
        };

    // Helper function to create modern buttons
    auto createModernButton = [this](wxWindow* parent, wxWindowID id,
        const wxString& label, const wxColour& baseColor) -> wxButton* {
            wxButton* btn = new wxButton(parent, id, label,
                wxDefaultPosition, wxSize(-1, 40));

            wxColour darkColor = baseColor.ChangeLightness(85);
            btn->SetBackgroundColour(darkColor);
            btn->SetForegroundColour(wxColour(255, 255, 255));

            wxFont btnFont = btn->GetFont();
            btnFont.SetPointSize(11);
            btnFont.SetWeight(wxFONTWEIGHT_MEDIUM);
            btn->SetFont(btnFont);

            btn->Bind(wxEVT_ENTER_WINDOW, &MainFrame::OnButtonHover, this);
            btn->Bind(wxEVT_LEAVE_WINDOW, &MainFrame::OnButtonLeave, this);

            return btn;
        };

    // Connection Section
    wxPanel* connectionPanel = createSection(panel, "Server Connection");
    wxBoxSizer* connectionSizer = new wxBoxSizer(wxVERTICAL);

    // Input fields with better spacing
    wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);

    // IP Address input
    wxPanel* ipContainer = new wxPanel(connectionPanel);
    ipContainer->SetBackgroundColour(wxColour(45, 45, 45));
    wxBoxSizer* ipSizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText* ipLabel = new wxStaticText(ipContainer, wxID_ANY, "IP Address:");
    ipLabel->SetForegroundColour(wxColour(180, 180, 180));
    txtIpAddress = new wxTextCtrl(ipContainer, wxID_ANY, "",
        wxPoint(-1, 1), wxDefaultSize, wxBORDER_NONE | wxTE_PROCESS_ENTER);
    txtIpAddress->SetBackgroundColour(wxColour(45, 45, 45));
    txtIpAddress->SetForegroundColour(wxColour(255, 255, 255));

    ipSizer->Add(ipLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 10);
    ipSizer->Add(txtIpAddress, 1, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    ipContainer->SetSizer(ipSizer);

    // Port input
    wxPanel* portContainer = new wxPanel(connectionPanel);
    portContainer->SetBackgroundColour(wxColour(45, 45, 45));
    wxBoxSizer* portSizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText* portLabel = new wxStaticText(portContainer, wxID_ANY, "Port:");
    portLabel->SetForegroundColour(wxColour(180, 180, 180));
    txtPort = new wxTextCtrl(portContainer, wxID_ANY, "",
        wxPoint(-1, 1), wxDefaultSize, wxBORDER_NONE | wxTE_PROCESS_ENTER);
    txtPort->SetBackgroundColour(wxColour(45, 45, 45));
    txtPort->SetForegroundColour(wxColour(255, 255, 255));

    portSizer->Add(portLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 10);
    portSizer->Add(txtPort, 1, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    portContainer->SetSizer(portSizer);

    inputSizer->Add(ipContainer, 0, wxEXPAND | wxRIGHT, 15);
    inputSizer->Add(portContainer, 0, wxEXPAND);

    // Connection buttons with enhanced styling
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    btnConnect = createModernButton(connectionPanel, ID_CONNECT,
        "Connect", wxColour(0, 120, 215));
    wxButton* btnDisconnect = createModernButton(connectionPanel, ID_DISCONNECT,
        "Disconnect", wxColour(215, 0, 64));
    btnDisconnect->Disable();
    btnDisconnect->SetBackgroundColour(wxColour(231, 76, 60).ChangeLightness(60));

    lblConnectionStatus = new wxStaticText(connectionPanel, wxID_ANY, "Status: Disconnected");
    lblConnectionStatus->SetForegroundColour(wxColour(215, 0, 64));
    wxFont statusFont = lblConnectionStatus->GetFont();
    statusFont.SetPointSize(10);
    lblConnectionStatus->SetFont(statusFont);

    buttonSizer->Add(btnConnect, 0, wxALL, 8);
    buttonSizer->Add(btnDisconnect, 0, wxALL, 8);
    buttonSizer->AddSpacer(20);
    buttonSizer->Add(lblConnectionStatus, 0, wxALIGN_CENTER_VERTICAL | wxALL, 8);

    wxBoxSizer* connectionContentSizer = dynamic_cast<wxBoxSizer*>(connectionPanel->GetSizer());
    connectionContentSizer->Add(inputSizer, 0, wxEXPAND | wxALL, 15);
    connectionContentSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 15);

    // Monitoring Section
    wxPanel* monitoringPanel = createSection(panel, "Email Monitoring");
    btnStartMonitoring = createModernButton(monitoringPanel, ID_START_MONITORING,
        "Start Monitoring", wxColour(0, 150, 136));
    btnStartMonitoring->Disable();

    wxBoxSizer* monitoringContentSizer = dynamic_cast<wxBoxSizer*>(monitoringPanel->GetSizer());
    monitoringContentSizer->Add(btnStartMonitoring, 0, wxEXPAND | wxALL, 15);


    //Direct Section
    wxPanel* commandPanel = createSection(panel, "Direct Commands");
    commandPanel->SetMinSize(wxSize(-1, 180));
    wxBoxSizer* commandContentSizer = dynamic_cast<wxBoxSizer*>(commandPanel->GetSizer());

    // Create scrolled window for commands
    wxScrolledWindow* commandScroll = new wxScrolledWindow(commandPanel);
    commandScroll->SetBackgroundColour(wxColour(30, 30, 30));
    commandScroll->SetScrollRate(0, 5);

    wxBoxSizer* scrollSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* leftColumnSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* rightColumnSizer = new wxBoxSizer(wxVERTICAL);

    // Helper function to create command group panels
    auto createCommandGroupPanel = [this](wxWindow* parent, const wxString& title,
        const wxColour& headerColor) -> CommandGroupPanels {
            wxPanel* group = new wxPanel(parent);
            group->SetBackgroundColour(wxColour(40, 44, 52));

            wxBoxSizer* groupSizer = new wxBoxSizer(wxVERTICAL);

            // Header panel với nút mũi tên
            wxPanel* header = new wxPanel(group);
            header->SetBackgroundColour(headerColor);
            wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);

            // Icon và tiêu đề như cũ
            wxStaticText* icon = new wxStaticText(header, wxID_ANY,
                title == "System Info" ? "\u2699" :      // ⚙
                title == "Monitoring" ? "\u25A0" :       // ■
                title == "System Control" ? "\u2630" :   // ☰
                "\u2692");
            icon->SetForegroundColour(wxColour(255, 255, 255));

            wxFont iconFont = icon->GetFont();
            iconFont.SetFamily(wxFONTFAMILY_DEFAULT);
            iconFont.SetStyle(wxFONTSTYLE_NORMAL);
            iconFont.SetWeight(wxFONTWEIGHT_BOLD);
            iconFont.SetPointSize(10); // Tùy chỉnh kích thước font
            icon->SetFont(iconFont);

            wxStaticText* headerText = new wxStaticText(header, wxID_ANY, title);
            headerText->SetForegroundColour(wxColour(255, 255, 255));

            // Thêm nút mũi tên
            wxButton* toggleBtn = new wxButton(header, wxID_ANY, "\u25BC",
                wxDefaultPosition, wxSize(20, 20), wxBU_EXACTFIT);

            headerSizer->Add(icon, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
            headerSizer->Add(headerText, 1, wxALL | wxALIGN_CENTER_VERTICAL, 10);
            headerSizer->Add(toggleBtn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
            header->SetSizer(headerSizer);

            // Content panel (khởi tạo ẩn)
            wxPanel* content = new wxPanel(group);
            content->SetBackgroundColour(wxColour(45, 49, 58));
            content->Show(false);
            wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);
            content->SetSizer(contentSizer);

            groupSizer->Add(header, 0, wxEXPAND);
            groupSizer->Add(content, 1, wxEXPAND);
            group->SetSizer(groupSizer);

            // Xử lý sự kiện click vào nút mũi tên
            toggleBtn->Bind(wxEVT_BUTTON, [content, toggleBtn](wxCommandEvent&) {
                content->Show(!content->IsShown());
                toggleBtn->SetLabel(content->IsShown() ? "\u25BC" : "\u25BA");
                content->GetParent()->Layout();
                // Refresh scroll window nếu cần
                wxScrolledWindow* scrollWin =
                    wxDynamicCast(content->GetParent()->GetParent(), wxScrolledWindow);
                if (scrollWin) {
                    scrollWin->FitInside();
                }
                });

            return { group, content };
        };

    // Helper function to create styled command buttons
    auto createStyledCommandButton = [](wxWindow* parent, const wxString& label,
        wxWindowID id, const wxColour& baseColor) -> wxButton* {
            wxButton* btn = new wxButton(parent, id, label,
                wxDefaultPosition, wxSize(-1, 45)); // Increase button height

            // Make buttons more visible
            btn->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
            btn->SetForegroundColour(wxColour(255, 255, 255));
            btn->SetBackgroundColour(baseColor);

            return btn;
        };

    // System Info Group
    auto sysInfoPanels = createCommandGroupPanel(commandScroll, "System Info",
        wxColour(52, 152, 219));
    wxBoxSizer* sysInfoContentSizer = dynamic_cast<wxBoxSizer*>(sysInfoPanels.contentPanel->GetSizer());

    auto btnListApp = createStyledCommandButton(sysInfoPanels.contentPanel,
        "Applications List", ID_LIST_APP, wxColour(41, 128, 185));
    auto btnListProc = createStyledCommandButton(sysInfoPanels.contentPanel,
        "Process List", ID_LIST_PROCESS, wxColour(41, 128, 185));
    auto btnListServ = createStyledCommandButton(sysInfoPanels.contentPanel,
        "Services List", ID_LIST_SERVICE, wxColour(41, 128, 185));

    sysInfoContentSizer->Add(btnListApp, 0, wxEXPAND | wxALL, 8);  
    sysInfoContentSizer->Add(btnListProc, 0, wxEXPAND | wxALL, 8);
    sysInfoContentSizer->Add(btnListServ, 0, wxEXPAND | wxALL, 8);


    // Monitoring Group
    auto monitorPanels = createCommandGroupPanel(commandScroll, "Monitoring",
        wxColour(46, 204, 113));
    wxBoxSizer* monitorContentSizer = dynamic_cast<wxBoxSizer*>(monitorPanels.contentPanel->GetSizer());

    auto btnScreenshot = createStyledCommandButton(monitorPanels.contentPanel,
        "Take Screenshot", ID_SCREENSHOT, wxColour(39, 174, 96));
    auto btnCamera = createStyledCommandButton(monitorPanels.contentPanel,
        "Camera Control", ID_OPEN_CAM, wxColour(39, 174, 96));

    monitorContentSizer->Add(btnScreenshot, 0, wxEXPAND | wxALL, 5);
    monitorContentSizer->Add(btnCamera, 0, wxEXPAND | wxALL, 5);

    // System Control Group
    auto sysCtrlPanels = createCommandGroupPanel(commandScroll, "System Control",
        wxColour(231, 76, 60));
    wxBoxSizer* sysCtrlContentSizer = dynamic_cast<wxBoxSizer*>(sysCtrlPanels.contentPanel->GetSizer());

    auto btnShutdown = createStyledCommandButton(sysCtrlPanels.contentPanel,
        "Shutdown", ID_SHUTDOWN, wxColour(192, 57, 43));
    auto btnRestart = createStyledCommandButton(sysCtrlPanels.contentPanel,
        "Restart", ID_RESTART, wxColour(192, 57, 43));
    auto btnLock = createStyledCommandButton(sysCtrlPanels.contentPanel,
        "Lock Screen", ID_LOCKSCREEN, wxColour(192, 57, 43));

    sysCtrlContentSizer->Add(btnShutdown, 0, wxEXPAND | wxALL, 5);
    sysCtrlContentSizer->Add(btnRestart, 0, wxEXPAND | wxALL, 5);
    sysCtrlContentSizer->Add(btnLock, 0, wxEXPAND | wxALL, 5);

    // Utilities Group
    auto utilsPanels = createCommandGroupPanel(commandScroll, "Utilities",
        wxColour(155, 89, 182));
    wxBoxSizer* utilsContentSizer = dynamic_cast<wxBoxSizer*>(utilsPanels.contentPanel->GetSizer());

    auto btnStartApp = createStyledCommandButton(utilsPanels.contentPanel,
        "Application Control", ID_TOGGLE_APP, wxColour(142, 68, 173));
    auto btnHelp = createStyledCommandButton(utilsPanels.contentPanel,
        "Help", ID_HELP, wxColour(142, 68, 173));

    utilsContentSizer->Add(btnStartApp, 0, wxEXPAND | wxALL, 5);
    utilsContentSizer->Add(btnHelp, 0, wxEXPAND | wxALL, 5);

    // Add groups to columns
    leftColumnSizer->Add(sysInfoPanels.groupPanel, 0, wxEXPAND | wxALL, 5);
    leftColumnSizer->Add(monitorPanels.groupPanel, 0, wxEXPAND | wxALL, 5);

    rightColumnSizer->Add(sysCtrlPanels.groupPanel, 0, wxEXPAND | wxALL, 5);
    rightColumnSizer->Add(utilsPanels.groupPanel, 0, wxEXPAND | wxALL, 5);

    // Add columns to scroll sizer
    scrollSizer->Add(leftColumnSizer, 1, wxEXPAND | wxALL, 5);
    scrollSizer->Add(rightColumnSizer, 1, wxEXPAND | wxALL, 5);

    commandScroll->SetSizer(scrollSizer);
    commandContentSizer->Add(commandScroll, 1, wxEXPAND | wxALL, 10);

    // Status and Commands sections
    wxBoxSizer* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

    // Helper function for creating modern panels
    auto createTextArea = [](wxPanel* parent, const wxString& title, bool isCommands = false) -> std::pair<wxPanel*, wxWindow*> {
        wxPanel* container = new wxPanel(parent);
        container->SetBackgroundColour(wxColour(30, 30, 30));

        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

        wxPanel* headerPanel = new wxPanel(container);
        headerPanel->SetBackgroundColour(wxColour(45, 45, 45));

        wxStaticText* label = new wxStaticText(headerPanel, wxID_ANY, title);
        label->SetForegroundColour(wxColour(220, 220, 220));
        wxFont labelFont = label->GetFont();
        labelFont.SetPointSize(11);
        labelFont.SetWeight(wxFONTWEIGHT_BOLD);
        label->SetFont(labelFont);

        wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);
        headerSizer->Add(label, 0, wxALL | wxALIGN_CENTER_VERTICAL, 12);
        headerPanel->SetSizer(headerSizer);

        wxWindow* content;
        wxFont contentFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

        if (isCommands) {
            wxScrolledWindow* scrolled = new wxScrolledWindow(container);
            scrolled->SetScrollRate(0, 20);
            scrolled->SetBackgroundColour(wxColour(35, 35, 35));
            scrolled->SetFont(contentFont);
            scrolled->SetSizer(new wxBoxSizer(wxVERTICAL));
            content = scrolled;
        }
        else {
            wxTextCtrl* textCtrl = new wxTextCtrl(container, wxID_ANY, "",
                wxDefaultPosition, wxSize(-1, 200),
                wxTE_MULTILINE | wxTE_RICH2 | wxTE_READONLY | wxVSCROLL | wxALWAYS_SHOW_SB | wxBORDER_NONE);
            textCtrl->SetBackgroundColour(wxColour(35, 35, 35));
            textCtrl->SetForegroundColour(wxColour(220, 220, 220));
            textCtrl->SetFont(contentFont);
            content = textCtrl;
        }

        sizer->Add(headerPanel, 0, wxEXPAND | wxALL, 1);
        sizer->Add(content, 1, wxEXPAND | wxALL, 8);
        container->SetSizer(sizer);

        return std::make_pair(container, content);
        };

    // Sử dụng:
    auto statusResult = createTextArea(panel, "System Status");
    wxPanel* statusContainer = statusResult.first;
    txtStatus = dynamic_cast<wxTextCtrl*>(statusResult.second);

    auto commandsResult = createTextArea(panel, "Received Commands", true);
    wxPanel* commandsContainer = commandsResult.first;
    commandsScroll = dynamic_cast<wxScrolledWindow*>(commandsResult.second);
    commandsSizer = dynamic_cast<wxBoxSizer*>(commandsScroll->GetSizer());

    bottomSizer->Add(statusContainer, 1, wxEXPAND | wxRIGHT, 8);
    bottomSizer->Add(commandsContainer, 1, wxEXPAND | wxLEFT, 8);

    // Add all sections to main sizer with proper spacing
    mainSizer->Add(connectionPanel, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(monitoringPanel, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(commandPanel, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(bottomSizer, 1, wxEXPAND | wxALL, 10);

    panel->SetSizer(mainSizer);

    // Load client secrets
    LoadClientSecrets();

    Centre();
}

void MainFrame::OnButtonHover(wxMouseEvent& event)
{
    wxButton* button = static_cast<wxButton*>(event.GetEventObject());
    button->SetBackgroundColour(button->GetBackgroundColour());
    button->Refresh();
}

void MainFrame::OnButtonLeave(wxMouseEvent& event)
{
    wxButton* button = static_cast<wxButton*>(event.GetEventObject());
    button->SetBackgroundColour(button->GetBackgroundColour());
    button->Refresh();
}

MainFrame::~MainFrame() {
    Unbind(wxEVT_OAUTH_CODE, &MainFrame::OnOAuthCode, this);
    delete socketClient;
    delete oauth;
    delete gmailAutomation;
    delete emailHandler;
    delete checkEmailTimer;
    delete callbackServer;

}

void MainFrame::UpdateCommandsList(const wxString& command, const EmailHandler::EmailInfo& emailInfo) {
    wxPanel* commandEntry = new wxPanel(commandsScroll);
    commandEntry->SetBackgroundColour(wxColour(40, 44, 52));

    wxBoxSizer* entrySizer = new wxBoxSizer(wxVERTICAL);

    // Helper function for creating rows
    auto createRow = [commandEntry](const wxString& label, const wxString& value,
        const wxColour& labelColor = wxColour(150, 150, 150),
        const wxColour& valueColor = wxColour(220, 220, 220)) {
            wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);

            wxStaticText* labelText = new wxStaticText(commandEntry, wxID_ANY,
                label, wxDefaultPosition, wxSize(80, -1));
            labelText->SetForegroundColour(labelColor);

            wxStaticText* valueText = new wxStaticText(commandEntry, wxID_ANY, value);
            valueText->SetForegroundColour(valueColor);

            rowSizer->Add(labelText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
            rowSizer->Add(valueText, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

            return rowSizer;
        };

    // Add command info rows
    wxDateTime now = wxDateTime::Now();
    entrySizer->Add(createRow("Time:", now.FormatTime()), 0, wxEXPAND | wxTOP, 8);
    entrySizer->Add(createRow("From:", emailInfo.from), 0, wxEXPAND | wxTOP, 4);
    entrySizer->Add(createRow("Subject:", emailInfo.subject), 0, wxEXPAND | wxTOP, 4);
    entrySizer->Add(createRow("Command:", command), 0, wxEXPAND | wxTOP, 4);

    // Add status with color based on command validity
    wxColour statusColor = command.StartsWith("[ERROR]") ?
        wxColour(239, 68, 68) : wxColour(34, 197, 94);
    wxString status = command.StartsWith("[ERROR]") ? "Error" : "Success";
    entrySizer->Add(createRow("Status:", status, wxColour(150, 150, 150), statusColor),
        0, wxEXPAND | wxTOP | wxBOTTOM, 4);

    // Add separator
    wxPanel* separator = new wxPanel(commandEntry, wxID_ANY,
        wxDefaultPosition, wxSize(-1, 1));
    separator->SetBackgroundColour(wxColour(75, 85, 99));
    entrySizer->Add(separator, 0, wxEXPAND | wxTOP, 4);

    commandEntry->SetSizer(entrySizer);
    commandEntry->Fit();

    // Add to scroll window
    commandsSizer->Add(commandEntry, 0, wxEXPAND | wxALL, 5);
    commandsScroll->FitInside();
    commandsScroll->Scroll(0, commandsSizer->GetSize().GetHeight());
    commandsScroll->Layout();
    commandsScroll->Refresh();
}

void MainFrame::ResetApplicationState() {
    // Reset all input fields
    txtIpAddress->SetValue("");
    txtPort->SetValue("");

    UpdateConnectionStatus();

    // Reset all buttons to initial state
    btnConnect->Enable();
    //btnAuthenticate->Disable();
    btnStartMonitoring->Enable();
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
    lblConnectionStatus->SetLabel("Status: Disconnected");

    isCameraOpen = false;
    wxButton* camButton = dynamic_cast<wxButton*>(FindWindow(ID_OPEN_CAM));
    if (camButton) {
        camButton->SetLabel("Open Camera");
    }

    isAppRunning = false;
    currentProcessId = 0;
    currentAppName.Clear();
    wxButton* toggleButton = dynamic_cast<wxButton*>(FindWindow(ID_TOGGLE_APP));
    if (toggleButton) {
        toggleButton->SetLabel("Start App");
    }

    // Reset OAuth tokens
    accessToken.clear();
    refreshToken.clear();

    if (callbackServer) {
        callbackServer->stop();
    }

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
        //btnAuthenticate->Enable();
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
        lblConnectionStatus->SetLabel("Status: Connected");
        lblConnectionStatus->SetForegroundColour(*wxGREEN);
        btnConnect->Disable();
        wxWindow* btnDisconnect = FindWindow(ID_DISCONNECT);
        if (btnDisconnect) btnDisconnect->Enable();
    }
    else {
        lblConnectionStatus->SetLabel("Status: Disconnected");
        lblConnectionStatus->SetForegroundColour(wxColour(255, 99, 71));
        btnConnect->Enable();
        wxWindow* btnDisconnect = FindWindow(ID_DISCONNECT);
        if (btnDisconnect) btnDisconnect->Disable();
    }
    lblConnectionStatus->Refresh();
    lblConnectionStatus->Update();
}

void MainFrame::OnAuthenticate(wxCommandEvent& event) {
    if (oauth == nullptr) {
        oauth = new GoogleOAuth(clientId, clientSecret, "http://localhost:8080");
    }

    // Khởi tạo callback server nếu chưa có
    if (callbackServer == nullptr) {
        callbackServer = new OAuthCallbackServer(this, 8080);
        Bind(wxEVT_OAUTH_CODE, &MainFrame::OnOAuthCode, this);
    }

    if (!callbackServer->startListening()) {
        UpdateStatus("Failed to start callback server");
        return;
    }

    std::string authUrl = oauth->getAuthUrl();
    if ((size_t)ShellExecuteA(NULL, "open", authUrl.c_str(), NULL, NULL, SW_SHOWNORMAL) > 32) {
        UpdateStatus("Browser opened for authentication. Automating Gmail auth...");
        btnAuthenticate->Disable();

        // Chạy automation trong thread riêng
        std::thread([this]() {
            // Khởi tạo automation
            if (gmailAutomation == nullptr) {
                gmailAutomation = new GmailUIAutomation();  
            }

            // Thực hiện automation
            bool success = gmailAutomation->automateGmailAuth(userGmail.ToStdString());

            // Sử dụng CallAfter để update UI từ worker thread
            CallAfter([this, success]() {
                if (success) {
                    UpdateStatus("Gmail authentication automated successfully");
                }
                else {
                    UpdateStatus("Failed to automate Gmail authentication. Please proceed manually.");
                }
                });

            }).detach();

    }
    else {
        UpdateStatus("Failed to open browser");
        callbackServer->stop();
    }
}

void MainFrame::AutoStartAuthentication() {
    if (oauth == nullptr) {
        oauth = new GoogleOAuth(clientId, clientSecret, "http://localhost:8080");
    }

    if (callbackServer == nullptr) {
        callbackServer = new OAuthCallbackServer(this, 8080);
        Bind(wxEVT_OAUTH_CODE, &MainFrame::OnOAuthCode, this);
    }

    if (!callbackServer->startListening()) {
        UpdateStatus("Failed to start callback server");
        return;
    }

    std::string authUrl = oauth->getAuthUrl();
    if ((size_t)ShellExecuteA(NULL, "open", authUrl.c_str(), NULL, NULL, SW_SHOWNORMAL) > 32) {
        UpdateStatus("Starting automatic authentication...");

        // Chạy automation trong thread riêng
        std::thread([this]() {
            if (gmailAutomation == nullptr) {
                gmailAutomation = new GmailUIAutomation();
            }

            bool success = gmailAutomation->automateGmailAuth(userGmail.ToStdString());

            CallAfter([this, success]() {
                if (success) {
                    UpdateStatus("Authentication completed successfully");
                }
                else {
                    UpdateStatus("Authentication failed. Please check your connection");
                }
                });
            }).detach();
    }
    else {
        UpdateStatus("Failed to start authentication process");
        callbackServer->stop();
    }
}

void MainFrame::ShowWithModernEffect() {
    wxPoint originalPos = GetPosition();
    wxSize originalSize = GetSize();

    SetSize(wxGetDisplaySize().GetWidth() / 2, wxGetDisplaySize().GetHeight() / 2);
    Centre();
    SetTransparent(0);
    Show();

    double scale = 0.5;
    int alpha = 0;
    while (scale < 1.0 || alpha < 255) {
        if (scale < 1.0) {
            scale += 0.0035;
            int newWidth = originalSize.GetWidth() * scale;
            int newHeight = originalSize.GetHeight() * scale;
            SetSize(newWidth, newHeight);
            Centre(); 
        }

        if (alpha < 255) {
            alpha += 4;
            SetTransparent(std::min(alpha, 255));
        }

        wxMilliSleep(1);
        Update();
    }

    SetSize(originalSize);
    SetPosition(originalPos);
    SetTransparent(255);

    Raise();
}

void MainFrame::OnOAuthCode(wxCommandEvent& event) {
    wxString code = event.GetString();

    if (code == "SERVER_ERROR") {
        UpdateStatus("Unable to start callback server");
        return;
    }

    refreshToken = oauth->getRefreshToken(code.ToStdString());
    if (refreshToken.empty()) {
        UpdateStatus("Unable to get refresh token");
        return;
    }

    accessToken = oauth->getAccessToken(refreshToken);
    if (accessToken.empty()) {
        UpdateStatus("Unable to get access token");
        return;
    }

    emailHandler = new EmailHandler(accessToken);
    UpdateStatus("Authentication successful");
    btnStartMonitoring->Enable();

    // Hiện MainFrame và đóng LoginFrame
    ShowWithModernEffect();
    
    if (parentFrame) {
        parentFrame->Close();
    }

}

void MainFrame::OnCheckEmail(wxTimerEvent& event) {
    if (!emailHandler) return;

    EmailHandler::EmailInfo emailInfo = emailHandler->readNewestEmail();
    if (!emailInfo.content.empty() && emailInfo.subject == "Mail Control") {
        emailInfo.content = trim(emailInfo.content);

        // Tách command và IP bằng dấu "-"
        size_t dashPos = emailInfo.content.find(" - ");
        if (dashPos == std::string::npos) {
            UpdateStatus("Invalid email format. Expected: command - IP");
            return;
        }

        std::string command = trim(emailInfo.content.substr(0, dashPos));
        std::string ip = trim(emailInfo.content.substr(dashPos + 3)); // +3 để bỏ qua " - "

        UpdateStatus("Received new email - Command: " + command + ", IP: " + ip);
        

        // Kết nối tới server với IP từ email
        if (!socketClient->connect(ip, 27015)) {
            UpdateStatus("Failed to connect to server: " + ip);
            return;
        }
        UpdateStatus("Connected to server: " + ip);

        // Kiểm tra tính hợp lệ của command
        if (command != "list::app" &&
            command != "list::service" &&
            command != "list::process" &&
            command != "help::cmd" &&
            command != "screenshot::capture" &&
            command != "camera::open" &&
            command != "camera::close" &&
            command.substr(0, 5) != "view::" &&
            command.substr(0, 10) != "app::start" &&
            command.substr(0, 9) != "app::stop" &&
            command.substr(0, 9) != "file::get" &&
            command.substr(0, 12) != "file::delete") {

            std::string errorCommand = "error::Invalid command: " + command;

            // Gửi thông báo lỗi đến server
            if (socketClient->sendData(errorCommand.c_str(), errorCommand.length()) == SOCKET_ERROR) {
                UpdateStatus("Failed to send error message to server");
            }
            else {
                UpdateStatus("Invalid command notification sent to server");
            }

            UpdateCommandsList("[ERROR] '" + command +"'", emailInfo);
            socketClient->disconnect();
            return;
        }

        // Gửi command tới server
        if (socketClient->sendData(command.c_str(), command.length()) == SOCKET_ERROR) {
            UpdateStatus("Failed to send command to server");
            socketClient->disconnect();
            return;
        }

        UpdateCommandsList(command, emailInfo);
        std::string filename = "";
        if (command == "list::app" || command == "list::service") {
            filename = (command == "list::app") ? "applications.txt" : "services.txt";
            socketClient->receiveAndSaveFile(filename);
        }
        else if (command == "list::process" || command == "help::cmd") {
            filename = (command == "list::process") ? "processes.txt" : "help.txt";
            socketClient->receiveAndSaveFile(filename);
        }
        else if (command == "screenshot::capture" || command == "camera::open") {
            filename = (command == "screenshot::capture") ? "screenshot.png" : "webcam.png";
            socketClient->receiveAndSaveImage(filename);

            if (command == "camera::open") {
                isCameraOpen = true;
                wxButton* camButton = dynamic_cast<wxButton*>(FindWindow(ID_OPEN_CAM));
                if (camButton) {
                    camButton->SetLabel("Close Camera");
                }
                UpdateStatus("Camera opened successfully");
            }
        }
        else if (command == "camera::close")
        {
            isCameraOpen = false;
            wxButton* camButton = dynamic_cast<wxButton*>(FindWindow(ID_OPEN_CAM));
            if (camButton) {
                camButton->SetLabel("Open Camera");
            }
            UpdateStatus("Camera closed successfully");
        }
        else if (command.substr(0, 5) == "view::") {
            filename = "received_file.txt";
            socketClient->receiveAndSaveFile(filename);
        }
        else if (command.substr(0, 10) == "app::start" || command.substr(0, 9) == "app::stop") {
            if (socketClient->sendData(command.c_str(), command.length()) == SOCKET_ERROR) {
                UpdateStatus("Failed to send application control command");
                socketClient->disconnect();
                return;
            }
            UpdateStatus("Application control command sent: " + command);
        }
        else if (command.substr(0, 9) == "file::get") {
            std::string filepath = command.substr(10);
            size_t lastSlash = filepath.find_last_of("/\\");
            filename = (lastSlash != std::string::npos) ? filepath.substr(lastSlash + 1) : filepath;
            socketClient->receiveAndSaveFile(filename);
            UpdateStatus("File received: " + filename);
        }
        else if (command.substr(0, 12) == "file::delete") {
            UpdateStatus("File delete command executed");
        }

        if (!filename.empty())
        {
            wxString currentDir = wxGetCwd();
            wxString filePath = wxFileName(currentDir, filename).GetFullPath();
            std::string path_command = "save_path:" + filePath.ToStdString();
            socketClient->sendData(path_command.c_str(), path_command.length());
        }

        // Gửi email phản hồi
        std::string replyMessage = "This is an automated reply to your email.\nCommand: " + command + "\nServer IP: " + ip;
        bool success = emailHandler->sendReplyEmail(
            emailInfo.from,
            emailInfo.subject,
            replyMessage,
            emailInfo.threadId,
            filename
        );

        if (success) {
            UpdateStatus(filename.empty() ?
                "Reply sent successfully" :
                "Reply sent successfully with attachment");
        }
        else {
            UpdateStatus(filename.empty() ?
                "Failed to send reply" :
                "Failed to send reply with attachment");
        }

        // Ngắt kết nối sau khi hoàn thành
        socketClient->disconnect();
        UpdateStatus("Disconnected from server: " + ip);
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

        wxFileDialog saveFileDialog(this, "Save Applications List", "", "applications.txt",
            "Text files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL) {
            UpdateStatus("Operation cancelled: No save location selected");
            return;
        }

        // Chỉ gửi command sau khi người dùng đã chọn nơi lưu file
        if (socketClient->sendData("list::app", 9) == SOCKET_ERROR) {
            UpdateStatus("Failed to send list app command");
            return;
        }

        wxString filePath = saveFileDialog.GetPath();
        socketClient->receiveAndSaveFile(filePath.ToStdString());

        // Gửi đường dẫn đến file cho server
        std::string path_command = "save_path:" + filePath.ToStdString();
        socketClient->sendData(path_command.c_str(), path_command.length());

        UpdateStatus("Applications list saved to " + filePath);
        ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL, SW_SHOW);
    }

    void MainFrame::OnListProcess(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        wxFileDialog saveFileDialog(this, "Save Process List", "", "processes.txt",
            "Text files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL) {
            UpdateStatus("Operation cancelled: No save location selected");
            return;
        }

        // Chỉ gửi command sau khi người dùng đã chọn nơi lưu file
        if (socketClient->sendData("list::process", 13) == SOCKET_ERROR) {
            UpdateStatus("Failed to send list process command");
            return;
        }

        wxString filePath = saveFileDialog.GetPath();
        socketClient->receiveAndSaveFile(filePath.ToStdString());

        // Gửi đường dẫn đến file cho server
        std::string path_command = "save_path:" + filePath.ToStdString();
        socketClient->sendData(path_command.c_str(), path_command.length());

        UpdateStatus("Process list saved to " + filePath);
        ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL, SW_SHOW);
    }

    void MainFrame::OnListService(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        wxFileDialog saveFileDialog(this, "Save Services List", "", "services.txt",
            "Text files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL) {
            UpdateStatus("Operation cancelled: No save location selected");
            return;
        }

        // Chỉ gửi command sau khi người dùng đã chọn nơi lưu file
        if (socketClient->sendData("list::service", 13) == SOCKET_ERROR) {
            UpdateStatus("Failed to send list service command");
            return;
        }

        wxString filePath = saveFileDialog.GetPath();
        socketClient->receiveAndSaveFile(filePath.ToStdString());

        // Gửi đường dẫn đến file cho server
        std::string path_command = "save_path:" + filePath.ToStdString();
        socketClient->sendData(path_command.c_str(), path_command.length());

        UpdateStatus("Services list saved to " + filePath);
        ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL, SW_SHOW);
    }

    void MainFrame::OnScreenshot(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        wxFileDialog saveFileDialog(this, "Save Screenshot", "", "screenshot.png",
            "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL) {
            UpdateStatus("Operation cancelled: No save location selected");
            return;
        }

        // Chỉ gửi command sau khi người dùng đã chọn nơi lưu file
        if (socketClient->sendData("screenshot::capture", 19) == SOCKET_ERROR) {
            UpdateStatus("Failed to send screenshot command");
            return;
        }

        wxString filePath = saveFileDialog.GetPath();
        socketClient->receiveAndSaveImage(filePath.ToStdString());

        // Gửi đường dẫn đến file cho server
        std::string path_command = "save_path:" + filePath.ToStdString();
        socketClient->sendData(path_command.c_str(), path_command.length());

        UpdateStatus("Screenshot saved to " + filePath);
        ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL, SW_SHOW);
    }

    void MainFrame::OnOpenCam(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        wxButton* camButton = dynamic_cast<wxButton*>(FindWindow(ID_OPEN_CAM));

        if (!isCameraOpen) {
            wxFileDialog saveFileDialog(this, "Save Webcam Image", "", "webcam.png",
                "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

            if (saveFileDialog.ShowModal() == wxID_CANCEL) {
                UpdateStatus("Operation cancelled: No save location selected");
                return;
            }

            // Chỉ gửi command sau khi người dùng đã chọn nơi lưu file
            if (socketClient->sendData("camera::open", 12) == SOCKET_ERROR) {
                UpdateStatus("Failed to send open cam command");
                return;
            }

            wxString filePath = saveFileDialog.GetPath();
            socketClient->receiveAndSaveImage(filePath.ToStdString());

            // Gửi đường dẫn đến file cho server
            std::string path_command = "save_path:" + filePath.ToStdString();
            socketClient->sendData(path_command.c_str(), path_command.length());

            UpdateStatus("Webcam image saved to " + filePath);
            ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL, SW_SHOW);

            if (camButton) {
                camButton->SetLabel("Close Camera");
            }
            isCameraOpen = true;
        }
        else {
            // Camera close không cần save file
            if (socketClient->sendData("camera::close", 13) == SOCKET_ERROR) {
                UpdateStatus("Failed to send close cam command");
                return;
            }

            if (camButton) {
                camButton->SetLabel("Open Camera");
            }
            isCameraOpen = false;
            UpdateStatus("Camera closed successfully");
        }
    }

    void MainFrame::OnHelp(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        wxFileDialog saveFileDialog(this, "Save Help Information", "", "help.txt",
            "Text files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL) {
            UpdateStatus("Operation cancelled: No save location selected");
            return;
        }

        // Chỉ gửi command sau khi người dùng đã chọn nơi lưu file
        if (socketClient->sendData("help::cmd", 9) == SOCKET_ERROR) {
            UpdateStatus("Failed to send help command");
            return;
        }

        wxString filePath = saveFileDialog.GetPath();
        socketClient->receiveAndSaveFile(filePath.ToStdString());

        // Gửi đường dẫn đến file cho server
        std::string path_command = "save_path:" + filePath.ToStdString();
        socketClient->sendData(path_command.c_str(), path_command.length());

        UpdateStatus("Help information saved to " + filePath);
        ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL, SW_SHOW);
    }

    void MainFrame::OnToggleApp(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        wxButton* toggleButton = dynamic_cast<wxButton*>(FindWindow(ID_TOGGLE_APP));

        if (!isAppRunning) {
            // Starting application
            wxTextEntryDialog dialog(
                this,
                "Enter process name:",
                "Process Manager",
                "",
                wxOK | wxCANCEL | wxCENTRE
            );

            if (dialog.ShowModal() == wxID_OK) {
                wxString appName = dialog.GetValue();
                currentAppName = appName;

                // Gửi lệnh start_app tới server
                std::string command = "app::start " + appName.ToStdString();
                if (socketClient->sendData(command.c_str(), command.length()) == SOCKET_ERROR) {
                    UpdateStatus("Failed to send start app command");
                    return;
                }

                isAppRunning = true;
                if (toggleButton) {
                    toggleButton->SetLabel("Stop App");
                }
                UpdateStatus("Start application command sent for: " + appName);
            }
        }
        else {
            // Gửi lệnh stop_app kèm tên ứng dụng tới server
            std::string command = "app::stop " + currentAppName.ToStdString();
            if (socketClient->sendData(command.c_str(), command.length()) == SOCKET_ERROR) {
                UpdateStatus("Failed to send stop app command");
                return;
            }

            isAppRunning = false;
            if (toggleButton) {
                toggleButton->SetLabel("Start App");
            }
            UpdateStatus("Stop application command sent for: " + currentAppName);
        }
    }

    void MainFrame::OnShutdown(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        if (socketClient->sendData("system::shutdown", 16) == SOCKET_ERROR) {
            UpdateStatus("Failed to send shutdown command");
            return;
        }
        UpdateStatus("Shutdown command sent successfully");
    }

    void MainFrame::OnRestart(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        if (socketClient->sendData("system::restart", 15) == SOCKET_ERROR) {
            UpdateStatus("Failed to send restart command");
            return;
        }
        UpdateStatus("Restart command sent successfully");
    }

    void MainFrame::OnLockScreen(wxCommandEvent& event) {
        if (!socketClient->isConnected()) {
            UpdateStatus("Error: Not connected to server");
            return;
        }

        if (socketClient->sendData("system::lock", 12) == SOCKET_ERROR) {
            UpdateStatus("Failed to send lock screen command");
            return;
        }
        UpdateStatus("Lock screen command sent successfully");
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


    // And here's the corrected UpdateStatus method:
    void MainFrame::UpdateStatus(const wxString& message) {
        wxDateTime now = wxDateTime::Now();
        wxString timestamp = now.FormatTime();

        wxFont teletype(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

        wxTextAttr timestampStyle;
        timestampStyle.SetTextColour(wxColour(128, 128, 128));
        timestampStyle.SetFont(teletype);

        wxTextAttr messageStyle;
        wxString prefix;
        wxString loweredMsg = message.Lower();

        if (loweredMsg.Contains("error") || loweredMsg.Contains("failed") ||
            loweredMsg.Contains("unable") || loweredMsg.Contains("invalid")) {
            messageStyle.SetTextColour(wxColour(255, 99, 71));
            prefix = "ERROR";
        }
        else if (loweredMsg.Contains("success") || loweredMsg.Contains("connected") ||
            loweredMsg.Contains("authenticated") || loweredMsg.Contains("saved")) {
            messageStyle.SetTextColour(wxColour(46, 204, 113));
            prefix = "SUCCESS";
        }
        else if (loweredMsg.Contains("warning")) {
            messageStyle.SetTextColour(wxColour(241, 196, 15));
            prefix = "WARNING";
        }
        else {
            messageStyle.SetTextColour(wxColour(189, 195, 199));
            prefix = "INFO";
        }

        txtStatus->SetDefaultStyle(timestampStyle);
        txtStatus->AppendText("[" + timestamp + "] ");

        teletype.SetWeight(wxFONTWEIGHT_BOLD);
        messageStyle.SetFont(teletype);
        txtStatus->SetDefaultStyle(messageStyle);
        txtStatus->AppendText("[" + prefix + "] ");

        teletype.SetWeight(wxFONTWEIGHT_NORMAL);
        messageStyle.SetFont(teletype);
        txtStatus->SetDefaultStyle(messageStyle);
        txtStatus->AppendText(message + "\n");

        txtStatus->ShowPosition(txtStatus->GetLastPosition());
    }

    