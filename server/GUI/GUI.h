#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/dialog.h>
#include <wx/mstream.h>
#include "socket.h"
#include "CommandExecutor.h"
#include <thread>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <wx/rawbmp.h>
#include <wx/filename.h>

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
        bool isVideo;
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

// Thêm vào GUI.h sau ImageDialog
// Trong GUI.h sau ImageDialog
class VideoDialog : public wxDialog {
public:
    VideoDialog(wxWindow* parent, const wxString& title, const std::vector<BYTE>& videoData)
        : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)),
        m_isPlaying(false), m_shouldExit(false)
    {
        SetBackgroundColour(wxColour(30, 30, 30));
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        // Panel hiển thị video
        m_videoPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition,
            wxSize(760, 480), wxBORDER_NONE);
        m_videoPanel->SetBackgroundColour(wxColour(0, 0, 0));
        mainSizer->Add(m_videoPanel, 1, wxEXPAND | wxALL, 10);

        // Controls
        wxBoxSizer* controlSizer = new wxBoxSizer(wxHORIZONTAL);

        // Buttons với ID riêng
        m_playPauseButton = CreateStyledButton("Play", wxColour(60, 60, 60));
        m_restartButton = CreateStyledButton("Restart", wxColour(60, 60, 60));
        m_closeButton = CreateStyledButton("Close", wxColour(60, 60, 60));

        controlSizer->Add(m_playPauseButton, 0, wxRIGHT, 10);
        controlSizer->Add(m_restartButton, 0, wxRIGHT, 10);
        controlSizer->Add(m_closeButton, 0, wxRIGHT, 10);

        mainSizer->Add(controlSizer, 0, wxALIGN_CENTER | wxBOTTOM, 10);

        SetSizer(mainSizer);

        // Events
        m_playPauseButton->Bind(wxEVT_BUTTON, &VideoDialog::OnPlayPause, this);
        m_restartButton->Bind(wxEVT_BUTTON, &VideoDialog::OnRestart, this);
        m_closeButton->Bind(wxEVT_BUTTON, &VideoDialog::OnClose, this);

        // Window close event
        Bind(wxEVT_CLOSE_WINDOW, &VideoDialog::OnCloseWindow, this);

        // Lưu video data vào file tạm
        m_tempVideoFile = wxFileName::CreateTempFileName("video") + ".avi";
        std::ofstream outFile(m_tempVideoFile.ToStdString(), std::ios::binary);
        outFile.write(reinterpret_cast<const char*>(videoData.data()), videoData.size());
        outFile.close();

        // Khởi tạo video capture
        InitializeVideoCapture();

        // Bắt đầu thread video
        m_videoThread = std::thread(&VideoDialog::VideoLoop, this);

        CenterOnParent();
    }

    ~VideoDialog() {
        StopVideo();
        if (m_videoThread.joinable()) {
            m_videoThread.join();
        }
        wxRemoveFile(m_tempVideoFile);
    }

