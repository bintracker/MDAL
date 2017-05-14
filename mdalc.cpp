#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "libmdal/mdal.h"

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

