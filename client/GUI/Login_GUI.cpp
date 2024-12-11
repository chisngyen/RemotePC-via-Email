#include "Login_GUI.h"
#include "GUI.h"

BEGIN_EVENT_TABLE(LoginFrame, wxFrame)
EVT_BUTTON(ID_LOGIN_BUTTON, LoginFrame::OnLogin)
EVT_PAINT(LoginFrame::OnPaint)
END_EVENT_TABLE()

LoginFrame::LoginFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(500, 400))
{
    // Create main panel with custom background
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(wxColour(22, 27, 34));

    // Create vertical sizer for layout
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Title Section
    wxBoxSizer* titleSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* welcomeText = new wxStaticText(panel, wxID_ANY, "Remote Control");
    welcomeText->SetForegroundColour(wxColour(255, 255, 255));
    wxFont welcomeFont = welcomeText->GetFont();
    welcomeFont.SetPointSize(24);
    welcomeFont.SetWeight(wxFONTWEIGHT_BOLD);
    welcomeText->SetFont(welcomeFont);

    wxStaticText* subText = new wxStaticText(panel, wxID_ANY, "Please login with your Gmail account");
    subText->SetForegroundColour(wxColour(156, 163, 175));
    wxFont subFont = subText->GetFont();
    subFont.SetPointSize(12);
    subText->SetFont(subFont);

    titleSizer->Add(welcomeText, 0, wxALIGN_CENTER | wxALL, 5);
    titleSizer->Add(subText, 0, wxALIGN_CENTER | wxBOTTOM, 20);

    // Add decorative line
    wxStaticLine* line = new wxStaticLine(panel, wxID_ANY, wxDefaultPosition,
        wxSize(350, 2), wxLI_HORIZONTAL);
    line->SetForegroundColour(wxColour(55, 65, 81));

    // Gmail input section
    wxBoxSizer* inputContainer = new wxBoxSizer(wxVERTICAL);

    // Create a container for the input field
    wxPanel* inputPanel = new wxPanel(panel, wxID_ANY, wxDefaultPosition,
        wxSize(350, 45), wxBORDER_NONE);
    inputPanel->SetBackgroundColour(wxColour(33, 38, 45));

    wxBoxSizer* gmailSizer = new wxBoxSizer(wxHORIZONTAL);

    // Question mark icon with better alignment
    wxStaticText* emailIcon = new wxStaticText(inputPanel, wxID_ANY, "✉",
        wxDefaultPosition, wxSize(30, -1),
        wxALIGN_CENTER);
    emailIcon->SetForegroundColour(wxColour(139, 148, 158));
    wxFont iconFont = emailIcon->GetFont();
    iconFont.SetPointSize(14);
    emailIcon->SetFont(iconFont);

    // Thay thế txtGmail bằng comboBox
    wxArrayString choices;
    gmailComboBox = new wxComboBox(inputPanel, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize,
        choices, wxCB_DROPDOWN | wxTE_PROCESS_ENTER);
    gmailComboBox->SetBackgroundColour(wxColour(33, 38, 45));
    gmailComboBox->SetForegroundColour(wxColour(255, 255, 255));

    wxFont inputFont = gmailComboBox->GetFont();
    inputFont.SetPointSize(11);
    gmailComboBox->SetFont(inputFont);

    // Load danh sách Gmail đã lưu
    LoadSavedGmails();

    gmailSizer->Add(emailIcon, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    gmailSizer->Add(gmailComboBox, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    inputPanel->SetSizer(gmailSizer);
    inputContainer->Add(inputPanel, 0, wxEXPAND | wxALL, 5);

    // Modern login button with gradient effect
    btnLogin = new wxButton(panel, ID_LOGIN_BUTTON, "Sign in",
        wxDefaultPosition, wxSize(350, 45));
    btnLogin->SetBackgroundColour(wxColour(47, 129, 247));
    btnLogin->SetForegroundColour(wxColour(255, 255, 255));
    wxFont buttonFont = btnLogin->GetFont();
    buttonFont.SetPointSize(12);
    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);
    btnLogin->SetFont(buttonFont);

    // Add content to main sizer with better spacing
    mainSizer->AddSpacer(40);
    mainSizer->Add(titleSizer, 0, wxALIGN_CENTER | wxALL, 10);
    mainSizer->Add(line, 0, wxALIGN_CENTER | wxALL, 20);
    mainSizer->Add(inputContainer, 0, wxALIGN_CENTER | wxALL, 10);
    mainSizer->Add(btnLogin, 0, wxALIGN_CENTER | wxALL, 15);
    mainSizer->AddSpacer(20);

    panel->SetSizer(mainSizer);
    Centre();

    // Set minimum size to prevent window from being too small
    SetMinSize(wxSize(450, 350));
}

void LoginFrame::LoadSavedGmails()
{
    // Danh sách Gmail mẫu
    wxArrayString savedGmails;
    savedGmails.Add("nguyentngu111@gmail.com");
    savedGmails.Add("tcnguyen2365@gmail.com");
    savedGmails.Add("chisboiz111@gmail.com");

    gmailComboBox->Append(savedGmails);
    if (!savedGmails.IsEmpty()) {
        gmailComboBox->SetValue(savedGmails[0]);
    }
}

void LoginFrame::CloseWithModernEffect() {
    wxSize originalSize = GetSize();
    wxPoint originalPos = GetPosition();
    
    double scale = 1.0;
    int alpha = 255;

    while (scale > 0.95 || alpha > 0) {
        // Scale down
        if (scale > 0.95) {
            scale -= 0.002;
            int newWidth = originalSize.GetWidth() * scale;
            int newHeight = originalSize.GetHeight() * scale;
            SetSize(newWidth, newHeight);
            Centre();
        }

        // Fade out
        if (alpha > 0) {
            alpha -= 4;
            SetTransparent(std::max(0, alpha));
        }

        wxMilliSleep(1);
        Update();
    }

    Close();
}

void LoginFrame::OnLogin(wxCommandEvent& event)
{
    wxString gmail = gmailComboBox->GetValue();

    if (!gmail.Contains("@gmail.com") || gmail.Length() < 11) {
        wxMessageDialog dialog(this,
            "Please enter a valid Gmail address\nExample: example@gmail.com",
            "Invalid Email",
            wxOK | wxICON_ERROR);
        dialog.ShowModal();
        gmailComboBox->SetFocus();
        return;
    }

    MainFrame* mainFrame = new MainFrame("Email Monitoring Application");
    mainFrame->SetUserGmail(gmail);
    mainFrame->SetParentFrame(this);
    mainFrame->AutoStartAuthentication();
}

void LoginFrame::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    CreateGradientBackground(dc);
}

