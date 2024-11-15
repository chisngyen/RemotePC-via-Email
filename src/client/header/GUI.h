#pragma once
#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/filedlg.h>
#include "socket.h"
#include "GmailAPI.h"
#include "handleMail.h"

// Control IDs
enum {
    ID_CONNECT = wxID_HIGHEST + 1,
    ID_DISCONNECT,
    ID_AUTHENTICATE,
    ID_SUBMIT_AUTH,
    ID_START_MONITORING,
    ID_CHECK_EMAIL_TIMER,
    ID_LIST_APP,
    ID_LIST_PROCESS,
    ID_LIST_SERVICE,
    ID_SCREENSHOT,
    ID_OPEN_CAM,
    ID_HELP
};
#define REDIRECT_URI "http://localhost"
class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);
    ~MainFrame();

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

    // Methods
    void LoadClientSecrets();
    void UpdateStatus(const wxString& message);
    void UpdateConnectionStatus();
    void UpdateCommandsList(const std::string& command);

    // Event handlers
    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnAuthenticate(wxCommandEvent& event);
    void OnSubmitAuth(wxCommandEvent& event);
    void OnStartMonitoring(wxCommandEvent& event);
    void OnCheckEmail(wxTimerEvent& event);
    void OnListApp(wxCommandEvent& event);
    void OnListProcess(wxCommandEvent& event);
    void OnListService(wxCommandEvent& event);
    void OnScreenshot(wxCommandEvent& event);
    void OnOpenCam(wxCommandEvent& event);
    void OnHelp(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};