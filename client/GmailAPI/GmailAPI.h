#pragma once
#include <string>
#include <windows.h>
#include <UIAutomation.h>
#include <string>

class GoogleOAuth {
private:
    const std::string client_id;
    const std::string client_secret;
    const std::string redirect_uri;
    const std::string auth_url = "https://accounts.google.com/o/oauth2/v2/auth";
    const std::string token_url = "https://accounts.google.com/o/oauth2/token";

    std::string urlEncode(const std::string& str);

public:
    GoogleOAuth(const std::string& client_id, const std::string& client_secret, const std::string& redirect_uri);
    std::string getAuthUrl();
    std::string getRefreshToken(const std::string& code);
    std::string getAccessToken(const std::string& refresh_token);
};

class GmailUIAutomation {
public:
    GmailUIAutomation();
    ~GmailUIAutomation();
    bool automateGmailAuth(const std::string& gmail = "");

private:
    IUIAutomation* automation;
    bool findAndClickButton(const wchar_t* buttonText);
    bool findAndClickButtonByAriaLabel(const wchar_t* label);
    bool waitForElement(const wchar_t* text, int timeoutMs = 5000);
};
