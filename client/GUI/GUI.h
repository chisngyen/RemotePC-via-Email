#pragma once

// Standard wxWidgets includes
#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/filedlg.h>
#include <wx/artprov.h>

// Custom includes
#include "socket.h"
#include "GmailAPI.h"
#include "TokenManager.h"
#include "handleMail.h"
#include "OAuthServer.h"

// Constants
#define REDIRECT_URI "http://localhost"

// Control IDs for various UI elements
enum {
    ID_CONNECT = wxID_HIGHEST + 1,
    ID_DISCONNECT,
    ID_AUTHENTICATE,
    ID_START_MONITORING,
    ID_CHECK_EMAIL_TIMER,
    ID_LIST_APP,
    ID_LIST_PROCESS,
    ID_LIST_SERVICE,
    ID_SCREENSHOT,
    ID_OPEN_CAM,
    ID_HELP,
    ID_SHUTDOWN,
    ID_RESTART,
    ID_LOCKSCREEN,
    ID_TOGGLE_APP,
    ID_LOGOUT
};

// Data structures
struct LogEntry {
    wxString message;
    wxString details;
    bool isCommand;
    wxString savedPath;
};

struct CommandGroupPanels {
    wxPanel* groupPanel;
    wxPanel* contentPanel;
};


class CommandPopup : public wxDialog {
public:
    CommandPopup(wxWindow* parent, const wxString& title,
        const wxArrayString& commands,
        const wxPoint& position,
        const wxColour& headerColor)
        : wxDialog(parent, wxID_ANY, title,
            position, wxDefaultSize,
            wxBORDER_NONE)
    {
        SetBackgroundColour(wxColour(45, 49, 58));

        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        // Header
        wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
        headerPanel->SetBackgroundColour(headerColor);

        wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);

        wxStaticText* titleText = new wxStaticText(headerPanel, wxID_ANY, title);
        titleText->SetForegroundColour(*wxWHITE);
        wxFont titleFont = titleText->GetFont();
        titleFont.SetPointSize(10);
        titleFont.SetWeight(wxFONTWEIGHT_BOLD);
        titleText->SetFont(titleFont);

        headerSizer->Add(titleText, 1, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        headerPanel->SetSizer(headerSizer);

        mainSizer->Add(headerPanel, 0, wxEXPAND);

        // Commands list
        wxPanel* contentPanel = new wxPanel(this);
        contentPanel->SetBackgroundColour(wxColour(40, 44, 52));
        wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);
        wxFont contentFont(11, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);

        for (const wxString& cmd : commands) {
            wxButton* cmdButton = new wxButton(contentPanel, wxID_ANY, cmd,
                wxDefaultPosition, wxSize(200, 40));
            cmdButton->SetBackgroundColour(wxColour(60, 64, 72));
            cmdButton->SetForegroundColour(*wxWHITE);
            cmdButton->SetFont(contentFont);
            contentSizer->Add(cmdButton, 0, wxEXPAND | wxALL, 5);

            // Xử lý sự kiện click cho từng nút lệnh
            cmdButton->Bind(wxEVT_BUTTON, [this, parent, cmd](wxCommandEvent&) {
                int commandId = -1;
                if (cmd == "Applications List") commandId = ID_LIST_APP;
                else if (cmd == "Process List") commandId = ID_LIST_PROCESS;
                else if (cmd == "Services List") commandId = ID_LIST_SERVICE;
                else if (cmd == "Take Screenshot") commandId = ID_SCREENSHOT;
                else if (cmd == "Camera Control") commandId = ID_OPEN_CAM;
                else if (cmd == "Shutdown") commandId = ID_SHUTDOWN;
                else if (cmd == "Restart") commandId = ID_RESTART;
                else if (cmd == "Lock Screen") commandId = ID_LOCKSCREEN;
                else if (cmd == "Application Control") commandId = ID_TOGGLE_APP;
                else if (cmd == "Help") commandId = ID_HELP;

                if (commandId != -1) {
                    wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, commandId);
                    parent->GetEventHandler()->ProcessEvent(event);
                }
                });

