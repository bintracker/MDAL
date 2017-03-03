#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>

#include "mdalc.h"

using namespace std;



mdConfig::mdConfig() {

	cfgLines = nullptr;
	mdCmdList = nullptr;
	seqLabel = ";sequence";
	seqMaxLength = 0;
	
	blockTypeCount = 0;
}


void mdConfig::init(string &configname, bool &verbose) {
	

	ifstream CFGFILE(configname.data());
	if (!CFGFILE.is_open()) throw ("Unknown configuration \"" + configname + "\".");


	try {
	
		if (verbose) cout << "configuration:\t\t" << configname << ".cfg" << endl;
	
		string tempstr;
	
		for (linecount = 0; getline(CFGFILE,tempstr); linecount++);
		int configEnd = linecount - 1;
	
		CFGFILE.clear();						//reset file pointer
		CFGFILE.seekg(0, ios::beg);
	
		cfgLines = new string[linecount];
	
		for (int i = 0; i < linecount; i++) getline(CFGFILE,cfgLines[i]);
	
		if (locateToken(string("MDAL_VERSION"), 0, configEnd) == configEnd) throw (string("MDAL_VERSION not specified."));
		string mdVersion = trimChars(getArgumentString(string("MDAL_VERSION"), 0, linecount-1), "()");
		if (getType(mdVersion) != DEC) throw (string("MDAL_VERSION argument is not a decimal number."));
		if (stoi(mdVersion, nullptr, 10) > MDALVERSION) throw ("MDAL VERSION " + mdVersion + " not supported in this version of mdalc.");
		if (verbose) cout << "MDAL version: \t\t" << stoi(mdVersion, nullptr, 10) << endl;
	
	
		wordDirective = trimChars(getArgumentString(string("WORD_DIRECTIVE"), 0, configEnd), "()\"");
		if (wordDirective == "") wordDirective = "dw";
		if (verbose) cout << "word directive:\t\t" << wordDirective << endl;
	
		byteDirective = trimChars(getArgumentString(string("BYTE_DIRECTIVE"), 0, configEnd), "()\"");
		if (byteDirective == "") byteDirective = "db";
		if (verbose) cout << "byte directive:\t\t" << byteDirective << endl;
	
		hexPrefix = trimChars(getArgumentString(string("HEX_PREFIX"), 0, configEnd), "()\"");
		if (hexPrefix == "") hexPrefix = "$";
		if (verbose) cout << "hex prefix:\t\t" << hexPrefix << endl;
	

		if (verbose) cout << "\nSEQUENCE CONFIGURATION\n======================" << endl;

		int blockStart = locateToken(string("CFG_SEQUENCE"), 0, configEnd);
	
		if (blockStart == configEnd) throw (string("No CFG_SEQUENCE block found."));
	
		else {
		
			int blockEnd = getBlockEnd(blockStart);
		
			useSeqEnd = false;
			useSeqLoop = false;
			useSeqLoopPointer = false;
		
		
			int tokenpos = locateToken(string("USE_LABEL"), blockStart, blockEnd);
		
			if (tokenpos != blockEnd) {
		
				seqLabel = trimChars(getArgument(cfgLines[tokenpos], 1), "\"");
				if (seqLabel == "") throw (string("CFG_SEQUENCE: Missing argument in USE_LABEL() declaration."));
			}
		
			if (verbose) cout << "Sequence label:\t\t" << seqLabel << endl;
		
		
		
			tokenpos = locateToken(string("USE_END"), blockStart, blockEnd);
		
			if (tokenpos != blockEnd) {
		
				useSeqEnd = true;
				seqEndString = trimChars(getArgument(cfgLines[tokenpos], 1), "\"");
				if (seqEndString == "") throw (string("CFG_SEQUENCE: Missing argument in USE_END() declaration."));
				if (verbose) cout << "Sequence end:\t\t" << seqEndString << endl;
			}
		
		
			tokenpos = locateToken(string("USE_LOOP"), blockStart, blockEnd);
		
			if (tokenpos != blockEnd) {
		
				useSeqLoop = true;
			
				int argCount = getArgumentCount(cfgLines[tokenpos]);
			
				if (argCount < 2) throw (string("CFG_SEQUENCE: Incomplete loop configuration."));
			
				string arg = getArgument(cfgLines[tokenpos], 1);
			
				if (arg != "LABEL" && arg != "POINTER") throw ("CFG_SEQUENCE: Invalid loop type \"" + arg + "\".");
				else if (arg == "POINTER") useSeqLoopPointer = true;
			
				seqLoopLabel = trimChars(getArgument(cfgLines[tokenpos], 2), "\"");
			
				if (verbose) cout << "Loop type:\t\t" << arg << "\nLoop label:\t\t" << seqLoopLabel << endl;
			}
		
		
			tokenpos = locateToken(string("MAX_LENGTH"), blockStart, blockEnd);
	
			if (tokenpos != blockEnd) {
	
				string maxLengthStr = trimChars(getArgument(cfgLines[tokenpos], 1), "\"");
				if (maxLengthStr == "") throw (string("CFG_SEQUENCE: Missing argument in MAX_LENGTH() declaration."));
				if (getType(maxLengthStr) == DEC) seqMaxLength = stoi(maxLengthStr, nullptr, 10);
				else if (getType(maxLengthStr) == HEX) seqMaxLength = stoi(trimChars(maxLengthStr, "$"), nullptr, 16);
				else throw (string("CFG_SEQUENCE: MAX_LENGTH() does not specify an integer."));
				if (verbose) cout << "Max. sequence length:\t" << seqMaxLength << endl;
			}
		}
	
		if (verbose) cout << endl;

	
	
		blockStart = locateToken(string("CFG_COMMANDS"), 0, configEnd);
		
		if (blockStart == linecount - 1) throw (string("No CFG_COMMANDS block found."));
		
		int blockEnd = getBlockEnd(blockStart);
		blockStart++;

		mdCmdCount = countBlockLines(blockStart, blockEnd);
		if (!mdCmdCount) throw (string("CFG_COMMANDS: No commands specified."));
	
		mdCmdList = new mdCommand[mdCmdCount];

		int cmdNr = 0;
	
		if (verbose) cout << "USER COMMANDS\n=============" << endl;
	
		for (int i = blockStart; i < blockEnd; i++) {
	
			string cmdStr = cfgLines[i];
			cmdStr = trimChars(cmdStr, " \t");	//TODO: this trims all whitespace, don't trim whitespace within cmd string params!
			size_t pos = cmdStr.find_first_of('/');
			if (pos != string::npos) cmdStr.erase(pos);
			if (cmdStr != "") {
		
				mdCmdList[cmdNr].init(cmdStr, verbose);
				cmdNr++;
			}
		}
	
		if (verbose) cout << endl;



		for (int line = 0; line < configEnd; line++) {
		
			if (cfgLines[line].find("CFG_BLOCK") != string::npos) blockTypeCount++;
		}


		if (!blockTypeCount) throw (string("No block configurations found."));	
		
		if (verbose) cout << "BLOCK CONFIGURATIONS\n====================\nBlock types: \t\t" << blockTypeCount << endl;


		blockTypes.reserve(blockTypeCount);
		
		int bline = 0;
		
		for (int i = 0; i < blockTypeCount; i++) {
		
			blockStart = locateToken(string("CFG_BLOCK"), bline, configEnd);
			int blockEnd = getBlockEnd(blockStart);
			bline = blockEnd + 1;
			
			int tokenpos = locateToken(string("ID"), blockStart, blockEnd);
			
			if (tokenpos >= blockEnd) throw (string("CFG_BLOCK: Missing block ID in block"));
			
			string id = trimChars(getArgument(cfgLines[tokenpos], 1), "\"");
			
			if (id == "") throw (string("Invalid ID in block"));

			blockTypes.emplace_back(id);
			
			tokenpos = locateToken(string("TYPE"), blockStart, blockEnd)	;
			
			if (tokenpos >= blockEnd) blockTypes.back().baseType = GENERIC;
			else {
			
				string type = getArgument(cfgLines[tokenpos], 1);
				if (type == "PATTERN") blockTypes.back().baseType = PATTERN;
				else if (type == "TABLE") blockTypes.back().baseType = TABLE;
				else if (type == "GENERIC") blockTypes.back().baseType = GENERIC;
				else throw ("CFG_BLOCK " + blockTypes.back().blockConfigID + ": Unknown block base type " + type);
			
			}
			
			if (verbose) {
			
				cout << "\nBLOCK TYPE " << blockTypes.back().blockConfigID << "\nBase type: \t\t"; 
				if (blockTypes.back().baseType == GENERIC) cout << "GENERIC";
				else if (blockTypes.back().baseType == PATTERN) cout << "PATTERN";
				else cout << "TABLE";
				cout << endl;
				
			}
			
			
			tokenpos = locateToken(string("USE_END"), blockStart, blockEnd);
		
			if (tokenpos != blockEnd) {
		
				blockTypes.back().useBlkEnd = true;
				blockTypes.back().blkEndString = trimChars(getArgument(cfgLines[tokenpos], 1), "\"");
				if (blockTypes.back().blkEndString == "") 
					throw("CFG_BLOCK " + blockTypes.back().blockConfigID + ": Missing argument in USE_END() declaration.");
				if (verbose) cout << "Block end:\t\t" << blockTypes.back().blkEndString << endl;
			}
			
			
			tokenpos = locateToken(string("MAX_LENGTH"), blockStart, blockEnd);
		
			if (tokenpos != blockEnd) {
		
				string maxLengthStr = trimChars(getArgument(cfgLines[tokenpos], 1), "\"");
				if (maxLengthStr == "") 
					throw ("CFG_BLOCK " + blockTypes.back().blockConfigID + ": Missing argument in MAX_LENGTH() declaration.");
				if (getType(maxLengthStr) == DEC) blockTypes.back().blkMaxLength = stoi(maxLengthStr, nullptr, 10);
				else if (getType(maxLengthStr) == HEX) blockTypes.back().blkMaxLength = stoi(trimChars(maxLengthStr, "$"), nullptr, 16);
				else throw("CFG_BLOCK " + blockTypes.back().blockConfigID + ": MAX_LENGTH() does not specify an integer.");
				if (verbose) cout << "Max. block length:\t" << blockTypes.back().blkMaxLength << endl;
			}
			
			
			tokenpos = locateToken(string("LABEL_PREFIX"), blockStart, blockEnd);
		
			if (tokenpos != blockEnd) {

				blockTypes.back().blkLabelPrefix = trimChars(getArgument(cfgLines[tokenpos], 1), "\"");
				if (blockTypes.back().blkLabelPrefix == "") 
					throw ("CFG_BLOCK " + blockTypes.back().blockConfigID + ": Missing argument in LABEL_PREFIX() declaration.");
			}
		
			if (verbose) cout << "Pattern label prefix:\t" << blockTypes.back().blkLabelPrefix << endl;
		
		
			tokenpos = locateToken(string("INIT_DEFAULTS"), blockStart, blockEnd);
		
			if (tokenpos != blockEnd) {
		
				blockTypes.back().initBlkDefaults = true;
				if (verbose) cout << "Initialize commands with default values at each block start" << endl;
			}
			
			
			blockTypes.back().blkFieldCount = countFields(blockStart, blockEnd);
			if (!blockTypes.back().blkFieldCount) throw ("CFG_BLOCK " + blockTypes.back().blockConfigID + ": No output fields specified.");
		
			blockTypes.back().blkFieldList = new mdField[blockTypes.back().blkFieldCount];
		
			int fieldNr = 0;
			string requireBlkBeginStr = "";
			string requireSeqBeginStr = "";
		
			for (int i = blockStart; i < blockEnd; i++) {
	
				string fieldStr = cfgLines[i];
				fieldStr = trimChars(fieldStr, " \t");	//TODO: this trims all whitespace, don't trim whitespace within cmd string params!
				size_t pos = fieldStr.find_first_of('/');
				if (pos != string::npos) fieldStr.erase(pos);
				if (fieldStr != "" && (fieldStr.find("WORD") != string::npos || fieldStr.find("BYTE") != string::npos)) {
		
					blockTypes.back().blkFieldList[fieldNr].init(mdCmdList, mdCmdCount, fieldStr, verbose);
			
					if (verbose) {
			
						cout << "Field " << fieldNr;
						if (blockTypes.back().blkFieldList[fieldNr].requiredAlways) cout << " always required";
						else {
							cout << " required if ";
							for (int j = 0; j < mdCmdCount; j++) {
				
								if (blockTypes.back().blkFieldList[fieldNr].requiredBy[j]) {
							
									bool lastentry = true;
									for (int k = j + 1; k < mdCmdCount; k++) 
										if (blockTypes.back().blkFieldList[fieldNr].requiredBy[k]) lastentry = false;
					
									cout << mdCmdList[j].mdCmdName;
									if (blockTypes.back().blkFieldList[fieldNr].requiredWhenSet[j]) cout << " is set ";
									else cout << " is not set ";
						
									if (blockTypes.back().blkFieldList[fieldNr].requiredByAny && !lastentry) cout << "or ";
									else if (!blockTypes.back().blkFieldList[fieldNr].requiredByAny && !lastentry) cout << "and ";
								}
							}
						}
				
						cout << endl;
					}
			
					fieldNr++;
				}
		
				if (fieldStr.find("REQUIRE_SEQ_BEGIN") != string::npos) requireSeqBeginStr = fieldStr;
				if (fieldStr.find("REQUIRE_BLK_BEGIN") != string::npos) requireBlkBeginStr = fieldStr;
			}
			
			//TODO implementation incomplete
			if (requireSeqBeginStr != "") {
	
				for (int i = 0; i < blockTypes.back().blkFieldCount; i++) blockTypes.back().blkFieldList[i].requiredSeqBegin = true;
			}
	
			if (requireBlkBeginStr != "") {
	
				for (int i = 0; i < blockTypes.back().blkFieldCount; i++) blockTypes.back().blkFieldList[i].requiredBlkBegin = true;	

			}
		
		}
		
		
		bool ptnBlockPresent = false;
		

		for (auto&& it : blockTypes) {
		
			if (it.baseType == PATTERN) ptnBlockPresent = true;
		}
		
		if (!ptnBlockPresent) throw(string("Must declare at least one block type as base type PATTERN"));
		
		
		for (int i = 0; i < mdCmdCount; i++) {
		
			if (mdCmdList[i].isBlkReference) {
			
				bool validRef = false;
				for (auto&& it : blockTypes) if (it.blockConfigID == mdCmdList[i].referenceBlkID) validRef = true;
				
				if (!validRef) throw("Block type referenced by command " + mdCmdList[i].mdCmdName + " does not exist.");
			}
		}
		
		//TODO: check reference command validity

	
		useSamples = false;
		if (locateToken(string("USE_SAMPLES"), 0, configEnd) != configEnd) {
	
			useSamples = true;
			if (verbose) cout << "using SAMPLES - this feature is not supported yet." << endl;
		}
	
	
		return;
	}
	catch(string &e) {
		throw (configname + ".cfg: " + e + "\nConfig validation failed.");
	}
}

