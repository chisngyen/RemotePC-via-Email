#pragma once
#include <wx/wx.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <string>
#include "handleMail.h"
#include "socket.h"
#include "GmailAPI.h"

class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);
    virtual ~MainFrame();

private:
    // GUI Controls
    wxTextCtrl* txtIpAddress;
    wxTextCtrl* txtPort;
    wxTextCtrl* txtAuthCode;
    wxTextCtrl* txtStatus;
    wxButton* btnConnect;
    wxButton* btnAuthenticate;
    wxButton* btnStartMonitoring;

    // Backend objects
    SocketClient* socketClient;
    GoogleOAuth* oauth;
    EmailHandler* emailHandler;
    bool isMonitoring;
    wxTimer* checkEmailTimer;

    // Configuration
    std::string clientId;
    std::string clientSecret;
    std::string refreshToken;
    std::string accessToken;
    const std::string REDIRECT_URI = "http://localhost";

    // Event handlers
    void OnConnect(wxCommandEvent& event);
    void OnAuthenticate(wxCommandEvent& event);
    void OnStartMonitoring(wxCommandEvent& event);
    void OnCheckEmail(wxTimerEvent& event);
    void LoadClientSecrets();
    void UpdateStatus(const wxString& message);



    DECLARE_EVENT_TABLE()
};

// Custom event IDs
enum {
    ID_CONNECT = wxID_HIGHEST + 1,
    ID_AUTHENTICATE,
    ID_START_MONITORING,
    ID_CHECK_EMAIL_TIMER
};