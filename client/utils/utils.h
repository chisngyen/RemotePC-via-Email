#ifndef UTILS_H
#define UTILS_H

#include <string>

using namespace std;

string base64_decode(const string& encoded_string);

string base64_encode(const string& input);

string trim(const string& str);
#endif // UTILS_H
