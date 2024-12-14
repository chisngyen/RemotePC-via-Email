#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/dialog.h>
#include <wx/mstream.h>
#include "socket.h"
#include "CommandExecutor.h"
#include <thread>

#define DEFAULT_PORT "27015"


// Custom event for logging
wxDECLARE_EVENT(wxEVT_SERVER_LOG, wxCommandEvent);

class ServerFrame : public wxFrame {
public:
    ServerFrame(const wxString& title);
    ~ServerFrame();

private:
    void FormatCommand(wxString& command);
    void OnClearHistory(wxCommandEvent& event);

    void OnButtonHover(wxMouseEvent& event);
    void OnButtonLeave(wxMouseEvent& event);

    // GUI Controls
    wxButton* startButton;
    wxButton* stopButton;
    wxStaticText* statusText;
    wxListCtrl* logList;
    wxTextCtrl* messageLog;  // For normal messages

    // Server components
    SocketServer* server;
    std::thread* serverThread;
    bool isRunning;
    Command cmd;

    // Event handlers
    void OnStart(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnLogEvent(wxCommandEvent& event);
    void OnLogItemDblClick(wxListEvent& event);

    // Server control methods
    void StartServer();
    void StopServer();
    void ServerLoop();

    // Logging methods
    void LogMessage(const wxString& message, const wxString& details = "", bool isCommand = false);

    // Store command responses for double-click viewing
    struct LogEntry {
        wxString message;
        wxString details;
        bool isCommand;
        wxString savedPath;
        wxString content;
        std::vector<BYTE> imageData;  // Thêm trường này để lưu dữ liệu ảnh
        bool isImage;                 // Flag để biết entry này có chứa ảnh không
    };
    std::vector<LogEntry> logEntries;

    DECLARE_EVENT_TABLE()
};

class ContentDialog : public wxDialog {
public:
    ContentDialog(wxWindow* parent, const wxString& title, const wxString& content)
        : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600))
    {
        // Đặt background màu trắng cho dialog
        SetBackgroundColour(wxColour(255, 255, 255));

        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

        // Text control với style sáng
        wxTextCtrl* textCtrl = new wxTextCtrl(this, wxID_ANY, content,
            wxDefaultPosition, wxDefaultSize,
            wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);

        // Màu nền trắng và chữ đen
        textCtrl->SetBackgroundColour(wxColour(255, 255, 255));
        textCtrl->SetForegroundColour(wxColour(0, 0, 0));

        // Font monospace rõ ràng
        wxFont font(11, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        textCtrl->SetFont(font);

        // Thêm padding để text không sát viền
        sizer->Add(textCtrl, 1, wxEXPAND | wxALL, 15);

        // Style cho nút OK
        wxButton* okButton = new wxButton(this, wxID_OK, "OK",
            wxDefaultPosition, wxSize(100, 30));
        okButton->SetBackgroundColour(wxColour(240, 240, 240));
        okButton->SetForegroundColour(wxColour(0, 0, 0));

        // Căn giữa nút OK và thêm margin
        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        buttonSizer->Add(okButton, 0, wxALIGN_CENTER);
        sizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);

        SetSizer(sizer);

        // Căn giữa dialog trên cửa sổ cha
        CenterOnParent();
    }
};

class ImageDialog : public wxDialog {
public:
    ImageDialog(wxWindow* parent, const wxString& title, const std::vector<BYTE>& imageData)
        : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600))
    {
        // Đặt background màu tối
        SetBackgroundColour(wxColour(30, 30, 30));

        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

        // Đảm bảo image handler được khởi tạo
        wxInitAllImageHandlers();

        // Convert image data to wxImage
        wxMemoryInputStream memStream(imageData.data(), imageData.size());
        wxImage image;
        if (image.LoadFile(memStream, wxBITMAP_TYPE_PNG)) {
            // Scale image to fit dialog while maintaining aspect ratio
            int dialogWidth, dialogHeight;
            GetSize(&dialogWidth, &dialogHeight);
            dialogHeight -= 50; // Space for button

            double scaleWidth = (double)dialogWidth / image.GetWidth();
            double scaleHeight = (double)dialogHeight / image.GetHeight();
            double scale = std::min(scaleWidth, scaleHeight);

            if (scale < 1) {
                image.Rescale(image.GetWidth() * scale, image.GetHeight() * scale, wxIMAGE_QUALITY_HIGH);
            }

            wxBitmap bitmap(image);
            wxStaticBitmap* staticBitmap = new wxStaticBitmap(this, wxID_ANY, bitmap);
            sizer->Add(staticBitmap, 1, wxALIGN_CENTER | wxALL, 5);
        }
        else {
            wxStaticText* errorText = new wxStaticText(this, wxID_ANY, "Failed to load image");
            errorText->SetForegroundColour(wxColour(255, 255, 255));  // White text
            sizer->Add(errorText, 1, wxALIGN_CENTER | wxALL, 5);
        }

        // Modern styled OK button
        wxButton* okButton = new wxButton(this, wxID_OK, "OK",
            wxDefaultPosition, wxSize(100, 30));
        okButton->SetBackgroundColour(wxColour(60, 60, 60));
        okButton->SetForegroundColour(wxColour(255, 255, 255));
        sizer->Add(okButton, 0, wxALIGN_CENTER | wxALL, 10);

        SetSizer(sizer);
        CenterOnParent();
    }
};