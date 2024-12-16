#include "GUI.h"
#include <wx/wx.h>
#include <wx/stattext.h>
#include <sstream>
#include <fstream>
#include <ctime>
#include <iomanip>

wxDEFINE_EVENT(wxEVT_SERVER_LOG, wxCommandEvent);

BEGIN_EVENT_TABLE(ServerFrame, wxFrame)
EVT_BUTTON(1001, ServerFrame::OnStart)
EVT_BUTTON(1002, ServerFrame::OnStop)
EVT_CLOSE(ServerFrame::OnClose)
EVT_COMMAND(wxID_ANY, wxEVT_SERVER_LOG, ServerFrame::OnLogEvent)
EVT_LIST_ITEM_ACTIVATED(wxID_ANY, ServerFrame::OnLogItemDblClick)
EVT_BUTTON(1003, ServerFrame::OnClearHistory)
END_EVENT_TABLE()

ServerFrame::ServerFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1024, 800))
{
    // Create main panel with modern dark theme
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(wxColour(18, 18, 18)); // Darker background

    // Set modern font
    wxFont defaultFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    panel->SetFont(defaultFont);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Helper function to create modern sections
    auto createSection = [](wxPanel* parent, const wxString& title) -> wxPanel* {
        wxPanel* section = new wxPanel(parent);
        section->SetBackgroundColour(wxColour(30, 30, 30));

        wxBoxSizer* sectionSizer = new wxBoxSizer(wxVERTICAL);

        wxStaticText* headerText = new wxStaticText(section, wxID_ANY, title);
        headerText->SetForegroundColour(wxColour(200, 200, 200));
        wxFont headerFont = headerText->GetFont();
        headerFont.SetPointSize(11);
        headerFont.SetWeight(wxFONTWEIGHT_BOLD);
        headerText->SetFont(headerFont);

        sectionSizer->Add(headerText, 0, wxALL, 15);
        section->SetSizer(sectionSizer);

        return section;
        };

    // Helper function to create modern buttons
    auto createModernButton = [this](wxWindow* parent, wxWindowID id,
        const wxString& label, const wxColour& baseColor) -> wxButton* {
            wxButton* btn = new wxButton(parent, id, label,
                wxDefaultPosition, wxSize(-1, 40));

            btn->SetBackgroundColour(baseColor);
            btn->SetForegroundColour(wxColour(255, 255, 255));

            wxFont btnFont = btn->GetFont();
            btnFont.SetPointSize(11);  // Increased font size
            btnFont.SetWeight(wxFONTWEIGHT_BOLD);  // Made text bold
            btn->SetFont(btnFont);

            btn->Bind(wxEVT_ENTER_WINDOW, &ServerFrame::OnButtonHover, this);
            btn->Bind(wxEVT_LEAVE_WINDOW, &ServerFrame::OnButtonLeave, this);

            return btn;
        };

    // Server Control Section
    wxPanel* controlPanel = createSection(panel, "Server Control");
    wxBoxSizer* controlContentSizer = dynamic_cast<wxBoxSizer*>(controlPanel->GetSizer());

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    // Brighter, more vibrant colors for buttons
    startButton = createModernButton(controlPanel, 1001, "Start Server",
        wxColour(46, 204, 113));  // Brighter green
    stopButton = createModernButton(controlPanel, 1002, "Stop Server",
        wxColour(231, 76, 60));   // Brighter red
    stopButton->Disable();
    stopButton->SetBackgroundColour(wxColour(231, 76, 60).ChangeLightness(60));

    wxBoxSizer* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* statusLabel = new wxStaticText(controlPanel, wxID_ANY, "Status:");
    statusLabel->SetForegroundColour(wxColour(200, 200, 200));  // Brighter text
    wxFont statusLabelFont = statusLabel->GetFont();
    statusLabelFont.SetPointSize(10);
    statusLabel->SetFont(statusLabelFont);

    statusText = new wxStaticText(controlPanel, wxID_ANY, "OFF");
    statusText->SetForegroundColour(*wxRED);
    wxFont statusFont = statusText->GetFont();
    statusFont.SetPointSize(10);
    statusFont.SetWeight(wxFONTWEIGHT_BOLD);
    statusText->SetFont(statusFont);

    buttonSizer->Add(startButton, 0, wxRIGHT, 10);
    buttonSizer->Add(stopButton, 0, wxRIGHT, 20);
    buttonSizer->Add(statusLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    buttonSizer->Add(statusText, 0, wxALIGN_CENTER_VERTICAL);

    controlContentSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 15);

    // Client Commands Log Section
    wxPanel* commandPanel = createSection(panel, "Client Commands");
    wxBoxSizer* commandContentSizer = dynamic_cast<wxBoxSizer*>(commandPanel->GetSizer());

    // Cải thiện label "Client Commands"
    wxStaticText* headerText = (wxStaticText*)commandPanel->GetChildren().GetFirst()->GetData();
    wxFont headerFont = headerText->GetFont();
    headerFont.SetPointSize(12);  // Tăng size
    headerFont.SetWeight(wxFONTWEIGHT_BOLD);  // Đậm hơn
    headerText->SetFont(headerFont);
    headerText->SetForegroundColour(wxColour(240, 240, 240));  // Màu sáng hơn

    // Thêm clear button
    wxButton* clearButton = createModernButton(commandPanel, 1003, "Clear History",
        wxColour(80, 80, 80));
    commandContentSizer->Add(clearButton, 0, wxALL, 10);

    // Enhanced ListView với font tốt hơn
    logList = new wxListCtrl(commandPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, 200),
        wxLC_REPORT | wxLC_SINGLE_SEL);
    logList->SetBackgroundColour(wxColour(45, 45, 45));
    logList->SetForegroundColour(wxColour(240, 240, 240));  // Màu chữ sáng hơn

    // Cải thiện font cho list
    wxFont listFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    logList->SetFont(listFont);

    // Style các cột với font tốt hơn
    logList->InsertColumn(0, "Time", wxLIST_FORMAT_LEFT, 120);
    logList->InsertColumn(1, "Command", wxLIST_FORMAT_LEFT, 400);
    logList->InsertColumn(2, "Status", wxLIST_FORMAT_LEFT, 100);

    // Cải thiện style cho headers
    wxListItem col;
    col.SetBackgroundColour(wxColour(60, 60, 60));
    col.SetTextColour(wxColour(240, 240, 240));  // Màu chữ sáng hơn
    wxFont headerFont2 = listFont;
    headerFont2.SetWeight(wxFONTWEIGHT_BOLD);
    headerFont2.SetPointSize(10);  // Size phù hợp cho header
    col.SetFont(headerFont2);

    for (int i = 0; i < 3; i++) {
        logList->SetColumn(i, col);
    }

    commandContentSizer->Add(logList, 1, wxEXPAND | wxALL, 10);

    // Server Messages Log Section
    wxPanel* messagePanel = createSection(panel, "Server Messages");
    wxBoxSizer* messageContentSizer = dynamic_cast<wxBoxSizer*>(messagePanel->GetSizer());

    messageLog = new wxTextCtrl(messagePanel, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxSize(-1, 200),
        wxTE_MULTILINE | wxTE_RICH2 | wxTE_READONLY | wxBORDER_NONE);
    messageLog->SetBackgroundColour(wxColour(35, 35, 35));
    messageLog->SetForegroundColour(wxColour(255, 255, 255));

    wxFont messageFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    messageLog->SetFont(messageFont);

    messageContentSizer->Add(messageLog, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);

    // Add all sections to main sizer with proper spacing
    mainSizer->Add(controlPanel, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(commandPanel, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(messagePanel, 1, wxEXPAND | wxALL, 10);

    panel->SetSizer(mainSizer);

    server = nullptr;
    serverThread = nullptr;
    isRunning = false;

    Centre();
}

void ServerFrame::OnButtonHover(wxMouseEvent& event)
{
    wxButton* button = static_cast<wxButton*>(event.GetEventObject());
    button->SetBackgroundColour(button->GetBackgroundColour());
    button->Refresh();
}

void ServerFrame::OnButtonLeave(wxMouseEvent& event)
{
    wxButton* button = static_cast<wxButton*>(event.GetEventObject());
    button->SetBackgroundColour(button->GetBackgroundColour());
    button->Refresh();
}
ServerFrame::~ServerFrame() {
    StopServer();
}

void ServerFrame::OnStart(wxCommandEvent& event) {
    if (!isRunning) {
        StartServer();
    }
}

void ServerFrame::OnStop(wxCommandEvent& event) {
    if (isRunning) {
        StopServer();
    }
}

void ServerFrame::OnClose(wxCloseEvent& event) {
    StopServer();
    event.Skip();
}


void ServerFrame::OnLogItemDblClick(wxListEvent& event) {
    long index = event.GetIndex();
    if (index >= 0 && static_cast<size_t>(index) < logEntries.size()) {
        const LogEntry& entry = logEntries[index];

        // Get the command text
        wxString commandText = logList->GetItemText(index, 1);

        if (entry.isVideo && !entry.imageData.empty()) {
            // Show video dialog
            VideoDialog dialog(this, "Video - " + commandText, entry.imageData);
            dialog.ShowModal();
        }
        else if (entry.isImage && !entry.imageData.empty()) {
            // Show image dialog
            ImageDialog dialog(this, "Image - " + commandText, entry.imageData);
            dialog.ShowModal();
        }
        else if (!entry.content.IsEmpty()) {
            // Show content dialog for text content
            ContentDialog dialog(this, "Command Details - " + commandText, entry.content);
            dialog.ShowModal();
        }
        else {
            ContentDialog dialog(this, "Command Details - " + commandText,
                "No content available for this command.");
            dialog.ShowModal();
        }
    }
}

void ServerFrame::StartServer() {
    server = new SocketServer(DEFAULT_PORT);

    if (!server->initialize()) {
        LogMessage("Failed to initialize Winsock", "", false);
        return;
    }

    if (!server->createListener()) {
        LogMessage("Failed to create listening socket", "", false);
        delete server;
        server = nullptr;
        return;
    }

    isRunning = true;
    statusText->SetLabel("ON");
    statusText->SetForegroundColour(*wxGREEN);
    startButton->Disable();
    startButton->SetBackgroundColour(wxColour(46, 204, 113).ChangeLightness(60)); // Tối hơn khi disable
    
    stopButton->Enable();
    stopButton->SetBackgroundColour(wxColour(231, 76, 60)); // Khôi phục màu gốc khi enable

    serverThread = new std::thread(&ServerFrame::ServerLoop, this);
}

void ServerFrame::StopServer() {
    if (isRunning) {
        isRunning = false;

        if (server) {
            server->closeClientConnection();
            delete server;
            server = nullptr;
        }

        if (serverThread) {
            serverThread->join();
            delete serverThread;
            serverThread = nullptr;
        }

        statusText->SetLabel("OFF");
        statusText->SetForegroundColour(*wxRED);
        startButton->Enable();
        startButton->SetBackgroundColour(wxColour(46, 204, 113)); // Khôi phục màu gốc khi enable

        stopButton->Disable();
        stopButton->SetBackgroundColour(wxColour(231, 76, 60).ChangeLightness(60)); // Tối hơn khi disable
    }
}

void ServerFrame::ServerLoop() {
    while (isRunning) {
        if (!server->acceptConnection()) {
            continue;
        }

        LogMessage("New client connected", "", false);

        while (isRunning) {
            string command = server->receiveMessage();

            if (!isRunning) break;

            if (command.empty()) {
                LogMessage("Client disconnected", "", false);
                break;
            }

            if (command.substr(0, 10) == "save_path:") {
                string path = command.substr(10);
                // Tìm và c?p nh?t LogEntry cu?i cùng v?i ???ng d?n file
                if (!logEntries.empty()) {
                    logEntries.back().savedPath = wxString::FromUTF8(path);
                }
                continue;
            }

            string response;
            vector<BYTE> imageData;

            // Log command t? client
            LogMessage(command, "", true);

            if (command == "list::app") {
                response = cmd.Applist();
                server->sendMessage(response);
                LogMessage("Sent application list", response, false);

                if (!logEntries.empty()) {
                    logEntries.back().content = response;
                }
            }
            else if (command == "list::service") {
                response = cmd.Listservice();
                server->sendMessage(response);
                LogMessage("Sent service list", response, false);

                if (!logEntries.empty()) {
                    logEntries.back().content = response;
                }
            }
            else if (command == "list::process") {
                response = cmd.Listprocess();
                server->sendMessage(response);
                LogMessage("Sent process list", response, false);

                if (!logEntries.empty()) {
                    logEntries.back().content = response;
                }
            }
            else if (command == "help::cmd") {
                response = cmd.help();
                server->sendMessage(response);
                LogMessage("Sent help information", response, false);

                if (!logEntries.empty()) {
                    logEntries.back().content = response;
                }
            }
            else if (command == "screenshot::capture") {
                int width, height;
                imageData = cmd.captureScreenWithGDIPlus(width, height);
                cmd.sendImage(server->getClientSocket(), imageData);
                LogMessage("Sent screenshot", "Screenshot taken", false);

                if (!logEntries.empty()) {
                    logEntries.back().imageData = imageData;
                    logEntries.back().isImage = true;
                }
            }
            else if (command == "system::shutdown") {
                LogMessage("Executing shutdown command", "", false);
                cmd.shutdownComputer();
            }
            else if (command == "camera::open") {
                cmd.openCamera();
                Sleep(2000);
                int width, height;
                imageData = cmd.captureScreenWithGDIPlus(width, height);
                cmd.sendImage(server->getClientSocket(), imageData);
                LogMessage("Camera capture taken", "", false);

                if (!logEntries.empty()) {
                    logEntries.back().imageData = imageData;
                    logEntries.back().isImage = true;
                }
            }
            else if (command == "camera::close") {
                cmd.closeCamera();
                LogMessage("Camera closed", "", false);
            }
            else if (command == "system::restart") {
                cmd.restartComputer();
                LogMessage("Executing restart command", "", false);
            }
            else if (command == "system::lock") {
                cmd.lockScreen();
                LogMessage("Executing lock screen command", "", false);
            }
            else if (command.substr(0, 10) == "app::start") {
                string appName = command.substr(11);  
                if (appName.find(".exe") == string::npos) {
                    appName += ".exe";
                }
                cmd.startApplication(appName);
                LogMessage("Starting application: " + appName, "", false);
            }
            else if (command.substr(0, 9) == "app::stop") {
                string appName = command.substr(10);   
                if (appName.find(".exe") == string::npos) {
                    appName += ".exe";
                }
                cmd.stopApplication(appName);
                LogMessage("Stopping application: " + appName, "", false);
            }
            else if (command.substr(0, 9) == "file::get") {
                string filepath = command.substr(10);
                cmd.handleGetFile(server->getClientSocket(), filepath);
                LogMessage("Sent file: " + filepath, "", false);
            }
            else if (command.substr(0, 12) == "file::delete") {
                string filepath = command.substr(13); 
                cmd.handleDeleteFile(server->getClientSocket(), filepath);
                LogMessage("Deleted file: " + filepath, "", false);
            }
            else if (command.substr(0, 14) == "camera::record") {
                try {
                    string durationStr = command.substr(14);
                    int seconds = std::stoi(durationStr);

                    if (seconds <= 0 || seconds > 300) {
                        LogMessage("Error: Invalid recording duration", "", false);
                        continue;
                    }

                    LogMessage("Opening camera and starting recording...", "", false);

                    // Gọi hàm record từ Command class (đã bao gồm việc mở/đóng camera)
                    vector<BYTE> videoData = cmd.recordVideo(seconds);

                    // Gửi dữ liệu về client
                    cmd.sendImage(server->getClientSocket(), videoData);

                    LogMessage("Video recording completed and sent", "", false);

                    if (!logEntries.empty()) {
                        logEntries.back().imageData = videoData;
                        logEntries.back().isVideo = true;  // Set the video flag
                        logEntries.back().isImage = false; // Make sure image flag is false
                        logEntries.back().content = "Video recording: " + to_string(videoData.size()) + " bytes";
                    }
                }
                catch (const std::exception& e) {
                    cmd.closeCamera(); // Ensure camera is closed in case of error
                    LogMessage("Error in video recording: " + string(e.what()), "", false);
                }
                }
            
        }

        server->closeClientConnection();
    }
}

void ServerFrame::LogMessage(const wxString& message, const wxString& details, bool isCommand) {
    LogEntry* entry = new LogEntry{ message, details, isCommand };
    entry->isImage = false;

    wxDateTime now = wxDateTime::Now();
    wxString timestamp = now.FormatTime();

    // Xác định kiểu message và style
    wxTextAttr style;
    wxString prefix;
    wxString source = isCommand ? "[CLIENT] " : "[SERVER] ";
    wxString loweredMsg = message.Lower();

    if (loweredMsg.Contains("error") || loweredMsg.Contains("failed")) {
        style.SetTextColour(wxColour(255, 99, 71));  // Đỏ
        prefix = "ERROR";
    }
    else if (loweredMsg.Contains("success") || loweredMsg.Contains("sent") || loweredMsg.Contains("deleted") || 
        loweredMsg.Contains("starting") ||
        loweredMsg.Contains("complete") || loweredMsg.Contains("saved")) {
        style.SetTextColour(wxColour(46, 204, 113));  // Xanh lá
        prefix = "SUCCESS";
    }
    else if (loweredMsg.Contains("::")) {
        style.SetTextColour(wxColour(135, 206, 235));
        prefix = "COMMAND";
    }    
    else if (loweredMsg.Contains("warning")) {
        style.SetTextColour(wxColour(241, 196, 15));  // Vàng
        prefix = "WARNING";
    }
    else {
        style.SetTextColour(wxColour(189, 195, 199)); // Xám nhạt
        prefix = "INFO";
    }

    // Timestamp style
    wxTextAttr timestampStyle;
    timestampStyle.SetTextColour(wxColour(128, 128, 128));
    timestampStyle.SetFont(messageLog->GetFont());
    messageLog->SetDefaultStyle(timestampStyle);
    messageLog->AppendText("[" + timestamp + "] ");

    // Prefix style với font đậm
    wxFont boldFont = messageLog->GetFont();
    boldFont.SetWeight(wxFONTWEIGHT_BOLD);
    style.SetFont(boldFont);
    messageLog->SetDefaultStyle(style);
    messageLog->AppendText("[" + prefix + "] " + source);

    // Message với font thường
    wxFont normalFont = messageLog->GetFont();
    normalFont.SetWeight(wxFONTWEIGHT_NORMAL);
    style.SetFont(normalFont);
    messageLog->SetDefaultStyle(style);
    messageLog->AppendText(message + "\n");

    messageLog->ShowPosition(messageLog->GetLastPosition());

    if (isCommand) {
        wxCommandEvent* event = new wxCommandEvent(wxEVT_SERVER_LOG);
        event->SetClientData(entry);
        wxQueueEvent(this, event);
    }
}

void ServerFrame::OnClearHistory(wxCommandEvent& event)
{
    if (wxMessageBox("Are you sure you want to clear the command history?",
        "Confirm Clear", wxYES_NO | wxICON_QUESTION) == wxYES) {
        logList->DeleteAllItems();
        logEntries.clear();
    }
}

void ServerFrame::FormatCommand(wxString& command) {
    // Convert to uppercase
    if (command.StartsWith("list::")) {
        command = "LIST " + command.Mid(6).Upper();
    }
    else if (command == "screenshot::capture") {
        command = "CAPTURE SCREEN";
    }
    else if (command == "system::shutdown") {
        command = "SYSTEM SHUTDOWN";
    }
    else if (command == "system::restart") {
        command = "SYSTEM RESTART";
    }
    else if (command == "system::lock") {
        command = "LOCK SCREEN";
    }
    else if (command.StartsWith("app::start")) {
        command = "START APP: " + command.Mid(10).Upper();
    }
    else if (command.StartsWith("app::stop")) {
        command = "STOP APP: " + command.Mid(9).Upper();
    }
    else if (command == "camera::open") {
        command = "OPEN CAMERA";
    }
    else if (command == "camera::close") {
        command = "CLOSE CAMERA";
    }
    else if (command.StartsWith("camera::record")) {
        command = "RECORD: " + command.Mid(14).Upper() + " SECONDS";
    }
    else if (command == "help::cmd") {
        command = "HELP COMMAND";
    }
    else if (command.StartsWith("file::get")) {
        command = "GET FILE: " + command.Mid(9).Upper();
    }
    else if (command.StartsWith("file::delete")) {
        command = "DELETE FILE: " + command.Mid(12).Upper();
    }
}

void ServerFrame::OnLogEvent(wxCommandEvent& event)
{
    LogEntry* entry = static_cast<LogEntry*>(event.GetClientData());
    if (!entry) return;

    if (entry->isCommand) {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::ostringstream timeStr;
        timeStr << std::put_time(&tm, "%H:%M:%S");

        // Format command before displaying
        wxString formattedCommand = entry->message;
        FormatCommand(formattedCommand);

        long itemIndex = logList->InsertItem(logList->GetItemCount(), timeStr.str());
        logList->SetItem(itemIndex, 1, formattedCommand);

        // Thêm status và màu sắc
        wxString status = "Executed";
        wxColour itemColor = wxColour(46, 204, 113); // Màu xanh mặc định

        if (entry->message.StartsWith("error") || entry->message.StartsWith("Error") || entry->message.StartsWith("ERROR") || entry->message.StartsWith("INVALID")) {
            status = "Failed";
            itemColor = wxColour(231, 76, 60); // Màu đỏ cho lỗi
        }

        logList->SetItem(itemIndex, 2, status);
        logList->SetItemTextColour(itemIndex, itemColor);

        // Tự động cuộn đến item mới
        logList->EnsureVisible(itemIndex);

        logEntries.push_back(*entry);
    }
    else {
        messageLog->AppendText(entry->message + "\n");
    }

    delete entry;
}