#include "TokenManager.h"

TokenManager::TokenManager() {
    loadTokens();
}

void TokenManager::loadTokens() {
    std::ifstream file(TOKEN_FILE);
    if (file.is_open()) {
        Json::Value root;
        Json::Reader reader;
        if (reader.parse(file, root)) {
            Json::Value::Members members = root.getMemberNames();
            for (const auto& gmail : members) {
                tokenStore[gmail] = root[gmail].asString();
            }
        }
        file.close();
    }
}

void TokenManager::saveTokens() {
    Json::Value root;
    for (const auto& pair : tokenStore) {
        root[pair.first] = pair.second;
    }

    std::ofstream file(TOKEN_FILE);
    if (file.is_open()) {
        Json::StyledWriter writer;
        file << writer.write(root);
        file.close();
    }
}

void TokenManager::saveRefreshToken(const std::string& gmail, const std::string& refreshToken) {
    tokenStore[gmail] = refreshToken;
    saveTokens();
}

std::string TokenManager::getRefreshToken(const std::string& gmail) {
    auto it = tokenStore.find(gmail);
    return (it != tokenStore.end()) ? it->second : "";
}

bool TokenManager::isTokenValid(const std::string& gmail, GoogleOAuth* oauth) {
    auto refreshToken = getRefreshToken(gmail);
    if (refreshToken.empty()) {
        return false;
    }

    // Try to get an access token using the refresh token
    std::string accessToken = oauth->getAccessToken(refreshToken);
    return !accessToken.empty();
}

void TokenManager::removeToken(const std::string& gmail) {
    tokenStore.erase(gmail);
    saveTokens();
}