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

class VideoDialog : public wxDialog {
public:
    VideoDialog(wxWindow* parent, const wxString& title, const std::vector<BYTE>& videoData)
        : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600),
            wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
        m_isPlaying(false), m_shouldExit(false), m_bitmap(nullptr)
    {
        SetBackgroundColour(wxColour(30, 30, 30));
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        // Video panel
        m_videoPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition,
            wxSize(760, 480), wxFULL_REPAINT_ON_RESIZE);
        m_videoPanel->SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        m_videoPanel->SetDoubleBuffered(true);

        m_videoPanel->Bind(wxEVT_PAINT, &VideoDialog::OnPaintVideo, this);
        m_videoPanel->Bind(wxEVT_SIZE, &VideoDialog::OnPanelResize, this);

        mainSizer->Add(m_videoPanel, 1, wxEXPAND | wxALL, 5);

        // Controls
        wxPanel* controlPanel = new wxPanel(this, wxID_ANY);
        controlPanel->SetBackgroundColour(wxColour(30, 30, 30));
        wxBoxSizer* controlSizer = new wxBoxSizer(wxHORIZONTAL);
        controlSizer->AddStretchSpacer(1);

        m_playPauseButton = CreateStyledButton(controlPanel, "Play", wxSize(90, 35));
        m_restartButton = CreateStyledButton(controlPanel, "Restart", wxSize(90, 35));
        m_closeButton = CreateStyledButton(controlPanel, "Close", wxSize(90, 35));

        controlSizer->Add(m_playPauseButton, 0, wxALIGN_CENTER | wxRIGHT, 8);
        controlSizer->Add(m_restartButton, 0, wxALIGN_CENTER | wxRIGHT, 8);
        controlSizer->Add(m_closeButton, 0, wxALIGN_CENTER);
        controlSizer->AddStretchSpacer(1);

        controlPanel->SetSizer(controlSizer);
        mainSizer->Add(controlPanel, 0, wxEXPAND | wxALL, 5);

        SetSizer(mainSizer);

        // Events
        m_playPauseButton->Bind(wxEVT_BUTTON, &VideoDialog::OnPlayPause, this);
        m_restartButton->Bind(wxEVT_BUTTON, &VideoDialog::OnRestart, this);
        m_closeButton->Bind(wxEVT_BUTTON, &VideoDialog::OnClose, this);
        Bind(wxEVT_CLOSE_WINDOW, &VideoDialog::OnCloseWindow, this);

        // Tạo file tạm
        m_tempVideoFile = wxFileName::CreateTempFileName("video") + ".avi";
        std::ofstream outFile(m_tempVideoFile.ToStdString(), std::ios::binary);
        outFile.write(reinterpret_cast<const char*>(videoData.data()), videoData.size());
        outFile.close();

        InitializeVideoCapture();

        // Start video thread
        m_videoThread = std::thread(&VideoDialog::VideoLoop, this);

        CenterOnParent();
    }

    ~VideoDialog() {
        StopVideo();
        if (m_videoThread.joinable()) {
            m_videoThread.join();
        }
        if (m_bitmap) {
            delete m_bitmap;
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
    wxBitmap* m_bitmap;

    wxButton* CreateStyledButton(wxWindow* parent, const wxString& label, const wxSize& size) {
        wxButton* button = new wxButton(parent, wxID_ANY, label, wxDefaultPosition, size);
        button->SetBackgroundColour(wxColour(60, 60, 60));
        button->SetForegroundColour(wxColour(255, 255, 255));
        return button;
    }

    void InitializeVideoCapture() {
        m_capture.open(m_tempVideoFile.ToStdString());
        if (!m_capture.isOpened()) {
            wxMessageBox("Failed to open video file", "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnPanelResize(wxSizeEvent& event) {
        m_videoPanel->Refresh(false);
        event.Skip();
    }

    void UpdateFrame(cv::Mat& frame) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Lấy kích thước panel
        int panelWidth = m_videoPanel->GetSize().GetWidth();
        int panelHeight = m_videoPanel->GetSize().GetHeight();

        // Tính toán tỷ lệ scale để giữ aspect ratio
        double scaleWidth = static_cast<double>(panelWidth) / frame.cols;
        double scaleHeight = static_cast<double>(panelHeight) / frame.rows;
        double scale = std::min(scaleWidth, scaleHeight);

        // Tính kích thước mới giữ nguyên tỷ lệ
        int newWidth = static_cast<int>(frame.cols * scale);
        int newHeight = static_cast<int>(frame.rows * scale);

        // Tạo frame mới với kích thước đã tính
        cv::Mat scaledFrame;
        cv::resize(frame, scaledFrame, cv::Size(newWidth, newHeight), 0, 0, cv::INTER_LINEAR);
        cv::cvtColor(scaledFrame, scaledFrame, cv::COLOR_BGR2RGB);

        // Tạo hoặc cập nhật bitmap
        if (!m_bitmap || m_bitmap->GetWidth() != newWidth || m_bitmap->GetHeight() != newHeight) {
            if (m_bitmap) delete m_bitmap;
            m_bitmap = new wxBitmap(newWidth, newHeight, 24);
        }

        // Copy dữ liệu frame vào bitmap
        wxNativePixelData data(*m_bitmap);
        if (data) {
            wxNativePixelData::Iterator p(data);
            for (int y = 0; y < scaledFrame.rows; y++) {
                wxNativePixelData::Iterator rowStart = p;
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

        m_videoPanel->Refresh(false);
    }

    void OnPaintVideo(wxPaintEvent& evt) {
        wxPaintDC dc(m_videoPanel);
        std::lock_guard<std::mutex> lock(m_mutex);

        // Clear background
        dc.SetBackground(wxBrush(wxColour(0, 0, 0)));
        dc.Clear();

        if (m_bitmap && m_bitmap->IsOk()) {
            // Tính toán vị trí để căn giữa bitmap trong panel
            int x = (m_videoPanel->GetSize().GetWidth() - m_bitmap->GetWidth()) / 2;
            int y = (m_videoPanel->GetSize().GetHeight() - m_bitmap->GetHeight()) / 2;
            dc.DrawBitmap(*m_bitmap, x, y, false);
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
        double fps = m_capture.get(cv::CAP_PROP_FPS);
        if (fps <= 0) fps = 30.0;

        const int frameDelay = static_cast<int>(1000.0 / fps);

        while (!m_shouldExit) {
            if (m_isPlaying) {
                auto startTime = std::chrono::steady_clock::now();

                bool frameRead;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    frameRead = m_capture.read(frame);
                    if (!frameRead) {
                        m_capture.set(cv::CAP_PROP_POS_FRAMES, 0);
                        continue;
                    }
                }

                if (!frame.empty()) {
                    CallAfter([this, frame]() mutable {
                        if (!m_shouldExit) {
                            UpdateFrame(frame);
                        }
                        });

                    auto endTime = std::chrono::steady_clock::now();
                    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>
                        (endTime - startTime).count();

                    if (elapsedMs < frameDelay) {
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(frameDelay - elapsedMs));
                    }
                }
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }
};