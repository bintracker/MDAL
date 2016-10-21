#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>

#include "mdalc.h"

using namespace std;



mdConfig::mdConfig(string &configname, bool &verbose) {

	cfgLines = nullptr;
	mdCmdList = nullptr;
	mdFieldList = nullptr;
	
	string filename = "config/" + configname + ".cfg";

	ifstream CFGFILE(filename.data());
	if (!CFGFILE.is_open()) throw ("Unknown configuration \"" + configname + "\".");

	if (verbose) cout << "configuration:\t\t" << configname << ".cfg" << endl;
	
	string tempstr;
	
	for (linecount = 0; getline(CFGFILE,tempstr); linecount++);
	int configEnd = linecount - 1;
	
	CFGFILE.clear();						//reset file pointer
	CFGFILE.seekg(0, ios::beg);
	
	cfgLines = new string[linecount];
	
	for (int i = 0; i < linecount; i++) getline(CFGFILE,cfgLines[i]);
	
	if (locateToken(string("MDAL_VERSION"), 0, configEnd) == configEnd) throw ("MDAL_VERSION not specified in " + configname);
	string mdVersion = trimChars(getArgumentString(string("MDAL_VERSION"), 0, linecount-1), "()");
	if (getType(mdVersion) != DEC) throw ("Invalid MDAL_VERSION specification in " + configname);
	if (stoi(mdVersion, nullptr, 10) > MDALVERSION) throw ("MDAL VERSION used in " + configname + ".cfg not supported in this version of mdalc");
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
	
	useSequence = false;
	if (locateToken(string("USE_SEQUENCE"), 0, configEnd) != configEnd) {
	//TODO: provide support for multi-track sequences
	
		useSequence = true;
		if (verbose) cout << "using SEQUENCE" << endl;
		
		int blockStart = locateToken(string("CFG_SEQUENCE"), 0, configEnd);
		
		if (blockStart == configEnd) throw ("No CFG_SEQUENCE block found in " + configname + ".cfg");
		
		else {
			
			int blockEnd = getBlockEnd(blockStart);
			
			useSeqEnd = false;
			useSeqLoop = false;
			useSeqLoopPointer = false;
			
			
			int tokenpos = locateToken(string("USE_END"), blockStart, blockEnd);
			
			if (tokenpos != blockEnd) {
			
				useSeqEnd = true;
				seqEndString = trimChars(getArgument(cfgLines[tokenpos], 1), "\"");
				if (seqEndString == "") throw ("Sequence end string not specified in " + configname + ".cfg");
 				if (verbose) cout << "Sequence end:\t\t" << seqEndString << endl;
			}
			
			
			tokenpos = locateToken(string("USE_LOOP"), blockStart, blockEnd);
			
			if (tokenpos != blockEnd) {
			
				useSeqLoop = true;
				
				int argCount = getArgumentCount(cfgLines[tokenpos]);
				
				if (argCount < 2) throw ("Incomplete loop configuration in " + configname + ".cfg");
				
				string arg = getArgument(cfgLines[tokenpos], 1);
				//cout << arg << endl;
				
				if (arg != "LABEL" && arg != "POINTER") throw ("Invalid loop type specified in " + configname + ".cfg");
				else if (arg == "POINTER") useSeqLoopPointer = true;
				
				seqLoopLabel = trimChars(getArgument(cfgLines[tokenpos], 2), "\"");
				
				if (verbose) cout << "Loop type:\t\t" << arg << "\nLoop label:\t\t" << seqLoopLabel << endl;
			}
		}
	}
	
	usePatterns = false;
	
	if (locateToken(string("USE_PATTERNS"), 0, configEnd) != configEnd) {
	
		useSequence = true;
		if (verbose) cout << "using PATTERNS" << endl;
		
		int blockStart = locateToken(string("CFG_PATTERNS"), 0, configEnd);
		
		if (blockStart == configEnd) throw ("No CFG_PATTERNS block found in " + configname + ".cfg");
		
		else {
			
			int blockEnd = getBlockEnd(blockStart);
			usePtnEnd = false;
		
			int tokenpos = locateToken(string("USE_END"), blockStart, blockEnd);
			
			if (tokenpos != blockEnd) {
			
				usePtnEnd = true;
				ptnEndString = trimChars(getArgument(cfgLines[tokenpos], 1), "\"");
				if (ptnEndString == "") throw ("Pattern end string not specified in " + configname + ".cfg");
 				if (verbose) cout << "Pattern end:\t\t" << ptnEndString << endl;
			}
			
			
			initPtnDefaults = false;
			
			tokenpos = locateToken(string("INIT_DEFAULTS"), blockStart, blockEnd);
			
			if (tokenpos != blockEnd) {
			
				initPtnDefaults = true;
 				if (verbose) cout << "Initialize commands with default values at each pattern start" << endl;
			}
		}
	}
	
	useTables = false;
	if (locateToken(string("USE_TABLES"), 0, configEnd) != configEnd) {
	
		useTables = true;
		if (verbose) cout << "using TABLES" << endl;
	}
	
	useSamples = false;
	if (locateToken(string("USE_SAMPLES"), 0, configEnd) != configEnd) {
	
		useSamples = true;
		if (verbose) cout << "using SAMPLES - this feature is not supported yet." << endl;
	}
	
	
	
	int blockStart = locateToken(string("CFG_COMMANDS"), 0, configEnd);
		
	if (blockStart == linecount - 1) throw ("No CFG_COMMANDS block found in " + configname + ".cfg");
		
	int blockEnd = getBlockEnd(blockStart);
	blockStart++;					//TODO: modify locateToken/getBlockEnd so we don't have to do this shit every time
	blockEnd--;
	mdCmdCount = countBlockLines(blockStart, blockEnd);
	if (!mdCmdCount) throw ("No commands specified in CFG_COMMAND block in " + configname + ".cfg");
	
	mdCmdList = new mdCommand[mdCmdCount];
	int cmdNr = 0;
	
	for (int i = blockStart; i < blockEnd; i++) {
	
		string cmdStr = cfgLines[i];
		cmdStr = trimChars(cmdStr, " \t");	//TODO: this trims all whitespace, don't trim whitespace within cmd string params!
		size_t pos = cmdStr.find_first_of('/');
		if (pos != string::npos) cmdStr.erase(pos);
		if (cmdStr != "") {
			mdCmdList[cmdNr].init(cmdStr, verbose);
			cmdNr++;
			//cout << cmdStr << ".\n";
		}
	}
	
	
	
	blockStart = locateToken(string("CFG_FIELDS"), 0, configEnd);
	
	if (blockStart == configEnd) throw ("No CFG_FIELDS block found in " + configname + ".cfg");
	
	blockEnd = getBlockEnd(blockStart);
	blockStart++;					//TODO: modify locateToken/getBlockEnd so we don't have to do this shit every time
	blockEnd--;
	mdFieldCount = countFieldBlockLines(blockStart, blockEnd);
	if (!mdFieldCount) throw ("No commands specified in CFG_FIELDS block in " + configname + ".cfg");
	
	
	mdFieldList = new mdField[mdFieldCount];
	int fieldNr = 0;
	string requirePtnBeginStr = "";
	string requireSeqBeginStr = "";
	
	for (int i = blockStart; i < blockEnd; i++) {
	
		string fieldStr = cfgLines[i];
		fieldStr = trimChars(fieldStr, " \t");	//TODO: this trims all whitespace, don't trim whitespace within cmd string params!
		size_t pos = fieldStr.find_first_of('/');
		if (pos != string::npos) fieldStr.erase(pos);
		if (fieldStr != "" && (fieldStr.find("WORD") != string::npos || fieldStr.find("BYTE") != string::npos)) {
		
			mdFieldList[fieldNr].init(mdCmdList, mdCmdCount, fieldStr, verbose);
			
			if (verbose) {
			
				cout << "Field " << fieldNr;
				if (mdFieldList[fieldNr].requiredAlways) cout << " always required";
				else {
					cout << " required if ";
					for (int j = 0; j < mdCmdCount; j++) {
				
						if (mdFieldList[fieldNr].requiredBy[j]) {
					
							cout << mdCmdList[j].mdCmdName;
							if (mdFieldList[fieldNr].requiredWhenSet[j]) cout << " is set ";
							else cout << " is not set ";
						
							if (mdFieldList[fieldNr].requiredByAny && j + 1 < mdCmdCount) cout << "or ";
							else if (mdFieldList[fieldNr].requiredByAny && j + 1 < mdCmdCount) cout << "and ";
							//else cout << endl;
						}
					}
				}
				
				cout << endl;
			}
			
			fieldNr++;
		}
		
		if (fieldStr.find("REQUIRE_SEQ_BEGIN") != string::npos) requireSeqBeginStr = fieldStr;
		if (fieldStr.find("REQUIRE_PTN_BEGIN") != string::npos) requirePtnBeginStr = fieldStr;
	}
	
	
	//TODO: currently only checks presence of token, expr is not evaluated
	
	if (requireSeqBeginStr != "") {
	
		for (int i = 0; i < mdFieldCount; i++) mdFieldList[i].requiredSeqBegin = true;
	}
	
	if (requirePtnBeginStr != "") {
	
		for (int i = 0; i < mdFieldCount; i++) mdFieldList[i].requiredPatBegin = true;		//TODO: PatBegin is inconsistently named
	
	}

	
	return;
}

mdConfig::~mdConfig() {

	delete[] mdFieldList;
	delete[] mdCmdList;
	delete[] cfgLines;

}



//count lines in the field definition block, omitting comments, empty lines, and global commands
int mdConfig::countFieldBlockLines(int &blockStart, int &blockEnd) {

	int cmdlines = 0;
	
	for (int line = blockStart; line < blockEnd; line++) {
	
		string temp = cfgLines[line];
		temp = trimChars(temp, " \t");
		size_t pos = temp.find_first_of('/');
		if (pos != string::npos) temp.erase(pos);
		
		if (temp != "" && (temp.find("WORD") != string::npos || temp.find("BYTE") != string::npos)) cmdlines++;
		
		//cout << "line: " << cmdlines << " = " << temp << endl;
	}
	
	
	return cmdlines;
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
	
	return line;
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
			if ((quotcnt & 1) == 1) throw ("Expecting another \" in line " + to_string(line+1) + " of config file");
			tempstr = trimChars(cfgLines[line].substr(pos + token.size()), " ");
		}
	}
	
	pos = tempstr.find(";");					//strip ";" and comment
	if (pos != string::npos) tempstr.erase(pos);
	
	return tempstr;
}
