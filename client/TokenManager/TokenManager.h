#pragma once
#include <string>
#include <map>
#include <json/json.h>
#include <fstream>
#include "GmailAPI.h"
#include <iostream>

class TokenManager {
public:
    TokenManager();
    void saveRefreshToken(const std::string& gmail, const std::string& refreshToken);
    std::string getRefreshToken(const std::string& gmail);
    bool isTokenValid(const std::string& gmail, GoogleOAuth* oauth);
    void removeToken(const std::string& gmail);

private:
    std::map<std::string, std::string> tokenStore;
    const std::string TOKEN_FILE = "tokens.json";
    void loadTokens();
    void saveTokens();
};