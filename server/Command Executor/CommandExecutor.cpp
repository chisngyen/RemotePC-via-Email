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
    helps += " 11. Start service: service::start [service_name]\n";
    helps += " 12. Stop service: service::stop [service_name]\n";
    helps += " 13. Shut down computer: system::shutdown\n";
    helps += " 14. Restart computer: system::restart\n";
    helps += " 15. Lock screen: system::lock\n";
    helps += " 16. Help: help::cmd\n";
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

    // Initialize camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        throw std::runtime_error("Error: Could not open camera");
    }

    // Get camera properties
    double fps = cap.get(cv::CAP_PROP_FPS);
    if (fps <= 0) fps = 30.0;
    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

    std::cout << "Camera settings:" << std::endl;
    std::cout << "- FPS: " << fps << std::endl;
    std::cout << "- Resolution: " << frame_width << "x" << frame_height << std::endl;

    // Create named window for preview
    cv::namedWindow("Camera Preview", cv::WINDOW_AUTOSIZE);
    // Move window to center of screen
    cv::moveWindow("Camera Preview",
        (GetSystemMetrics(SM_CXSCREEN) - frame_width) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - frame_height) / 2);

    HWND hwnd = FindWindowA(nullptr, "Camera Preview");
    if (!hwnd) {
        throw std::runtime_error("Failed to find OpenCV window");
    }

    // Đặt cửa sổ luôn ở trên cùng
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TOPMOST);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // Create memory buffer to store frames
    std::vector<cv::Mat> frames;
    auto start_time = std::chrono::steady_clock::now();
    int frameCount = 0;

    std::cout << "Recording in progress..." << std::endl;

    try {
        while (true) {
            cv::Mat frame;
            cap >> frame;

            if (frame.empty()) {
                throw std::runtime_error("Failed to capture frame");
            }

            // Tính thời gian còn lại
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            int remaining_time = std::max(0, seconds - static_cast<int>(elapsed));

            // Vẽ số giây đếm ngược lên khung hình
            std::string countdown_text = "Time left: " + std::to_string(remaining_time) + "s";
            cv::putText(frame, countdown_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

            // Save frame and show preview
            frames.push_back(frame.clone());
            cv::imshow("Camera Preview", frame);
            frameCount++;

            if (frameCount % 30 == 0) {
                std::cout << "Recording: " << elapsed << "/" << seconds << " seconds" << std::endl;
            }

            // Kiểm tra hoàn thành hoặc nhấn phím ESC để dừng
            if (elapsed >= seconds || cv::waitKey(1) == 27) { // 27 là phím ESC
                break;
            }

            // Cho phép UI cập nhật
            wxYield();
        }
    }

    catch (const std::exception& e) {
        cap.release();
        cv::destroyWindow("Camera Preview");
        throw;
    }

    // Release camera and close preview
    cap.release();
    cv::destroyWindow("Camera Preview");
    std::cout << "Recording completed: " << frameCount << " frames captured" << std::endl;

    // Get AppData path for temporary storage
    PWSTR appDataPath = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appDataPath))) {
        throw std::runtime_error("Failed to get AppData path");
    }

    // Create temporary file path
    std::wstring widePath(appDataPath);
    std::string appDataStr(widePath.begin(), widePath.end());
    std::string appDir = appDataStr + "\\EmailPCControl\\temp";
    CreateDirectoryA((appDataStr + "\\EmailPCControl").c_str(), nullptr);
    CreateDirectoryA(appDir.c_str(), nullptr);
    std::string videoPath = appDir + "\\temp_recording.avi";
    CoTaskMemFree(appDataPath);

    // Save frames to video file
    cv::VideoWriter video(videoPath,
        cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
        fps,
        cv::Size(frame_width, frame_height));

    if (!video.isOpened()) {
        throw std::runtime_error("Failed to create video writer");
    }

    for (const auto& frame : frames) {
        video.write(frame);
    }
    video.release();

    // Read video file into buffer
    std::vector<BYTE> buffer;
    std::ifstream videoFile(videoPath, std::ios::binary | std::ios::ate);

    if (!videoFile) {
        throw std::runtime_error("Failed to read recorded video file");
    }

    std::streamsize videoSize = videoFile.tellg();
    videoFile.seekg(0, std::ios::beg);
    buffer.resize(videoSize);

    if (!videoFile.read(reinterpret_cast<char*>(buffer.data()), videoSize)) {
        videoFile.close();
        throw std::runtime_error("Error reading video data");
    }

    videoFile.close();

    // Clean up temporary file
    try {
        if (remove(videoPath.c_str()) != 0) {
            std::cout << "Warning: Could not delete temporary file" << std::endl;
        }
        else {
            std::cout << "Successfully cleaned up temporary file" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cout << "Warning: File cleanup failed: " << e.what() << std::endl;
    }

    std::cout << "Video processing completed. Buffer size: " << buffer.size() << " bytes" << std::endl;
    return buffer;
}

