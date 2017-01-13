#include <iostream>
#include <string>
#include <fstream>

#include "mdalc.h"

using namespace std;


mdModule::mdModule(string &infile, string &outfile, bool &verbose) {

	moduleLines = nullptr;
	mdBlock = nullptr;
	modulePatterns = nullptr;
	moduleTables = nullptr;
	//uniqueTableCount = 0;

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
	
	try {
	
		if (verbose) cout << endl << "Module data:" << endl;
	
		if (config.useSequence) {
		
			int blockStart = locateToken(string(":SEQUENCE"));
			int blockEnd = getBlockEnd(blockStart);
		
			if (blockStart > blockEnd) throw (string("Sequence contains no patterns."));
		
			mdBlock = new string[blockEnd - blockStart];
		
			for (int i = blockStart + 1; i <= blockEnd; i++) mdBlock[i - blockStart - 1] = moduleLines[i];

			mdSequence seq(mdBlock, blockEnd - blockStart, config, verbose);
		
			delete[] mdBlock;
			mdBlock = nullptr;
		
			if (verbose) cout << seq << endl;
			MUSICASM << seq << endl;
	
		
		
			if (config.useTables) {
			
				moduleTables = new vector<mdTable>;
			
				for (int i = 0; i < config.mdCmdCount; i++) {
			
					if (config.cmdIsTablePointer[i]) {
				
						bool isUsed = false;
					
						for (auto it : *moduleTables) {
						
							if (config.mdCmdList[i].mdCmdDefaultValString == it.tblName) isUsed = true;	
						}
					
						if (!isUsed) moduleTables->push_back(config.mdCmdList[i].mdCmdDefaultValString);
					}
				}
			
	// 			vector<mdTable>::iterator it;
	// 			it = moduleTables->begin();
	// 			cout << it->tblName << endl;
	//			cout << "Capacity: " << moduleTables->capacity() << endl;	
			}
		
		
			//if USE_PATTERNS
			modulePatterns = new mdPattern[seq.uniquePtnCount];		//TODO should be conditional
		
		
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
			
				try {
					modulePatterns[i].read(mdBlock, i, blockEnd - blockStart, config, moduleTables, verbose);
				}
				catch(string &e) {
					throw ("In pattern \"" + seq.uniquePtnList[i] + "\": " + e);
				}
			
				delete[] mdBlock;
				mdBlock = nullptr;
			
				if (verbose) {
			
					cout << config.ptnLabelPrefix + seq.uniquePtnList[i] << endl;
					cout << modulePatterns[i];
				}
			
				MUSICASM << config.ptnLabelPrefix + seq.uniquePtnList[i] << endl;
				MUSICASM << modulePatterns[i];
			
			}
		
		
			if (config.useTables) {
		
				moduleTables->shrink_to_fit();
		
				if (verbose) {
			
					cout << "Unique tables: " << moduleTables->size();
				
					for (auto it : *moduleTables) cout << ", " << it.tblName;
				
					cout << endl;
				}
			
				for (auto it : *moduleTables) {
			
					blockStart = locateToken(":" + it.tblName);
					blockEnd = getBlockEnd(blockStart);
			
					if (blockStart >= linecount - 1) throw ("Table \"" + it.tblName + "\" is not defined.");
					if (blockStart >= blockEnd) throw ("Table \"" + it.tblName + "\" contains no data");
				

					mdBlock = new string[blockEnd - blockStart];
			
					for (int j = blockStart + 1; j <= blockEnd; j++) mdBlock[j - blockStart - 1] = moduleLines[j];
			
					try {
						it.read(mdBlock, blockEnd - blockStart, config, verbose);
					}
					catch(string &e) {
						throw ("In table \"" + it.tblName + "\": " + e);
					}
			
					delete[] mdBlock;
					mdBlock = nullptr;
				
				
				
			
					MUSICASM << it;
					if (verbose) cout << it;
				}
			}
		
		}

		return;
		
	}
	catch(string &e) {
		throw (infile + ": " + e + "\nModule validation failed.");
	}
}


mdModule::~mdModule() {
	
	delete[] mdBlock;
	delete[] moduleLines;
	delete[] modulePatterns;
	delete moduleTables;
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
//	size_t pos = string::npos;
	string tempstr = "";
	string tempstr2;
		
	for (line = 0; line < linecount && tempstr2 != token; line++) {		//used to be line <= linecount

		tempstr2 = moduleLines[line];
		if (tempstr2.find('/') != string::npos) tempstr2.erase(tempstr2.find_first_of('/'));
		tempstr2 = trimChars(tempstr2, " \t");
	}

	if (tempstr2 == token) line--;

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