            // Hiệu ứng hover
            cmdButton->Bind(wxEVT_ENTER_WINDOW, [cmdButton](wxMouseEvent&) {
                cmdButton->SetBackgroundColour(wxColour(70, 74, 82));
                cmdButton->Refresh();
                });

            cmdButton->Bind(wxEVT_LEAVE_WINDOW, [cmdButton](wxMouseEvent&) {
                cmdButton->SetBackgroundColour(wxColour(60, 64, 72));
                cmdButton->Refresh();
                });
        }

        contentPanel->SetSizer(contentSizer);
        mainSizer->Add(contentPanel, 1, wxEXPAND | wxALL, 5);

        SetSizer(mainSizer);
        Fit();
    }
};

class MainFrame : public wxFrame {
public:
    // Constructor and destructor
    MainFrame(const wxString& title);
    ~MainFrame();

    // Public methods
    void AutoStartAuthentication();
    void SetUserGmail(const wxString& gmail) { userGmail = gmail; }
    void SetParentFrame(wxFrame* parent) { parentFrame = parent; }
    void ShowWithModernEffect();
    void SetExistingRefreshToken(const std::string& token);

private:
    // GUI Controls
    wxTextCtrl* txtIpAddress;
    wxTextCtrl* txtPort;
    wxButton* btnConnect;
    wxStaticText* lblConnectionStatus;
    wxButton* btnAuthenticate;
    wxButton* btnSubmitAuth;
    wxTextCtrl* txtAuthCode;
    wxButton* btnStartMonitoring;
    wxTextCtrl* txtStatus;
    wxTextCtrl* txtCommands;
    wxButton* btnLogout;
    wxButton* btnToggleApp;
    wxScrolledWindow* commandsScroll;
    wxBoxSizer* commandsSizer;

    // Backend components
    SocketClient* socketClient;
    GoogleOAuth* oauth;
    EmailHandler* emailHandler;
    wxTimer* checkEmailTimer;
    GmailUIAutomation* gmailAutomation;
    TokenManager tokenManager;
    OAuthCallbackServer* callbackServer;
    CommandPopup* currentPopup;

    // State variables
    bool isMonitoring;
    bool isAppRunning;
    bool isCameraOpen;
    DWORD currentProcessId;
    wxString currentAppName;
    wxString userGmail;
    wxFrame* parentFrame;

    // OAuth credentials
    std::string clientId;
    std::string clientSecret;
    std::string refreshToken;
    std::string accessToken;

    // Private methods
    void LoadClientSecrets();
    void UpdateStatus(const wxString& message);
    void UpdateConnectionStatus();
    void UpdateCommandsList(const wxString& command, const EmailHandler::EmailInfo& emailInfo);
    void ResetApplicationState();

    // Event handlers for buttons
    void OnLogout(wxCommandEvent& event);
    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnAuthenticate(wxCommandEvent& event);
    void OnStartMonitoring(wxCommandEvent& event);
    void OnCheckEmail(wxTimerEvent& event);
    void OnListApp(wxCommandEvent& event);
    void OnListProcess(wxCommandEvent& event);
    void OnListService(wxCommandEvent& event);
    void OnScreenshot(wxCommandEvent& event);
    void OnOpenCam(wxCommandEvent& event);
    void OnHelp(wxCommandEvent& event);
    void OnShutdown(wxCommandEvent& event);
    void OnRestart(wxCommandEvent& event);
    void OnLockScreen(wxCommandEvent& event);
    void OnToggleApp(wxCommandEvent& event);
    void OnOAuthCode(wxCommandEvent& event);

    // UI event handlers
    void OnClose(wxCloseEvent& event);
    void OnButtonHover(wxMouseEvent& event);
    void OnButtonLeave(wxMouseEvent& event);

    DECLARE_EVENT_TABLE()
};

