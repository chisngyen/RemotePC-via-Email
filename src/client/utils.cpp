#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

string base64_decode(const string& encoded_string) {
    const string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    string decoded_string;
    vector<int> vec(256, -1);
    for (int i = 0; i < 64; i++)
        vec[base64_chars[i]] = i;

    int val = 0, bits = -8;
    for (const auto& c : encoded_string) {
        if (vec[c] == -1) continue;
        val = (val << 6) + vec[c];
        bits += 6;
        if (bits >= 0) {
            decoded_string.push_back(char((val >> bits) & 0xFF));
            bits -= 8;
        }
    }

    return decoded_string;
}

string base64_encode(const string& input) {
    string output;
    int val = 0, valb = -6;
    const string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            output.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) output.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (output.size() % 4) output.push_back('=');
    return output;
}

string trim(const string& str) {
    size_t start = 0;
    while (start < str.length() && (str[start] == ' ' || str[start] == '\n' || str[start] == '\r' || str[start] == '\t')) {
        start++;
    }

    size_t end = str.length();
    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\n' || str[end - 1] == '\r' || str[end - 1] == '\t')) {
        end--;
    }

    return str.substr(start, end - start);
}