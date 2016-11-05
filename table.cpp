#include <iostream>
#include <string>
#include <algorithm>

#include "mdalc.h"

using namespace std;

mdTable::mdTable(string name) {

	tblName = name;
	tblString = "";
	tblLength = 0;
	
	requestList = nullptr;
	lineCommands = nullptr;
	lineCmdVals = nullptr;
	lineCmdStrVals = nullptr;

}


mdTable::~mdTable() {

	for (int i = 0; i < tblLength; i++) {
	
		delete[] lineCommands[i];
		delete[] lineCmdVals[i];
		delete[] lineCmdStrVals[i];
	}

	delete[] lineCommands;
	delete[] lineCmdVals;
	delete[] lineCmdStrVals;
	
	delete[] requestList;


}


void mdTable::read(const string *tblBlock, const int blockLength, const mdConfig &config, const bool &verbose) {

	tblString = config.tblLabelPrefix + tblName + "\n";
	
	for (int i = 0; i < blockLength; i++) {
	
		string temp = trimChars(tblBlock[i], " ()\t");

		if (temp != "") tblLength++;
	}
	
	if (config.tblMaxLength && tblLength > config.tblMaxLength) {
	
		tblLength = 0;
		throw (string("Maximum table length exceeded."));
	}
	
	
	lineCommands = new bool*[tblLength];
	lineCmdVals = new int*[tblLength];
	lineCmdStrVals = new string*[tblLength];
	
	
	for (int row = 0; row < tblLength; row++) {
	
		lineCommands[row] = new bool[config.mdCmdCount];
		fill_n(lineCommands[row], config.mdCmdCount, false);
		
		lineCmdVals[row] = new int[config.mdCmdCount];
		fill_n(lineCmdVals[row], config.mdCmdCount, -1);
		
		lineCmdStrVals[row] = new string[config.mdCmdCount];
		fill_n(lineCmdStrVals[row], config.mdCmdCount, "");
	}
	
	
	//for (int row = 0; row < tblLength; row++) cout << lineCmdVals[row][1] << endl;
	
	
	int row = 0;
	
	for (int i = 0; i < blockLength; i++) {
	
		string rowStr = trimChars(tblBlock[i], " ()\t");

		bool validData = false;
		
		while (rowStr != "") {
		
			validData = true;
		
			if (rowStr == ".") rowStr = "";
			else {
			
				if (!count(begin(rowStr), end(rowStr), '=') || count(begin(rowStr), end(rowStr), ',') != count(begin(rowStr), end(rowStr), '=') - 1)
					throw ("Syntax error in module, line " + tblBlock[i]);
					
						
				string temp = rowStr.substr(0, rowStr.find_first_of("="));				
				int cmdNr = -1;
				
				
				for (int j = 0; j < config.mdCmdCount && cmdNr == -1; j++) {
				
					if (config.mdCmdList[j].mdCmdName == temp) cmdNr = j;
				}
							
				
				if (cmdNr == -1) throw ("Unknown command \"" + temp + "\" found in " + tblBlock[i]);

				
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
						throw ("Invalid argument \"" + temp + "\" in " + tblBlock[i]);
					lineCmdStrVals[row][cmdNr] = temp;
				}
			}	
		}
		
		if (validData) row++;
	}
	

	
	for (int row = 0; row < tblLength; row++) {
	
		for (int cmd = 0; cmd < config.mdCmdCount; cmd++) {
	
			//cout << "Reset row " << row << " cmd " << cmd << endl;		//DEBUG
			
			if (row == 0) config.mdCmdList[cmd].resetToDefault();
			else config.mdCmdList[cmd].reset();			//TODO reset all LastVals to default at beginning of pattern
			
			if (lineCommands[row][cmd]) {
				//cout << "Set row " << row << " cmd " << cmd << endl;	//DEBUG
				
// 				if (config.useTables && config.cmdIsTablePointer[cmd]) {
// 				
// 					bool isUsed = false;
// 					
// 					for (auto it : *moduleTables) {
// 						
// 						if (lineCmdStrVals[row][cmd] == it.tblName) isUsed = true;	
// 					}
// 					
// 					if (!isUsed) moduleTables->push_back(lineCmdStrVals[row][cmd]);
// 				
// 				}
				
				config.mdCmdList[cmd].set(lineCmdVals[row][cmd], lineCmdStrVals[row][cmd]);
				
			}
		}
		
		
		bool lastFieldIsWord = config.tblFieldList[0].isWord;
		bool firstSet = false;
		
		requestList = new bool[config.mdCmdCount];
		fill_n(requestList, config.mdCmdCount, false);
		
		
		
		for (int field = 0; field < config.tblFieldCount; field++) {
		
//			bool seqBegin = (patternNumber == 0) ? true : false;
			config.tblFieldList[field].getRequests(requestList, config, row, false);
			
		}
		
// 		for (int i = 0; i < config.mdCmdCount; i++) cout << boolalpha << i << ": " << requestList[i] << ", ";	//DEBUG ok
// 		cout << endl;												//DEBUG
		
		
		
		for (int field = 0; field < config.tblFieldCount; field++) {
		
			//cout << "row " << row << " field " << field << endl;	//DEBUG
			
			//bool seqBegin = (patternNumber == 0) ? true : false;
			
			string fieldString = config.tblFieldList[field].getFieldString(requestList, config);
			
			
			if (fieldString != "") {
			
				if (!firstSet || lastFieldIsWord != config.tblFieldList[field].isWord) {
				
					firstSet = true;
					lastFieldIsWord = config.tblFieldList[field].isWord;
					
					tblString += "\n\t";
					
					if (lastFieldIsWord) tblString += config.wordDirective + " ";
					else tblString += config.byteDirective + " ";	
				}
				else tblString += ", ";
			}
		
			tblString += fieldString;
		}
		
		delete[] requestList;
		requestList = nullptr;
		//cout << "row: " << row << "str: " << tblString << endl;
		//tblString += "\n";
	}
	
	if (config.useTblEnd) tblString += "\n\t" + config.tblEndString;
	tblString += "\n";

}


ostream& operator<<(ostream& os, const mdTable &tbl) {

	os << tbl.tblString << endl << endl;
	
	return os;
}