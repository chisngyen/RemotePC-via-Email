#include <wx/wx.h>
#include "GUI.h"
#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <json/json.h>
#include <fstream>
#include <windows.h>
#include <chrono>
#include <algorithm>
#include <thread>
#include "utils.h"
#include "socket.h"
#include "handleMail.h"
#include "GmailAPI.h"

#pragma comment (lib, "Ws2_32.lib")

// Define the Application class
class MyApp : public wxApp {
public:
    virtual bool OnInit() {
        // Create the main application window
        MainFrame* frame = new MainFrame("Email Monitoring Application");

        // Show the frame
        frame->Show(true);

        // Set as the top window
        SetTopWindow(frame);

        return true;
    }
};

// Implement the wxApp
wxIMPLEMENT_APP(MyApp);

// If you need the old console-based main for testing, you can keep it commented:
/*
int main_console() {
    // Your original console-based main code here
    return 0;
}
*/