mdConfig::~mdConfig() {


//	delete[] ptnFieldList;
//	delete[] tblFieldList;
	delete[] mdCmdList;
//	delete[] cmdIsTablePointer;
	delete[] cfgLines;
}



int mdConfig::countFields(int &blockStart, int &blockEnd) {

	int fieldCount = 0;
	
	for (int line = blockStart; line < blockEnd; line++) {
	
		string temp = cfgLines[line];
		if (temp.find_first_of('/') != string::npos) temp.erase(temp.find_first_of('/'));
		if (temp.find("WORD") != string::npos || temp.find("BYTE") != string::npos) fieldCount++;
	}
	
	return fieldCount;
}


//count lines in a definition block, omitting comments and empty lines
int mdConfig::countBlockLines(int &blockStart, int &blockEnd) {

	int cmdlines = 0;
	
	for (int line = blockStart; line < blockEnd; line++) {
	
		string temp = cfgLines[line];
		temp = trimChars(temp, " \t");
		size_t pos = temp.find_first_of('/');
		if (pos != string::npos) temp.erase(pos);
		if (temp != "") cmdlines++;
	}
	
	
	return cmdlines;
}


int mdConfig::getBlockEnd(int blockStart) {

	int line;
	size_t pos = string::npos;
	
	for (line = blockStart; line < linecount && pos == string::npos; line++) pos = cfgLines[line].find("}");
	
	return line - 1;
}


