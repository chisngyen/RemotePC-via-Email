#pragma once
#include <string>
using namespace std;

class GoogleOAuth {
private:
    string client_id;
    string client_secret;
    string redirect_uri;
    const string auth_url = "https://accounts.google.com/o/oauth2/v2/auth";
    const string token_url = "https://accounts.google.com/o/oauth2/token";
    string urlEncode(const string& str);
public:
    GoogleOAuth(const string& client_id, const string& client_secret, const string& redirect_uri);
    string getAuthUrl();
    string getRefreshToken(const string& code);
    string getAccessToken(const string& refresh_token);
};
