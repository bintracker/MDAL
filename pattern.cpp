#include <iostream>
#include <string>
#include <algorithm>

#include "mdalc.h"

using namespace std;

mdPattern::mdPattern() {

	lineCommands = nullptr;
	lineCmdVals = nullptr;
	lineCmdStrVals = nullptr;
	
	requestList = nullptr;
	
	ptnLength = 0;
	firstInSequence = false;

}


mdPattern::~mdPattern() {

	for (int i = 0; i < ptnLength; i++) {
	
		delete[] lineCommands[i];
		delete[] lineCmdVals[i];
		delete[] lineCmdStrVals[i];
	}

	delete[] lineCommands;
	delete[] lineCmdVals;
	delete[] lineCmdStrVals;
	
	delete[] requestList;

}

void mdPattern::read(const string *ptnBlock, const int patternNumber, const int blockLength, const mdConfig &config, vector<mdTable> *moduleTables, const bool &verbose) {

	if (patternNumber == 0) firstInSequence = true;

	for (int i = 0; i < blockLength; i++) {
	
		string temp = trimChars(ptnBlock[i], " ()\t");

		if (temp != "") ptnLength++;
	}
	
	if (config.ptnMaxLength && ptnLength > config.ptnMaxLength) {
	
		ptnLength = 0;
		throw (string("Maximum pattern length exceeded."));
	}
	
	lineCommands = new bool*[ptnLength];
	lineCmdVals = new int*[ptnLength];
	lineCmdStrVals = new string*[ptnLength];
	
	for (int row = 0; row < ptnLength; row++) {
	
		lineCommands[row] = new bool[config.mdCmdCount];
		fill_n(lineCommands[row], config.mdCmdCount, false);
		
		lineCmdVals[row] = new int[config.mdCmdCount];
		fill_n(lineCmdVals[row], config.mdCmdCount, -1);
		
		lineCmdStrVals[row] = new string[config.mdCmdCount];
		fill_n(lineCmdStrVals[row], config.mdCmdCount, "");
	}
	
	
	//for (int row = 0; row < ptnLength; row++) cout << lineCmdVals[row][1] << endl;
	
	
	int row = 0;
	
	for (int i = 0; i < blockLength; i++) {
	
		string rowStr = trimChars(ptnBlock[i], " ()\t");

		bool validData = false;
		
		while (rowStr != "") {
		
			validData = true;
		
			if (rowStr == ".") rowStr = "";
			else {
			
				if (!count(begin(rowStr), end(rowStr), '=') || count(begin(rowStr), end(rowStr), ',') != count(begin(rowStr), end(rowStr), '=') - 1)
					throw ("Syntax error in module, line " + ptnBlock[i]);
					
						
				string temp = rowStr.substr(0, rowStr.find_first_of("="));				
				int cmdNr = -1;
				
				
				for (int j = 0; j < config.mdCmdCount && cmdNr == -1; j++) {
				
					if (config.mdCmdList[j].mdCmdName == temp) cmdNr = j;
				}
							
				
				if (cmdNr == -1) throw ("Unknown command \"" + temp + "\" found in " + ptnBlock[i]);

				
				lineCommands[row][cmdNr] = true;
				
				rowStr.erase(0, temp.size() + 1);
				
				
				if (rowStr.find_first_of(",") != string::npos) {
					temp = rowStr.substr(0, rowStr.find_first_of(","));
					rowStr.erase(0, temp.size() + 1);
				}
				else {
					temp = rowStr;
					rowStr = "";
				}
				
				
				if (getType(temp) == BOOL) {
					if (temp == "false") lineCmdVals[row][cmdNr] = 0;
					else  lineCmdVals[row][cmdNr] = 1;
				}
				else if (getType(temp) == DEC) lineCmdVals[row][cmdNr] = stoi(temp, nullptr, 10);
				else if (getType(temp) == HEX) lineCmdVals[row][cmdNr] = stoi(trimChars(temp, "$"), nullptr, 16);
				else {
					if (temp.find_first_of("0123456789") < temp.find_first_not_of("0123456789"))
						throw ("Invalid argument \"" + temp + "\" in " + ptnBlock[i]);
					lineCmdStrVals[row][cmdNr] = temp;
				}
			}	
		}
		
		if (validData) row++;
	}
	
	ptnString = "";		//TODO: add pattern name externally
	
	for (int row = 0; row < ptnLength; row++) {
	
		for (int cmd = 0; cmd < config.mdCmdCount; cmd++) {
	
			//cout << "Reset row " << row << " cmd " << cmd << endl;		//DEBUG
			
			if (row == 0 && config.initPtnDefaults) config.mdCmdList[cmd].resetToDefault();
			else config.mdCmdList[cmd].reset();			//TODO reset all LastVals to default at beginning of pattern
			
			if (lineCommands[row][cmd]) {
				//cout << "Set row " << row << " cmd " << cmd << endl;	//DEBUG
				
				if (config.useTables && config.cmdIsTablePointer[cmd]) {
				
					bool isUsed = false;
					
					for (auto it : *moduleTables) {
						
						if (lineCmdStrVals[row][cmd] == it.tblName) isUsed = true;	
					}
					
					if (!isUsed) moduleTables->push_back(lineCmdStrVals[row][cmd]);
				
				}
				
				config.mdCmdList[cmd].set(lineCmdVals[row][cmd], lineCmdStrVals[row][cmd]);
				
			}
		}
		
		
		bool lastFieldIsWord = config.ptnFieldList[0].isWord;
		bool firstSet = false;
		
		requestList = new bool[config.mdCmdCount];
		fill_n(requestList, config.mdCmdCount, false);
		
		
		
		for (int field = 0; field < config.ptnFieldCount; field++) {
		
			bool seqBegin = (patternNumber == 0) ? true : false;
			config.ptnFieldList[field].getRequests(requestList, config, row, seqBegin);
			
		}
		
// 		for (int i = 0; i < config.mdCmdCount; i++) cout << boolalpha << i << ": " << requestList[i] << ", ";	//DEBUG ok
// 		cout << endl;												//DEBUG
		
		
		
		for (int field = 0; field < config.ptnFieldCount; field++) {
		
			//cout << "row " << row << " field " << field << endl;	//DEBUG
			
			//bool seqBegin = (patternNumber == 0) ? true : false;
			
			string fieldString = config.ptnFieldList[field].getFieldString(requestList, config);
			
			
			if (fieldString != "") {
			
				if (!firstSet || lastFieldIsWord != config.ptnFieldList[field].isWord) {
				
					firstSet = true;
					lastFieldIsWord = config.ptnFieldList[field].isWord;
					
					ptnString += "\n\t";
					
					if (lastFieldIsWord) ptnString += config.wordDirective + " ";
					else ptnString += config.byteDirective + " ";	
				}
				else ptnString += ", ";
			}
		
			ptnString += fieldString;
		}
		
		delete[] requestList;
		requestList = nullptr;
		//cout << "row: " << row << "str: " << ptnString << endl;
		//ptnString += "\n";
	}
	
	if (config.usePtnEnd) ptnString += "\n\t" + config.ptnEndString;
	ptnString += "\n";

}


ostream& operator<<(ostream& os, const mdPattern &ptn) {

	os << ptn.ptnString << endl;
	
	return os;
}