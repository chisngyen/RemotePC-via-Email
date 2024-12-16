#include "CommandExecutor.h"

Command::Command() {
    InitializeGDIPlus(gdiplusToken);
}

Command::~Command() {
    ShutdownGDIPlus(gdiplusToken);
}

void Command::InitializeGDIPlus(ULONG_PTR& gdiplusToken) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
}

void Command::ShutdownGDIPlus(ULONG_PTR gdiplusToken) {
    GdiplusShutdown(gdiplusToken);
}

void Command::SendMessages(SOCKET clientSocket, const std::string& message) {
    int messageSize = message.size();
    send(clientSocket, (char*)&messageSize, sizeof(messageSize), 0);
    send(clientSocket, message.c_str(), messageSize, 0);
}

void Command::sendFile(SOCKET clientSocket, const std::string& fileName) {
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cout << "[ERROR] Unable to open file: " << fileName << std::endl;
        SendMessages(clientSocket, "Unable to open file.");
        return;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::cout << "[INFO] Sending file size: " << fileSize << " bytes" << std::endl;
    send(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);

    char buffer[DEFAULT_BUFLEN];
    std::streamsize totalSent = 0;

    while (totalSent < fileSize) {
        file.read(buffer, DEFAULT_BUFLEN);
        int bytesRead = file.gcount();

        int bytesSent = 0;
        while (bytesSent < bytesRead) {
            int result = send(clientSocket, buffer + bytesSent, bytesRead - bytesSent, 0);
            if (result == SOCKET_ERROR) {
                std::cout << "[ERROR] Failed to send file. WSA Error: " << WSAGetLastError() << std::endl;
                SendMessages(clientSocket, "Error occurred while sending file.");
                file.close();
                return;
            }
            bytesSent += result;
        }
        totalSent += bytesSent;
        std::cout << "[INFO] Progress: " << totalSent << "/" << fileSize << " bytes sent" << std::endl;
    }

    file.close();
    std::cout << "[SUCCESS] File sent successfully: " << fileName << std::endl;
    SendMessages(clientSocket, "File sent successfully.");
}

void Command::handleGetFile(SOCKET clientSocket, const std::string& fileName) {
    std::ifstream checkFile(fileName);
    if (!checkFile.good()) {
        std::cout << "[ERROR] File does not exist: " << fileName << std::endl;
        SendMessages(clientSocket, "File not found.");
        return;
    }
    checkFile.close();

    std::cout << "[INFO] Processing file request: " << fileName << std::endl;
    sendFile(clientSocket, fileName);
}

void Command::handleDeleteFile(SOCKET clientSocket, const string& fileName) {
    std::cout << "[INFO] Attempting to delete file: " << fileName << std::endl;

    std::ifstream checkFile(fileName);
    if (!checkFile.good()) {
        std::cout << "[ERROR] File does not exist: " << fileName << std::endl;
        SendMessages(clientSocket, "File not found.");
        return;
    }
    checkFile.close();

    if (std::remove(fileName.c_str()) == 0) {
        std::cout << "[SUCCESS] File deleted successfully: " << fileName << std::endl;
        SendMessages(clientSocket, "File deleted successfully.");
        return;
    }

    if (DeleteFileA(fileName.c_str())) {
        std::cout << "[SUCCESS] File deleted successfully using Windows API: " << fileName << std::endl;
        SendMessages(clientSocket, "File deleted successfully.");
    }
    else {
        DWORD error = GetLastError();
        std::cout << "[ERROR] Failed to delete file. Error code: " << error << std::endl;
        SendMessages(clientSocket, "Unable to delete file. Error code: " + std::to_string(error));
    }
}

bool Command::IsVisibleWindow(DWORD processID) {
    HWND hwnd = GetTopWindow(NULL);
    while (hwnd) {
        DWORD windowProcessID;
        GetWindowThreadProcessId(hwnd, &windowProcessID);

        if (windowProcessID == processID && IsWindowVisible(hwnd)) {
            return true;
        }
        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }
    return false;
}

