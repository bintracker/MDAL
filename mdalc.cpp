#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>

#include "mdalc.h"

using namespace std;


int main(int argc, char *argv[]){

	cout << "MDAL COMPILER v0.0.1\n\n";

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
		mdModule mdf(infile, outfile, verbose);
	}
	catch(string &e) {
		cout << "ERROR: " << e << "\nCompilation terminated." << endl;
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

	//cout << "getType: " << param << endl;	//DEBUG ok

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