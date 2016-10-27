#include <iostream>
#include <string>
#include <fstream>

#include "mdalc.h"

using namespace std;


mdModule::mdModule(string &infile, string &outfile, bool &verbose) {

	moduleLines = nullptr;
	mdBlock = nullptr;
	modulePatterns = nullptr;

	ifstream MDFILE(infile.data());	
	if (!MDFILE.is_open()) throw (infile + " not found.");
	
	ofstream MUSICASM(outfile.data());
	if (!MUSICASM.is_open()) throw ("Could not open " + outfile + ".");	
	
	string tempstr;
	
	for (linecount = 0; getline(MDFILE,tempstr); linecount++);
	
	MDFILE.clear();										//reset file pointer
	MDFILE.seekg(0, ios::beg);
	
	moduleLines = new string[linecount];
	bool blockComment = false;
	
	for (int i = 0; i < linecount; i++) {
	
		getline(MDFILE, moduleLines[i]);

		string remains = "";
		
		if (moduleLines[i].find("/*", 0, 2) != string::npos) {				//detect and strip block comments
		
			remains = moduleLines[i].erase(moduleLines[i].find("/*", 0, 2), moduleLines[i].find("*/", 0, 2));
			blockComment = true;	
		}
		
		if (moduleLines[i].find("*/", 0, 2) != string::npos) {
		
			blockComment = false;
			moduleLines[i].erase(0, moduleLines[i].find("*/", 0, 2) + 2);
		}
		
		if (blockComment) moduleLines[i] = "" + remains;
		else {
		
			size_t commentPos = moduleLines[i].find("//", 0, 2);			//strip regular comments
			if (commentPos != string::npos) moduleLines[i].erase(commentPos);
		}
	}

	string configname = getArgument(string("CONFIG"));
	
	mdConfig config(configname, verbose);
	
	if (verbose) cout << endl << "Module data:" << endl;
	
	if (config.useSequence) {
		
		int blockStart = locateToken(string(":SEQUENCE"));
		int blockEnd = getBlockEnd(blockStart);
		
		if (blockStart > blockEnd) throw (string("Sequence contains no patterns"));
		
		mdBlock = new string[blockEnd - blockStart];
		
		for (int i = blockStart + 1; i <= blockEnd; i++) mdBlock[i - blockStart - 1] = moduleLines[i];

		mdSequence seq(mdBlock, blockEnd - blockStart, config, verbose);
		
		delete[] mdBlock;
		mdBlock = nullptr;
		
		if (verbose) cout << seq << endl;
		MUSICASM << seq << endl;
	
	
		//if USE_PATTERNS
		modulePatterns = new mdPattern[seq.uniquePtnCount];
		
		for (int i = 0; i < seq.uniquePtnCount; i++) {
		
			//cout << "Detecting pattern " << seq.uniquePtnList[i] << "... ";	//DEBUG
	
			blockStart = locateToken(":" + seq.uniquePtnList[i]);
			blockEnd = getBlockEnd(blockStart);
			
			//cout << "detected." << endl;					//DEBUG
			
			if (blockStart >= linecount - 1) throw ("Pattern \"" + seq.uniquePtnList[i] + "\" is not defined.");
			
			//cout << seq.uniquePtnList[i] << " s: " << blockStart << " e: " << blockEnd << endl;
	
			if (blockStart >= blockEnd) throw ("Pattern \"" + seq.uniquePtnList[i] + "\" contains no data");
			//TODO: does not reliably detect empty patterns.
			
			mdBlock = new string[blockEnd - blockStart];
			
			for (int j = blockStart + 1; j <= blockEnd; j++) mdBlock[j - blockStart - 1] = moduleLines[j];
			
			modulePatterns[i].read(mdBlock, i, blockEnd - blockStart, config, verbose);
			
			delete[] mdBlock;
			mdBlock = nullptr;
			
			if (verbose) {
			
				cout << config.ptnLabelPrefix + seq.uniquePtnList[i] << endl;
				cout << modulePatterns[i];
			}
			
			MUSICASM << config.ptnLabelPrefix + seq.uniquePtnList[i] << endl;
			MUSICASM << modulePatterns[i];
			
		}
	
	}

	return;
}


mdModule::~mdModule() {
	
	delete[] mdBlock;
	delete[] moduleLines;
	delete[] modulePatterns;
	//delete seq;
}


int mdModule::getBlockEnd(int blockStart) {
	
	int line;
	size_t pos = string::npos;
	
	for (line = blockStart + 1; line < linecount && pos == string::npos; line++) pos = moduleLines[line].find(":");
	
	if (line == linecount) return line - 1;
	return line - 2;
}

//TODO: throw error if token not found
int mdModule::locateToken(string token) {

	int line;
	size_t pos = string::npos;
	string tempstr = "";
	string tempstr2;
	
	for (line = 0; line < linecount && tempstr == ""; line++) {	//used to be line <= linecount
	
		tempstr2 = trimChars(moduleLines[line], " \t");
		pos = tempstr2.find(token.data());			//check if token is commented out
		
		if (pos == 0) tempstr = tempstr2;
	}

	if (pos == 0) line--;			//TODO: this seems somewhat fishy, investigate more

	return line;
}

string mdModule::getArgument(string token) {

	string tempstr = "";
	int line;
	
	for (line = 0; line < linecount && tempstr == ""; line++) {
	
		size_t pos = moduleLines[line].find(token.data());
		if (pos != string::npos) tempstr = trimChars(moduleLines[line].substr(pos + token.size()), " =");
	}
	
	if (line == linecount) throw ("No " + token + " statement found.");
	
	return tempstr;
}


// ostream& operator<<(ostream &os, const mdModule &mdf) {
//
// 	os << mdf.seq << endl;
// 	
// 	for (int i = 0; i < seq.uniquePtnCount; i++) {
// 	
// 		os << mdf.seq.uniquePtnList[i] << endl;
// 		os << mdf.modulePatterns[i];
// 	}
// 	
// 	return os;
// }
