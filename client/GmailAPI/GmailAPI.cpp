#include <iostream>
#include <curl/curl.h>
#include <json/json.h>
#include <sstream>
#include <TlHelp32.h>
#include <thread>
#include <chrono>
#include "GmailAPI.h"

namespace {
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    bool CleanupChrome() {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
        PROCESSENTRY32W processEntry{ sizeof(PROCESSENTRY32W) };
        bool found = false;

        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (_wcsicmp(processEntry.szExeFile, L"chrome.exe") == 0) {
                    if (HANDLE process = OpenProcess(PROCESS_TERMINATE, 0, processEntry.th32ProcessID)) {
                        TerminateProcess(process, 9);
                        CloseHandle(process);
                        found = true;
                    }
                }
            } while (Process32NextW(snapshot, &processEntry));
        }

        CloseHandle(snapshot);
        return found;
    }
}

// GoogleOAuth Implementation
GoogleOAuth::GoogleOAuth(const std::string& client_id, const std::string& client_secret, const std::string& redirect_uri)
    : client_id(client_id), client_secret(client_secret), redirect_uri(redirect_uri) {}

std::string GoogleOAuth::urlEncode(const std::string& str) {
    CURL* curl = curl_easy_init();
    char* encoded = curl_easy_escape(curl, str.c_str(), str.length());
    std::string result(encoded);
    curl_free(encoded);
    curl_easy_cleanup(curl);
    return result;
}

std::string GoogleOAuth::getAuthUrl() {
    return auth_url + "?scope=" + urlEncode("https://mail.google.com") +
        "&access_type=offline&response_type=code&prompt=consent" +
        "&redirect_uri=" + urlEncode(redirect_uri) +
        "&client_id=" + urlEncode(client_id);
}

std::string GoogleOAuth::getRefreshToken(const std::string& code) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Không thể khởi tạo CURL");
    }

    std::string response;
    std::string postData = "code=" + urlEncode(code) +
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
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("Lỗi lấy refresh token: " + std::string(curl_easy_strerror(res)));
    }

    Json::Value root;
    if (!Json::Reader().parse(response, root) || !root.isMember("refresh_token")) {
        throw std::runtime_error("Lỗi phân tích refresh token: " + response);
    }

    return root["refresh_token"].asString();
}

std::string GoogleOAuth::getAccessToken(const std::string& refresh_token) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return "";
    }

    std::string response;
    std::string postFields = "client_id=" + urlEncode(client_id) +
        "&client_secret=" + urlEncode(client_secret) +
        "&refresh_token=" + urlEncode(refresh_token) +
        "&grant_type=refresh_token";

    curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "Lỗi thực thi CURL: " << curl_easy_strerror(res) << std::endl;
        return "";
    }

    Json::Value root;
    if (!Json::Reader().parse(response, root)) {
        std::cout << "Lỗi phân tích JSON" << std::endl;
        return "";
    }

    return root["access_token"].asString();
}

// GmailUIAutomation Implementation
GmailUIAutomation::GmailUIAutomation() : automation(nullptr) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    CoCreateInstance(__uuidof(CUIAutomation), NULL, CLSCTX_INPROC_SERVER,
        __uuidof(IUIAutomation), reinterpret_cast<void**>(&automation));
}

GmailUIAutomation::~GmailUIAutomation() {
    if (automation) {
        automation->Release();
    }
    CoUninitialize();
}

bool GmailUIAutomation::findAndClickButton(const wchar_t* buttonText) {
    Sleep(1000);
    IUIAutomationElement* root = nullptr;
    if (!automation || FAILED(automation->GetRootElement(&root))) {
        return false;
    }

    VARIANT prop{};
    prop.vt = VT_BSTR;
    prop.bstrVal = SysAllocString(buttonText);

    IUIAutomationCondition* condition;
    automation->CreatePropertyCondition(UIA_NamePropertyId, prop, &condition);

    IUIAutomationElement* button;
    root->FindFirst(TreeScope_Descendants, condition, &button);

    bool result = false;
    if (button) {
        IUIAutomationInvokePattern* invoke = nullptr;
        button->GetCurrentPatternAs(UIA_InvokePatternId, __uuidof(IUIAutomationInvokePattern),
            (void**)&invoke);

        if (invoke) {
            invoke->Invoke();
            invoke->Release();
            result = true;
        }
        button->Release();
    }

    condition->Release();
    root->Release();
    SysFreeString(prop.bstrVal);

    return result;
}


bool GmailUIAutomation::automateGmailAuth(const std::string& gmail) {
    Sleep(2000);
    bool accountSelected = false;
    bool firstClick = false;
    IUIAutomationElement* root = nullptr;
    automation->GetRootElement(&root);

    // PHẦN 1: Chọn tài khoản email
    VARIANT prop{};
    prop.vt = VT_BSTR;
    std::wstring wGmail(gmail.begin(), gmail.end());
    prop.bstrVal = SysAllocString(wGmail.c_str());

    IUIAutomationCondition* condition;
    automation->CreatePropertyCondition(UIA_NamePropertyId, prop, &condition);

    // Tìm và nhấp vào tài khoản email
    IUIAutomationElement* emailDiv;
    for (int i = 0; i < 3; i++) { // Thử nhấp 3 lần
        if (SUCCEEDED(root->FindFirst(TreeScope_Descendants, condition, &emailDiv)) && emailDiv) {
            IUIAutomationInvokePattern* invoke = nullptr;
            emailDiv->GetCurrentPatternAs(UIA_InvokePatternId, __uuidof(IUIAutomationInvokePattern),
                (void**)&invoke);
            if (invoke) {
                invoke->Invoke();
                invoke->Release();
                accountSelected = true;
                break; // Thoát khỏi vòng lặp nếu thành công
            }
            emailDiv->Release();
        }
        Sleep(500); // Thêm thời gian chờ giữa các lần thử
    }

    Sleep(500);

    // PHẦN 2: Click các nút "Tiếp tục"
    // Thử click nút với class name
    prop.bstrVal = SysAllocString(L"VfPpkd-LgbsSe");
    automation->CreatePropertyCondition(UIA_ClassNamePropertyId, prop, &condition);
    IUIAutomationElement* button;
    if (SUCCEEDED(root->FindFirst(TreeScope_Descendants, condition, &button)) && button) {
        IUIAutomationInvokePattern* invoke = nullptr;
        button->GetCurrentPatternAs(UIA_InvokePatternId, __uuidof(IUIAutomationInvokePattern),
            (void**)&invoke);
        if (invoke) {
            invoke->Invoke();
            invoke->Release();
            firstClick = true;
        }
        button->Release();
    }

    // Thử các text khác nhau nếu không thành công
    if (!firstClick) {
        const wchar_t* texts[] = { L"Tiếp tục", L"Continue", L"Next" };
        for (const wchar_t* text : texts) {
            if (findAndClickButton(text)) {
                firstClick = true;
                break;
            }
        }
    }

    if (firstClick) {
        Sleep(500);
        const wchar_t* texts[] = { L"Tiếp tục", L"Continue", L"Next" };
        for (const wchar_t* text : texts) {
            if (findAndClickButton(text)) {
                Sleep(1000);
                if (HWND browserWindow = FindWindowW(L"Chrome_WidgetWin_1", NULL)) {
                    Sleep(1000);
                    PostMessage(browserWindow, WM_CLOSE, 0, 0);
                    CleanupChrome();
                }
                return true;
            }
        }
    }

    root->Release();
    SysFreeString(prop.bstrVal);
    return false;
}