vector<wstring> Command::GetRunningApplications() {
    set<wstring> applications;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                if (IsVisibleWindow(pe32.th32ProcessID)) {
                    applications.insert(wstring(pe32.szExeFile));
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    return vector<wstring>(applications.begin(), applications.end());
}

string Command::Applist() {
    string applist;
    vector<wstring> apps = GetRunningApplications();
    for (const auto& app : apps) {
        string narrowApp(app.begin(), app.end());
        applist += narrowApp + "\n";
    }
    return applist;
}

vector<wstring> Command::GetRunningServices() {
    vector<wstring> services;
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);

    if (scManager == NULL) {
        cout << "OpenSCManager failed: " << GetLastError() << endl;
        return services;
    }

    DWORD bytesNeeded = 0;
    DWORD servicesReturned = 0;
    DWORD resumeHandle = 0;
    ENUM_SERVICE_STATUS_PROCESS* pServices = NULL;

    if (!EnumServicesStatusEx(scManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
        SERVICE_ACTIVE, NULL, 0, &bytesNeeded, &servicesReturned, &resumeHandle, NULL)) {
        if (GetLastError() != ERROR_MORE_DATA) {
            cout << "EnumServicesStatusEx failed: " << GetLastError() << endl;
            CloseServiceHandle(scManager);
            return services;
        }
    }

    pServices = (ENUM_SERVICE_STATUS_PROCESS*)malloc(bytesNeeded);
    if (!pServices) {
        cout << "Memory allocation failed" << endl;
        CloseServiceHandle(scManager);
        return services;
    }

    if (!EnumServicesStatusEx(scManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
        SERVICE_ACTIVE, (LPBYTE)pServices, bytesNeeded, &bytesNeeded,
        &servicesReturned, &resumeHandle, NULL)) {
        cout << "EnumServicesStatusEx failed: " << GetLastError() << endl;
        free(pServices);
        CloseServiceHandle(scManager);
        return services;
    }

    for (DWORD i = 0; i < servicesReturned; i++) {
        services.push_back(pServices[i].lpServiceName);
    }

    free(pServices);
    CloseServiceHandle(scManager);
    return services;
}

string Command::Listservice() {
    vector<wstring> apps = GetRunningServices();
    string listservice;
    for (const auto& app : apps) {
        string narrowApp(app.begin(), app.end());
        listservice += narrowApp + "\n";
    }
    return listservice;
}

vector<pair<wstring, DWORD>> Command::GetAllRunningProcesses() {
    vector<pair<wstring, DWORD>> processes;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                processes.push_back(make_pair(wstring(pe32.szExeFile), pe32.th32ProcessID));
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }

    return processes;
}

string Command::Listprocess() {
    vector<pair<wstring, DWORD>> processes = GetAllRunningProcesses();
    ostringstream oss;

    oss << left << setw(40) << "Process Name" << "PID\n";
    oss << string(45, '-') << "\n";

    for (const auto& process : processes) {
        string narrowProcess(process.first.begin(), process.first.end());
        oss << left << setw(40) << narrowProcess << process.second << "\n";
    }

    return oss.str();
}

int Command::GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;

    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == nullptr) return -1;

    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }

    free(pImageCodecInfo);
    return -1;
}

vector<BYTE> Command::captureScreenWithGDIPlus(int& width, int& height) {
    HDC hScreenDC = GetDC(nullptr);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    SelectObject(hMemoryDC, hBitmap);

    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

    Bitmap bitmap(hBitmap, nullptr);
    CLSID clsid;
    GetEncoderClsid(L"image/png", &clsid);

    IStream* stream = nullptr;
    CreateStreamOnHGlobal(nullptr, TRUE, &stream);
    bitmap.Save(stream, &clsid, nullptr);

    STATSTG stats;
    stream->Stat(&stats, STATFLAG_DEFAULT);
    ULONG imageSize = stats.cbSize.LowPart;

    vector<BYTE> buffer(imageSize);
    LARGE_INTEGER liZero = {};
    stream->Seek(liZero, STREAM_SEEK_SET, nullptr);
    ULONG bytesRead = 0;
    stream->Read(buffer.data(), imageSize, &bytesRead);

    stream->Release();
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(nullptr, hScreenDC);

    return buffer;
}

void Command::sendImage(SOCKET clientSocket, const vector<BYTE>& image) {
    int imageSize = image.size();
    send(clientSocket, (char*)&imageSize, sizeof(imageSize), 0);

    int bytesSent = 0;
    while (bytesSent < imageSize) {
        int result = send(clientSocket, (char*)image.data() + bytesSent, imageSize - bytesSent, 0);
        if (result == SOCKET_ERROR) {
            cerr << "Error data.\n";
            return;
        }
        bytesSent += result;
    }

    cout << "Success send image with size: " << imageSize << " bytes\n";
}

void Command::openCamera() {
    system("start microsoft.windows.camera:");
}

void Command::closeCamera() {
    system("taskkill /IM WindowsCamera.exe /F");
}

void Command::shutdownComputer() {
    system("shutdown /s /t 0");
}

void Command::restartComputer() {
    system("shutdown /r /t 0");  
}

void Command::lockScreen() {
    LockWorkStation(); 
}

