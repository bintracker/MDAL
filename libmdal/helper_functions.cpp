#include <string>
#include <cstring>
#include <algorithm>
#include <vector>

#include "mdal.h"

using namespace std;

string trimChars(const string& inputString, const char* chars) {

    string str = inputString;

    for (unsigned i = 0; i < strlen(chars); ++i) str.erase(remove(str.begin(), str.end(), chars[i]), str.end());

    return str;
}

//TODO: flag strings that start with or contain only numbers as invalid
int getType(const string& param) {

    if (param == "true" || param == "false") return BOOL;
    if (param.find('"') != string::npos) return STRING;
    if (param.find_first_of('$') != string::npos) {

        if (param.find_first_of('$') != 0) return INVALID;
        string temp = trimChars(param, "$");
        if (temp.find_first_not_of("0123456789abcdefABCDEF") != string::npos) return INVALID;
        else return HEX;
    }
    if (param.find_first_not_of("0123456789") != string::npos) return INVALID;

    return DEC;
}

string getArgument(string token, const vector<string> &moduleLines) {

    string tempstr = "";

    for (auto&& it: moduleLines) {

        size_t pos = it.find(token.data());
        if (pos != string::npos) tempstr = trimChars(it.substr(pos + token.size()), " =");
    }

    if (tempstr == "") throw ("No " + token + " statement found.");

    return tempstr;
}

bool isNumber(const string &str) {

    if (str.size() == 0) return false;
    if ((str.find_first_not_of("0123456789") == string::npos) ||
            (((str.substr(0,1) == "#") || (str.substr(0,1) == "$"))
             && ((str.substr(1, string::npos)).find_first_not_of("0123456789abcdefABCDEF") == string::npos)
             && str.substr(1, string::npos).size())) return true;

    return false;
}

long strToNum(string str) {

    long num;

    if (str.find("$") != string::npos) {

        str.erase(0, 1);
        num = stol(str, nullptr, 16);
    } else num = stol(str, nullptr, 10);

    return num;
}