void LoginFrame::CreateGradientBackground(wxDC& dc)
{
    wxSize size = GetClientSize();

    // Create modern gradient from dark blue to darker blue
    wxColour startColor(17, 24, 39);    // Dark blue
    wxColour midColor(31, 41, 55);      // Mid blue
    wxColour endColor(55, 65, 81);      // Lighter blue

    for (int i = 0; i < size.GetHeight(); i++)
    {
        double ratio = (double)i / size.GetHeight();
        wxColour currentColor;

        if (ratio < 0.5) {
            // Blend from start to mid
            double blendRatio = ratio * 2;
            currentColor = wxColour(
                startColor.Red() + (midColor.Red() - startColor.Red()) * blendRatio,
                startColor.Green() + (midColor.Green() - startColor.Green()) * blendRatio,
                startColor.Blue() + (midColor.Blue() - startColor.Blue()) * blendRatio
            );
        }
        else {
            // Blend from mid to end
            double blendRatio = (ratio - 0.5) * 2;
            currentColor = wxColour(
                midColor.Red() + (endColor.Red() - midColor.Red()) * blendRatio,
                midColor.Green() + (endColor.Green() - midColor.Green()) * blendRatio,
                midColor.Blue() + (endColor.Blue() - midColor.Blue()) * blendRatio
            );
        }

        wxPen pen(currentColor, 1);
        dc.SetPen(pen);
        dc.DrawLine(0, i, size.GetWidth(), i);
    }

    // Add subtle pattern overlay
    for (int i = 0; i < size.GetHeight(); i += 4) {
        for (int j = 0; j < size.GetWidth(); j += 4) {
            if ((i + j) % 8 == 0) {
                dc.SetPen(wxPen(wxColour(255, 255, 255, 10)));
                dc.DrawPoint(j, i);
            }
        }
    }
}