#include <iostream>
#include <string>
#include <algorithm>

#include "mdalc.h"

using namespace std;

mdBlock::mdBlock(string name, bool seqStart) {

	blkName = name;
	blkString = "";
	blkLength = 0;
	
	firstInSequence = seqStart;
	
	requestList = nullptr;
	lineCommands = nullptr;
	lineCmdVals = nullptr;
	lineCmdStrVals = nullptr;

}


mdBlock::~mdBlock() {

	for (int i = 0; i < blkLength; i++) {
	
		delete[] lineCommands[i];
		delete[] lineCmdVals[i];
		delete[] lineCmdStrVals[i];
	}

	delete[] lineCommands;
	delete[] lineCmdVals;
	delete[] lineCmdStrVals;
	
	delete[] requestList;


}


void mdBlock::read(const string *rawData, const int blockLength, const mdConfig &config, const mdBlockConfig &blkConfig, const bool &verbose) {

	blkString = blkConfig.blkLabelPrefix + blkName + "\n";
	
	for (int i = 0; i < blockLength; i++) {
	
		string temp = trimChars(rawData[i], " ()\t");

		if (temp != "") blkLength++;
	}
	
	if (blkConfig.blkMaxLength && blkLength > blkConfig.blkMaxLength) throw (string("Maximum table length exceeded."));
	
	
	lineCommands = new bool*[blkLength];
	lineCmdVals = new int*[blkLength];
	lineCmdStrVals = new string*[blkLength];
	
	
	for (int row = 0; row < blkLength; row++) {
	
		lineCommands[row] = new bool[config.mdCmdCount];
		fill_n(lineCommands[row], config.mdCmdCount, false);
		
		lineCmdVals[row] = new int[config.mdCmdCount];
		fill_n(lineCmdVals[row], config.mdCmdCount, -1);
		
		lineCmdStrVals[row] = new string[config.mdCmdCount];
		fill_n(lineCmdStrVals[row], config.mdCmdCount, "");
	}
	
	
	//for (int row = 0; row < blkLength; row++) cout << lineCmdVals[row][1] << endl;
	
	
	int row = 0;
	
	for (int i = 0; i < blockLength; i++) {
	
		string rowStr = trimChars(rawData[i], " ()\t");

		bool validData = false;
		
		while (rowStr != "") {
		
			validData = true;
		
			if (rowStr == ".") rowStr = "";
			else {
			
				if (count(begin(rowStr), end(rowStr), ',') > count(begin(rowStr), end(rowStr), '='))
					throw ("Syntax error in module, line " + rawData[i]);
					
				string temp;
						
				if (rowStr.find_first_of(",=") == string::npos) temp = rowStr;
				else temp = rowStr.substr(0, rowStr.find_first_of(",="));
								
				int cmdNr = -1;
				
				
				for (int j = 0; j < config.mdCmdCount && cmdNr == -1; j++) {
				
					if (config.mdCmdList[j].mdCmdName == temp) cmdNr = j;
				}
							
				
				if (cmdNr == -1) throw ("Unknown command \"" + temp + "\" found in " + rawData[i]);

				
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
				
				
				
				if (!config.mdCmdList[cmdNr].mdCmdAuto) {
				
					if (getType(temp) == BOOL) {
						if (temp == "false") lineCmdVals[row][cmdNr] = 0;
						else  lineCmdVals[row][cmdNr] = 1;
					}
					else if (getType(temp) == DEC) lineCmdVals[row][cmdNr] = stoi(temp, nullptr, 10);
					else if (getType(temp) == HEX) lineCmdVals[row][cmdNr] = stoi(trimChars(temp, "$"), nullptr, 16);
					else {
						if (temp.find_first_of("0123456789") < temp.find_first_not_of("0123456789"))
							throw ("Invalid argument \"" + temp + "\" in " + rawData[i]);
						lineCmdStrVals[row][cmdNr] = temp;
					}
				}
			}	
		}
		
		if (validData) row++;
	}
	

	
	for (int row = 0; row < blkLength; row++) {
	
		for (int cmd = 0; cmd < config.mdCmdCount; cmd++) {
	
			//cout << "Reset row " << row << " cmd " << cmd << endl;		//DEBUG
			
			if (row == 0) config.mdCmdList[cmd].resetToDefault();
			else config.mdCmdList[cmd].reset();			//TODO reset all LastVals to default at beginning of pattern
			
			if (lineCommands[row][cmd]) config.mdCmdList[cmd].set(lineCmdVals[row][cmd], lineCmdStrVals[row][cmd]);

		}
		
		
		bool lastFieldIsWord = blkConfig.blkFieldList[0].isWord;
		bool firstSet = false;
		
		requestList = new bool[config.mdCmdCount];
		fill_n(requestList, config.mdCmdCount, false);
		
		
		
		for (int field = 0; field < blkConfig.blkFieldCount; field++) {
		
//			bool seqBegin = (patternNumber == 0) ? true : false;
			blkConfig.blkFieldList[field].getRequests(requestList, config, row, firstInSequence);
			
		}		
		
		
		for (int field = 0; field < blkConfig.blkFieldCount; field++) {
		
			//cout << "row " << row << " field " << field << endl;	//DEBUG
			
			//bool seqBegin = (patternNumber == 0) ? true : false;
			
			string fieldString = blkConfig.blkFieldList[field].getFieldString(requestList, config);		//TODO: ...
			
			
			if (fieldString != "") {
			
				if (!firstSet || lastFieldIsWord != blkConfig.blkFieldList[field].isWord) {
				
					firstSet = true;
					lastFieldIsWord = blkConfig.blkFieldList[field].isWord;
					
					blkString += "\n\t";
					
					if (lastFieldIsWord) blkString += config.wordDirective + " ";
					else blkString += config.byteDirective + " ";	
				}
				else blkString += ", ";
			}
		
			blkString += fieldString;
		}
		
		delete[] requestList;
		requestList = nullptr;
 	}
	
	if (blkConfig.useBlkEnd) blkString += "\n\t" + blkConfig.blkEndString;
	blkString += "\n";

}


ostream& operator<<(ostream& os, const mdBlock &blk) {

	os << blk.blkString << endl << endl;
	
	return os;
}