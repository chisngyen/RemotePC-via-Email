#pragma once
#include <string>
#include <map>
#include <json/json.h>
#include <fstream>
#include "GmailAPI.h"
#include <iostream>
#include <ShlObj.h>
#include <KnownFolders.h>

class TokenManager {
public:
    TokenManager();
    void saveRefreshToken(const std::string& gmail, const std::string& refreshToken);
    std::string getRefreshToken(const std::string& gmail);
    bool isTokenValid(const std::string& gmail, GoogleOAuth* oauth);
    void removeToken(const std::string& gmail);

private:
    std::string getTokenFilePath() {
        PWSTR appDataPath = nullptr;
        std::string tokenPath;

        // Get the AppData\Roaming path
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appDataPath))) {
            std::wstring widePath(appDataPath);
            std::string path(widePath.begin(), widePath.end());

            // Create application directory if it doesn't exist
            std::string appDir = path + "\\EmailPCControl";
            CreateDirectoryA(appDir.c_str(), nullptr);

            // Set the full path for tokens.json
            tokenPath = appDir + "\\tokens.json";
            CoTaskMemFree(appDataPath);
        }

        return tokenPath;
    }

    std::map<std::string, std::string> tokenStore;
    std::string TOKEN_FILE;
    void loadTokens();
    void saveTokens();

    
};