void Command::startService(const string& serviceName) {
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
        DWORD error = GetLastError();
        string response = "Failed to open Service Control Manager. Error code: " + to_string(error);
        cout << response << endl;
        return;
    }

    SC_HANDLE schService = OpenServiceA(
        schSCManager,
        serviceName.c_str(),
        SERVICE_START | SERVICE_QUERY_STATUS
    );

    if (schService == NULL) {
        DWORD error = GetLastError();
        string response = "Failed to open service " + serviceName + ". Error code: " + to_string(error);
        cout << response << endl;
        CloseServiceHandle(schSCManager);
        return;
    }

    // Check current service status
    SERVICE_STATUS_PROCESS ssp;
    DWORD bytesNeeded;
    if (QueryServiceStatusEx(
        schService,
        SC_STATUS_PROCESS_INFO,
        (LPBYTE)&ssp,
        sizeof(SERVICE_STATUS_PROCESS),
        &bytesNeeded)) {

        if (ssp.dwCurrentState != SERVICE_STOPPED) {
            cout << "Service " << serviceName << " is already running or pending." << endl;
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return;
        }
    }

    // Try to start the service
    if (StartServiceA(schService, 0, NULL)) {
        cout << "Service " << serviceName << " start command sent successfully." << endl;

        // Wait for the service to start
        int attempts = 0;
        while (attempts < 10) {
            if (QueryServiceStatusEx(
                schService,
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssp,
                sizeof(SERVICE_STATUS_PROCESS),
                &bytesNeeded)) {

                if (ssp.dwCurrentState == SERVICE_RUNNING) {
                    cout << "Service " << serviceName << " started successfully." << endl;
                    break;
                }
                else if (ssp.dwCurrentState == SERVICE_STOPPED) {
                    cout << "Service " << serviceName << " failed to start." << endl;
                    break;
                }
            }
            Sleep(1000);  // Wait 1 second before checking again
            attempts++;
        }
    }
    else {
        DWORD error = GetLastError();
        string response = "Failed to start service " + serviceName + ". Error code: " + to_string(error);
        cout << response << endl;
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

void Command::stopService(const string& serviceName) {
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
        DWORD error = GetLastError();
        string response = "Failed to open Service Control Manager. Error code: " + to_string(error);
        cout << response << endl;
        return;
    }

    SC_HANDLE schService = OpenServiceA(
        schSCManager,
        serviceName.c_str(),
        SERVICE_STOP | SERVICE_QUERY_STATUS
    );

    if (schService == NULL) {
        DWORD error = GetLastError();
        string response = "Failed to open service " + serviceName + ". Error code: " + to_string(error);
        cout << response << endl;
        CloseServiceHandle(schSCManager);
        return;
    }

    // Check current service status
    SERVICE_STATUS_PROCESS ssp;
    DWORD bytesNeeded;
    if (QueryServiceStatusEx(
        schService,
        SC_STATUS_PROCESS_INFO,
        (LPBYTE)&ssp,
        sizeof(SERVICE_STATUS_PROCESS),
        &bytesNeeded)) {

        if (ssp.dwCurrentState == SERVICE_STOPPED) {
            cout << "Service " << serviceName << " is already stopped." << endl;
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return;
        }
    }

    // Try to stop the service
    SERVICE_STATUS status;
    if (ControlService(schService, SERVICE_CONTROL_STOP, &status)) {
        cout << "Service " << serviceName << " stop command sent successfully." << endl;

        // Wait for the service to stop
        int attempts = 0;
        while (attempts < 10) {
            if (QueryServiceStatusEx(
                schService,
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssp,
                sizeof(SERVICE_STATUS_PROCESS),
                &bytesNeeded)) {

                if (ssp.dwCurrentState == SERVICE_STOPPED) {
                    cout << "Service " << serviceName << " stopped successfully." << endl;
                    break;
                }
                else if (ssp.dwCurrentState == SERVICE_RUNNING) {
                    cout << "Service " << serviceName << " failed to stop." << endl;
                    break;
                }
            }
            Sleep(1000);  // Wait 1 second before checking again
            attempts++;
        }
    }
    else {
        DWORD error = GetLastError();
        string response = "Failed to stop service " + serviceName + ". Error code: " + to_string(error);
        cout << response << endl;
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}
