#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>
#include <sstream>
#include "GmailAPI.h"

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Định nghĩa hàm urlEncode của class GoogleOAuth
string GoogleOAuth::urlEncode(const string& str) {
    CURL* curl = curl_easy_init();
    char* encoded = curl_easy_escape(curl, str.c_str(), str.length());
    string result(encoded);
    curl_free(encoded);
    curl_easy_cleanup(curl);
    return result;
}

// Constructor
GoogleOAuth::GoogleOAuth(const string& client_id, const string& client_secret, const string& redirect_uri)
    : client_id(client_id), client_secret(client_secret), redirect_uri(redirect_uri) {}

// Định nghĩa getAuthUrl
string GoogleOAuth::getAuthUrl() {
    string url = auth_url + "?scope=" + urlEncode("https://mail.google.com") +
        "&access_type=offline" +
        "&response_type=code" +
        "&redirect_uri=" + urlEncode(redirect_uri) +
        "&client_id=" + urlEncode(client_id);
    return url;
}

// Định nghĩa getRefreshToken
string GoogleOAuth::getRefreshToken(const string& code) {
    CURL* curl = curl_easy_init();
    string response;

    if (curl) {
        string postData = "code=" + urlEncode(code) +
            "&client_id=" + urlEncode(client_id) +
            "&client_secret=" + urlEncode(client_secret) +
            "&redirect_uri=" + urlEncode(redirect_uri) +
            "&grant_type=authorization_code";

        curl_easy_setopt(curl, CURLOPT_URL, token_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            throw runtime_error("Failed to get refresh token: " + std::string(curl_easy_strerror(res)));
        }

        curl_easy_cleanup(curl);

        Json::Value root;
        Json::Reader reader;
        if (reader.parse(response, root)) {
            if (root.isMember("refresh_token")) {
                return root["refresh_token"].asString();
            }
        }
        throw runtime_error("Failed to parse refresh token from response: " + response);
    }
    throw runtime_error("Failed to initialize CURL");
}

// Định nghĩa getAccessToken
string GoogleOAuth::getAccessToken(const string& refresh_token) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        string postFields = "client_id=" + urlEncode(client_id) +
            "&client_secret=" + urlEncode(client_secret) +
            "&refresh_token=" + urlEncode(refresh_token) +
            "&grant_type=refresh_token";

        curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            return "";
        }
    }

    Json::Value root;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse(readBuffer, root);
    if (!parsingSuccessful) {
        cout << "Failed to parse the JSON" << endl;
        return "";
    }

    return root["access_token"].asString();
}