string Command::help() {
    string helps = "HELP\n";
    helps += "=====\n";
    helps += "Functions performed by the server: \n";
    helps += "  1. Check list applications: list::app\n";
    helps += "  2. Check list process: list::process\n";
    helps += "  3. Check list services: list::service\n";
    helps += "  4. Screenshot: screenshot::capture\n";
    helps += "  5. Select file: file::get [path_file]\n";
    helps += "  6. Delete file: file::delete [path_file]\n";
    helps += "  7. Open webcam: camera::open\n";
    helps += "  8. Close webcam: camera::close\n";
    helps += "  9. Start application: app::start [app_name]\n";
    helps += " 10. Stop application: app::stop [app_name]\n";
    helps += " 11. Shut down computer: system::shutdown\n";
    helps += " 12. Restart computer: system::restart\n";
    helps += " 13. Lock screen: system::lock\n";
    helps += " 14. Help: help::cmd\n";
    return helps;
}

void Command::startApplication(const string& appName) {
    SHELLEXECUTEINFOA sei = { 0 };
    sei.cbSize = sizeof(SHELLEXECUTEINFOA);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = "open";
    sei.lpFile = appName.c_str();
    sei.nShow = SW_SHOW;

    if (ShellExecuteExA(&sei)) {
        currentAppHandle = sei.hProcess;
        currentAppPID = GetProcessId(sei.hProcess);
        string response = "Application " + appName + " started successfully (PID: " + to_string(currentAppPID) + ")";
        cout << response << endl;
    }
    else {
        DWORD error = GetLastError();
        string response = "Failed to start " + appName + ". Error code: " + to_string(error);
        cout << response << endl;
    }
}

void Command::stopApplication(const string& appName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        cout << "Failed to create process snapshot" << endl;
        return;
    }

    PROCESSENTRY32W processEntry;
    processEntry.dwSize = sizeof(processEntry);

    bool found = false;
    if (Process32FirstW(snapshot, &processEntry)) {
        do {
            // Convert WCHAR array to string for comparison
            wstring wProcessName = processEntry.szExeFile;
            string processName(wProcessName.begin(), wProcessName.end());

            if (_stricmp(processName.c_str(), appName.c_str()) == 0) {
                HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, processEntry.th32ProcessID);
                if (processHandle != NULL) {
                    if (TerminateProcess(processHandle, 0)) {
                        found = true;
                        cout << "Successfully terminated " << appName << endl;
                    }
                    CloseHandle(processHandle);
                }
            }
        } while (Process32NextW(snapshot, &processEntry));
    }

    CloseHandle(snapshot);

    if (!found) {
        cout << "Could not find or terminate " << appName << endl;
    }
}

std::vector<BYTE> Command::recordVideo(int seconds) {
    std::cout << "Starting video recording..." << std::endl;

    // Mở camera
    cv::VideoCapture cap(0); // Mở camera mặc định
    if (!cap.isOpened()) {
        throw std::runtime_error("Error: Could not open the camera.");
    }

    // Lấy FPS và kích thước khung hình của camera
    double fps = cap.get(cv::CAP_PROP_FPS);
    if (fps <= 0) fps = 30.0; // Nếu không lấy được FPS thì set mặc định là 30
    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

    std::cout << "Camera FPS: " << fps << std::endl;
    std::cout << "Resolution: " << frame_width << "x" << frame_height << std::endl;

    // Tạo file lưu video
    cv::VideoWriter video("webcam_recording.avi",
        cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
        fps,
        cv::Size(frame_width, frame_height));

    if (!video.isOpened()) {
        cap.release();
        throw std::runtime_error("Error: Could not create video writer.");
    }

    std::cout << "Recording started..." << std::endl;

    auto start_time = std::chrono::steady_clock::now();

    // Ghi video trong vòng thời gian yêu cầu
    while (true) {
        cv::Mat frame;
        cap >> frame; // Đọc khung hình từ camera

        if (frame.empty()) {
            std::cout << "Error: Could not read frame from camera." << std::endl;
            break;
        }

        // Ghi frame vào file video
        video.write(frame);

        // Hiển thị video trực tiếp
        cv::imshow("Recording", frame);

        // Kiểm tra thời gian đã ghi
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();

        if (elapsed >= seconds || cv::waitKey(1) == 'q') { // Nếu đã đủ thời gian hoặc nhấn 'q' thì dừng
            break;
        }
    }

    std::cout << "\nRecording finished." << std::endl;

    // Giải phóng tài nguyên
    cap.release();
    video.release();
    cv::destroyAllWindows();

    // Kiểm tra file ghi
    std::ifstream checkFile("webcam_recording.avi", std::ios::binary | std::ios::ate);
    if (!checkFile) {
        throw std::runtime_error("Cannot open recorded file.");
    }

    std::streamsize fileSize = checkFile.tellg();
    std::cout << "Recorded file size: " << fileSize << " bytes" << std::endl;
    checkFile.close();

    // Đọc dữ liệu video vào buffer
    std::ifstream videoFile("webcam_recording.avi", std::ios::binary);
    std::vector<BYTE> buffer(std::istreambuf_iterator<char>(videoFile), {});
    videoFile.close();

    std::cout << "Successfully read " << buffer.size() << " bytes of video data" << std::endl;

    return buffer;
}