private:
    wxPanel* m_videoPanel;
    wxButton* m_playPauseButton;
    wxButton* m_restartButton;
    wxButton* m_closeButton;
    cv::VideoCapture m_capture;
    std::thread m_videoThread;
    bool m_isPlaying;
    bool m_shouldExit;
    wxString m_tempVideoFile;
    std::mutex m_mutex;

    wxButton* CreateStyledButton(const wxString& label, const wxColour& color) {
        wxButton* button = new wxButton(this, wxID_ANY, label,
            wxDefaultPosition, wxSize(80, 30));
        button->SetBackgroundColour(color);
        button->SetForegroundColour(wxColour(255, 255, 255));
        return button;
    }

    void InitializeVideoCapture() {
        m_capture.open(m_tempVideoFile.ToStdString());
        if (!m_capture.isOpened()) {
            wxMessageBox("Failed to open video file", "Error",
                wxOK | wxICON_ERROR);
        }
    }

    void OnPlayPause(wxCommandEvent& event) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isPlaying = !m_isPlaying;
        m_playPauseButton->SetLabel(m_isPlaying ? "Pause" : "Play");
    }

    void OnRestart(wxCommandEvent& event) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_capture.set(cv::CAP_PROP_POS_FRAMES, 0);
        m_isPlaying = true;
        m_playPauseButton->SetLabel("Pause");
    }

    void OnClose(wxCommandEvent& event) {
        Close();
    }

    void OnCloseWindow(wxCloseEvent& event) {
        StopVideo();
        event.Skip();
    }

    void StopVideo() {
        m_shouldExit = true;
        m_isPlaying = false;
    }

    void VideoLoop() {
        cv::Mat frame;

        // Lấy FPS từ video gốc
        double fps = m_capture.get(cv::CAP_PROP_FPS);
        if (fps <= 0) fps = 30.0; // Nếu không lấy được FPS thì dùng 30 FPS mặc định

        const int frameDelay = static_cast<int>(1000.0 / fps); // Chuyển FPS sang milliseconds

        while (!m_shouldExit) {
            if (m_isPlaying) {
                auto startTime = std::chrono::steady_clock::now();

                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (!m_capture.read(frame)) {
                        m_capture.set(cv::CAP_PROP_POS_FRAMES, 0);
                        continue;
                    }
                }

                if (!frame.empty()) {
                    int panelWidth, panelHeight;
                    m_videoPanel->GetSize(&panelWidth, &panelHeight);

                    double scaleWidth = static_cast<double>(panelWidth) / frame.cols;
                    double scaleHeight = static_cast<double>(panelHeight) / frame.rows;
                    double scale = std::min(scaleWidth, scaleHeight);

                    cv::Mat scaledFrame;
                    if (scale < 1) {
                        cv::resize(frame, scaledFrame, cv::Size(), scale, scale,
                            cv::INTER_LINEAR);
                    }
                    else {
                        scaledFrame = frame;
                    }

                    cv::cvtColor(scaledFrame, scaledFrame, cv::COLOR_BGR2RGB);
                    wxBitmap bitmap(scaledFrame.cols, scaledFrame.rows, 24);
                    wxPixelData<wxBitmap, wxNativePixelFormat> data(bitmap);

                    if (data) {
                        wxPixelData<wxBitmap, wxNativePixelFormat>::Iterator p(data);
                        for (int y = 0; y < scaledFrame.rows; y++) {
                            wxPixelData<wxBitmap, wxNativePixelFormat>::Iterator rowStart = p;
                            for (int x = 0; x < scaledFrame.cols; x++) {
                                p.Red() = scaledFrame.at<cv::Vec3b>(y, x)[0];
                                p.Green() = scaledFrame.at<cv::Vec3b>(y, x)[1];
                                p.Blue() = scaledFrame.at<cv::Vec3b>(y, x)[2];
                                ++p;
                            }
                            p = rowStart;
                            p.OffsetY(data, 1);
                        }
                    }

                    CallAfter([this, bitmap]() {
                        if (!m_shouldExit) {
                            wxClientDC dc(m_videoPanel);
                            dc.Clear();
                            int x = (m_videoPanel->GetSize().GetWidth() - bitmap.GetWidth()) / 2;
                            int y = (m_videoPanel->GetSize().GetHeight() - bitmap.GetHeight()) / 2;
                            dc.DrawBitmap(bitmap, x, y, false);
                        }
                        });

                    // Đảm bảo mỗi frame được hiển thị đúng thời gian
                    auto endTime = std::chrono::steady_clock::now();
                    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>
                        (endTime - startTime).count();

                    if (elapsedMs < frameDelay) {
                        wxMilliSleep(frameDelay - elapsedMs);
                    }
                }
            }
            else {
                wxMilliSleep(50);  // Khi pause
            }
        }
    }
};