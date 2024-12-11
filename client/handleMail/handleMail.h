#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <curl/curl.h>
#include <json/json.h>
#include <chrono>
#include <fstream>
#include <regex>
using namespace std;

class EmailHandler {
public:
    struct EmailInfo {
        string subject;
        string from;
        string date;
        string content;
        string threadId;
    };

    // Constructor
    explicit EmailHandler(const string& token);

    // Public methods
    EmailInfo readNewestEmail();
    EmailInfo decodeEmailContent(const string& emailContent);
    bool sendReplyEmail(const string& to,
        const string& subject,
        const string& message_body,
        const string& thread_id,
        const string& attachment_path = "");

private:
    string access_token;
    string lastProcessedId;

    // Private helper methods
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* s);
    string extractEmail(const string& from);
    string formatDate(const string& date);
    string fetchEmailContent(const string& messageId);
};
