#include "HandleMail.h"
#include "utils.h" // Cho base64_encode và base64_decode
#include <iostream>

using namespace std;

EmailHandler::EmailHandler(const string& token) : access_token(token) {}

size_t EmailHandler::WriteCallback(void* contents, size_t size, size_t nmemb, string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
    }
    catch (bad_alloc& e) {
        return 0;
    }
    return newLength;
}

string EmailHandler::extractEmail(const string& from) {
    regex emailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    smatch match;
    if (regex_search(from, match, emailRegex)) {
        return match.str();
    }
    return from;
}

string EmailHandler::formatDate(const string& date) {
    istringstream iss(date);
    string dayOfWeek, month, dateNum, time, year;

    iss >> dayOfWeek >> month >> dateNum >> time >> year;

    if (dayOfWeek.back() == ',') {
        dayOfWeek.pop_back();
    }

    return dayOfWeek + ", " + dateNum + " " + month + " " + year + " " + time;
}

string EmailHandler::fetchEmailContent(const string& messageId) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        string url = "https://www.googleapis.com/gmail/v1/users/me/messages/" + messageId + "?format=full";
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            return "";
        }
    }
    else {
        cerr << "Failed to initialize cURL." << endl;
        return "";
    }

    return readBuffer;
}

EmailHandler::EmailInfo EmailHandler::decodeEmailContent(const string& emailContent) {
    Json::Value emailDetail;
    Json::CharReaderBuilder readerBuilder;
    istringstream emailStream(emailContent);
    string errs;
    EmailInfo info;

    if (!Json::parseFromStream(readerBuilder, emailStream, &emailDetail, &errs)) {
        cerr << "Cannot parse email JSON content: " << errs << endl;
        return info;
    }

    // Extract headers
    const Json::Value& headers = emailDetail["payload"]["headers"];
    for (const auto& header : headers) {
        string name = header["name"].asString();
        string value = header["value"].asString();

        if (name == "Subject") info.subject = value;
        else if (name == "From") info.from = extractEmail(value);
        else if (name == "Date") info.date = formatDate(value);
    }

    // Extract body
    string body;
    if (emailDetail["payload"].isMember("parts")) {
        for (const auto& part : emailDetail["payload"]["parts"]) {
            if (part["mimeType"].asString() == "text/plain") {
                body = part["body"]["data"].asString();
                break;
            }
        }
    }
    else if (emailDetail["payload"]["body"].isMember("data")) {
        body = emailDetail["payload"]["body"]["data"].asString();
    }

    if (!body.empty()) {
        try {
            info.content = base64_decode(body);
        }
        catch (const std::exception& e) {
            cerr << "Error decoding base64: " << e.what() << endl;
        }
    }

    info.threadId = emailDetail["threadId"].asString();
    return info;
}

EmailHandler::EmailInfo EmailHandler::readNewestEmail() {
    CURL* curl;
    CURLcode res;
    string readBuffer;
    EmailInfo emptyInfo;

    curl = curl_easy_init();
    if (curl) {
        string url = "https://www.googleapis.com/gmail/v1/users/me/messages?q=in:inbox&maxResults=1";
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            return emptyInfo;
        }
    }

    Json::Value root;
    Json::CharReaderBuilder reader;
    string errs;
    istringstream iss(readBuffer);
    if (!Json::parseFromStream(reader, iss, &root, &errs)) {
        cerr << "Failed to parse the JSON" << endl;
        return emptyInfo;
    }

    const Json::Value& messages = root["messages"];
    if (messages.size() > 0) {
        string messageId = messages[0]["id"].asString();
        if (messageId != lastProcessedId) {
            lastProcessedId = messageId;
            string emailContent = fetchEmailContent(messageId);

            if (!emailContent.empty()) {
                return decodeEmailContent(emailContent);
            }
        }
    }

    return emptyInfo;
}

bool EmailHandler::sendReplyEmail(const string& to, const string& subject,
    const string& message_body, const string& thread_id,
    const string& attachment_path) {

    CURL* curl;
    CURLcode res;
    string readBuffer;

    string boundary = "==boundary_" + to_string(chrono::system_clock::now().time_since_epoch().count());

    // Construct the email message
    stringstream email_content;
    email_content << "MIME-Version: 1.0\r\n";
    email_content << "To: " << to << "\r\n";
    email_content << "Subject: Re: " << subject << "\r\n";
    email_content << "Content-Type: multipart/mixed; boundary=\"" << boundary << "\"\r\n\r\n";

    // Text part
    email_content << "--" << boundary << "\r\n";
    email_content << "Content-Type: text/plain; charset=utf-8\r\n\r\n";
    email_content << message_body << "\r\n\r\n";

    // Attachment part (if provided)
    if (!attachment_path.empty()) {
        ifstream file(attachment_path, ios::binary);
        if (file) {
            string file_content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            file.close();

            size_t last_slash = attachment_path.find_last_of("/\\");
            string file_name = (last_slash == string::npos) ? attachment_path : attachment_path.substr(last_slash + 1);

            email_content << "--" << boundary << "\r\n";
            email_content << "Content-Type: application/octet-stream\r\n";
            email_content << "Content-Transfer-Encoding: base64\r\n";
            email_content << "Content-Disposition: attachment; filename=\"" << file_name << "\"\r\n\r\n";
            email_content << base64_encode(file_content) << "\r\n";
        }
        else {
            cerr << "Failed to open attachment file: " << attachment_path << endl;
        }
    }

    email_content << "--" << boundary << "--\r\n";

    // Encode the email content to base64
    string encoded_message = base64_encode(email_content.str());

    // Construct the JSON payload
    Json::Value payload;
    payload["raw"] = encoded_message;
    payload["threadId"] = thread_id;

    Json::FastWriter writer;
    string json_payload = writer.write(payload);

    curl = curl_easy_init();
    if (curl) {
        string url = "https://www.googleapis.com/gmail/v1/users/me/messages/send";
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            return false;
        }
    }
    else {
        cerr << "Failed to initialize cURL." << endl;
        return false;
    }

    return true;
}