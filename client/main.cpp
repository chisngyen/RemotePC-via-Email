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
#include "Login_GUI.h"

#pragma comment (lib, "Ws2_32.lib")

class MyApp : public wxApp {
public:
    virtual bool OnInit() {
        // Create and show the login window first
        LoginFrame* loginFrame = new LoginFrame("Gmail Login");
        loginFrame->Show(true);
        SetTopWindow(loginFrame);
        return true;
    }
};

// Implement the wxApp
wxIMPLEMENT_APP(MyApp);