int mdConfig::locateToken(string token, int blockStart, int blockEnd) {

	int line;
	size_t pos = string::npos;
	string tempstr = "";
	string tempstr2;
	
	for (line = blockStart; line <= blockEnd && tempstr == ""; line++) {
	
		tempstr2 = trimChars(cfgLines[line], " \t");
		pos = tempstr2.find(token.data());			//check if token is commented out
		
		if (pos == 0) tempstr = tempstr2;
	}

	if (pos == 0) line--;			//TODO: this seems somewhat fishy, investigate more
	else line = blockEnd;
	return line;
}


int mdConfig::getArgumentCount(string argString) {

	return count(argString.begin(), argString.end(), ',') + 1;
}


string mdConfig::getArgument(string argString, int argNumber) {

	string arg = argString;
	size_t pos = arg.find("(");
	
	arg.erase(0, pos + 1);
	
	arg.erase(0, arg.find_first_not_of(' '));
	
	
	for (int removeCnt = 0; removeCnt < argNumber - 1; removeCnt++) {
		pos = arg.find(",");
		arg.erase(0, pos + 1);
		arg.erase(0, arg.find_first_not_of(' '));
	}

	pos = arg.find(",");
	if (pos != string::npos) arg.erase(pos);
	
	pos = arg.find(")");
	if (pos != string::npos) arg.erase(pos);
	
	arg.erase(arg.find_last_not_of(' ')+1); 
	
	return arg;
}

