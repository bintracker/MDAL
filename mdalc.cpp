#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>

#include "mdalc.h"

using namespace std;


int main(int argc, char *argv[]){

	cout << "MDAL COMPILER v0.0.2\n\n";

	bool verbose = false;
	string infile = "";
	string outfile = "music.asm";

	for (int i = 1; i < argc; i++) {
	
		string arg = argv[i];
		if (arg == "-v") verbose = true;
		if (arg == "-i" && i < argc-1) infile = argv[i+1];
		if (arg == "-o" && i < argc-1) outfile = argv[i+1];
	}
	
	if (infile == "") {
	
		cout << "Usage: mdalc -i <infile> [-o <outfile> -v]\n";
		return -1;
	}

	try {
		ifstream MDFILE(infile.data());	
		if (!MDFILE.is_open()) throw (infile + " not found.");
		
	 	ofstream ASM(outfile.data());
	 	if (!ASM.is_open()) throw ("Could not open " + outfile + ".");
		
		string tempstr;
		vector<string> moduleLines;

		bool blockComment = false;

		while (getline(MDFILE, tempstr)) {

			string remains = "";
	
			if (tempstr.find("/*", 0, 2) != string::npos) {				//detect and strip block comments
	
				remains = tempstr.erase(tempstr.find("/*", 0, 2), tempstr.find("*/", 0, 2));
				blockComment = true;	
			}
	
			if (tempstr.find("*/", 0, 2) != string::npos) {
	
				blockComment = false;
				tempstr.erase(0, tempstr.find("*/", 0, 2) + 2);
			}
	
			if (blockComment) tempstr = "" + remains;
			else {
	
				size_t commentPos = tempstr.find("//", 0, 2);			//strip regular comments
				if (commentPos != string::npos) tempstr.erase(commentPos);
			}
			
			moduleLines.push_back(tempstr);
		}
		
		string configname = "config/" + getArgument(string("CONFIG"), moduleLines) + ".mdconf";
		mdConfig config;
		config.init(configname, verbose);
	 
		mdModule mdf(moduleLines, config, verbose);
		
		ASM << mdf;
	}
	catch(string &e) {
		cout << "In " << infile << endl << "ERROR: " << e << "\nCompilation terminated." << endl;
		return -1;
	}
	
	return 0;
}

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
	}
	else num = stol(str, nullptr, 10);
	
	return num;
}

