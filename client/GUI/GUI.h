#pragma once
#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/filedlg.h>
#include "socket.h"
#include "GmailAPI.h"
#include "handleMail.h"
#include "OAuthServer.h"
// Control IDs

struct LogEntry {
    wxString message;
    wxString details;
    bool isCommand;
    wxString savedPath;
};

// Struct for command group panels
struct CommandGroupPanels {
    wxPanel* groupPanel;
    wxPanel* contentPanel;
};

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
    ID_TOGGLE_APP
};
#define REDIRECT_URI "http://localhost"
class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);
    ~MainFrame();

    void AutoStartAuthentication();
    void SetUserGmail(const wxString& gmail) { userGmail = gmail; }
    void SetParentFrame(wxFrame* parent) { parentFrame = parent; }

    void ShowWithModernEffect();

private:

    // Background button
    void OnButtonHover(wxMouseEvent& event);
    void OnButtonLeave(wxMouseEvent& event);

    wxString userGmail;
    wxFrame* parentFrame = nullptr;
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

    wxScrolledWindow* commandsScroll;
    wxBoxSizer* commandsSizer;

    wxButton* btnToggleApp;
    bool isAppRunning = false;
    DWORD currentProcessId = 0;
    wxString currentAppName;

    // Backend components
    SocketClient* socketClient;
    GoogleOAuth* oauth;
    EmailHandler* emailHandler;
    wxTimer* checkEmailTimer;
    bool isMonitoring;

    // OAuth credentials
    std::string clientId;
    std::string clientSecret;
    std::string refreshToken;
    std::string accessToken;

    // Get Code
    OAuthCallbackServer* callbackServer;
    void OnOAuthCode(wxCommandEvent& event);

    GmailUIAutomation* gmailAutomation;

    // Methods
    void LoadClientSecrets();
    void UpdateStatus(const wxString& message);
    void UpdateConnectionStatus();
    void UpdateCommandsList(const wxString& command, const EmailHandler::EmailInfo& emailInfo);
    void ResetApplicationState(); // Added this method declaration

    // Event handlers
    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnAuthenticate(wxCommandEvent& event);
    void OnStartMonitoring(wxCommandEvent& event);
    void OnCheckEmail(wxTimerEvent& event);
    void OnListApp(wxCommandEvent& event);
    void OnListProcess(wxCommandEvent& event);
    void OnListService(wxCommandEvent& event);
    void OnScreenshot(wxCommandEvent& event);
    bool isCameraOpen = false;
    void OnOpenCam(wxCommandEvent& event);

    void OnHelp(wxCommandEvent& event);
    void OnShutdown(wxCommandEvent& event);
    void OnRestart(wxCommandEvent& event);
    void OnLockScreen(wxCommandEvent& event);
    void OnToggleApp(wxCommandEvent& event);
    

    DECLARE_EVENT_TABLE()
};