#pragma once
#include <wx/wx.h>
#include <wx/statline.h>

class LoginFrame : public wxFrame {
public:
    LoginFrame(const wxString& title);
    void CloseWithModernEffect();
    void LoadSavedGmails();

private:
    wxComboBox* gmailComboBox;
    wxTextCtrl* txtGmail;
    wxButton* btnLogin;
    void OnLogin(wxCommandEvent& event);
    void OnPaint(wxPaintEvent& event);
    void CreateGradientBackground(wxDC& dc);


    wxDECLARE_EVENT_TABLE();
};

enum {
    ID_LOGIN_BUTTON = 1001
};