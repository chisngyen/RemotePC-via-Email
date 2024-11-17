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
    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
        SendMessages(clientSocket, "Unable to open file.");
        return;
    }
    char buffer[DEFAULT_BUFLEN];
    while (!file.eof()) {
        file.read(buffer, DEFAULT_BUFLEN);
        int bytesRead = file.gcount();
        send(clientSocket, buffer, bytesRead, 0);
    }
    file.close();
}

void Command::handleViewFile(SOCKET clientSocket, const std::string& fileName) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        SendMessages(clientSocket, "Unable to open file.");
        return;
    }
    sendFile(clientSocket, fileName);
}

void Command::handleDeleteFile(SOCKET clientSocket, const string& fileName) {
    cout << fileName;
    if (DeleteFileA(fileName.c_str())) {
        SendMessages(clientSocket, "File successfully deleted.");
    }
    else {
        SendMessages(clientSocket, "Unable to delete file.");
    }
}

void Command::handleClientCommands(SOCKET clientSocket, string command) {
    string action, fileName;
    size_t spacePos = command.find(' ');
    if (spacePos != string::npos) {
        action = command.substr(0, spacePos);
        fileName = command.substr(spacePos + 1);
    }
    if (action == "view") {
        handleViewFile(clientSocket, fileName);
    }
    else if (action == "delete") {
        handleDeleteFile(clientSocket, fileName);
    }
    else {
        SendMessages(clientSocket, "Invalid command.");
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

string Command::help() {
    string helps = "HELP\n";
    helps += "=====\n";
    helps += "Functions performed by the server: \n";
    helps += "  1. Check list applications: list app\n";
    helps += "  2. Check list process: list process\n";
    helps += "  3. Check list services: list service\n";
    helps += "  4. Screenshot: screenshot\n";
    helps += "  5. Select file: view [name_file]\n";
    helps += "  6. Delete file: delete [name_file]\n";
    helps += "  7. Open webcam: open_cam\n";
    helps += "  8. Close wbcam: close_cam\n";
    helps += "  9. Shut down computer: shutdown\n";
    helps += " 10. Help: help\n";
    return helps;
}