//TODO: do not delete any chars enclosed in quotation marks
//not needed for now, can use getArgument() which does not trim whitespace
string mdConfig::getArgumentString(string token, int blockStart, int blockEnd) {

	string tempstr = "";
	int line;
	size_t pos;
	size_t quotcnt;

	for (line = blockStart; line <= blockEnd && tempstr == ""; line++) {
	
		pos = cfgLines[line].find(token.data());
		if (pos != string::npos) {
		
			quotcnt = count(cfgLines[line].begin(), cfgLines[line].end(), '"');
			if ((quotcnt & 1) == 1) throw ("Expecting another \" in line " + to_string(line+1));
			tempstr = trimChars(cfgLines[line].substr(pos + token.size()), " ");
		}
	}
	
	pos = tempstr.find(";");					//strip ";" and comment
	if (pos != string::npos) tempstr.erase(pos);
	
	return tempstr;
}


mdBlockConfig::mdBlockConfig(string id) {

	blockConfigID = id;
	baseType = GENERIC;
	useBlkEnd = false;
	blkEndString = "";
	initBlkDefaults = false;
	blkLabelPrefix = "mdb_" + id + "_";
	blkFieldList = nullptr;
	blkFieldCount = 0;
	blkMaxLength = 0;
	
}

mdBlockConfig::~mdBlockConfig() {

	delete[] blkFieldList;
}
