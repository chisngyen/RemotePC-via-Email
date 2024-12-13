#pragma once

// wxWidgets includes
#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/artprov.h>


// Custom includes
#include "TokenManager.h"

// Control IDs
enum {
    ID_LOGIN_BUTTON = 1001
};

// Login Frame class definition
class LoginFrame : public wxFrame {
public:
    // Constructor and public methods
    LoginFrame(const wxString& title);
    void CloseWithModernEffect();
    void LoadSavedGmails();

private:
    // GUI Controls
    wxComboBox* gmailComboBox;
    wxTextCtrl* txtGmail;
    wxButton* btnLogin;

    // Backend components
    TokenManager tokenManager;
    GoogleOAuth* oauth;

    // Event handlers
    void OnLogin(wxCommandEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnClose(wxCloseEvent& event);

    // UI helper methods
    void CreateGradientBackground(wxDC& dc);

    wxDECLARE_EVENT_TABLE();
};