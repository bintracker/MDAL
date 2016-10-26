#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#include "mdalc.h"

using namespace std;


mdField::mdField() {

	isWord = false;
	currentIsString = false;
	currentValue = -1;			//TODO probably does not need to be a class member
	currentValueString = "";		//TODO probably does not need to be a class member

	requiredSeqBegin = false;
	requiredPatBegin = false;
	requiredAlways = false;
	requiredBy = nullptr;
	requiredWhenSet = nullptr;
	
	requiredByAny = true;
	
	setIfCount = 0;
	setIfAlways = nullptr;
	setIfBy = nullptr;
	setIfWhenSet = nullptr;
	setIfByAny = nullptr;
	setIfMask = nullptr;
	setIfClear = nullptr;
	
	setBitsCount = 0;
	setBitsBy = nullptr;
	setBitsMask = nullptr;
	setBitsClear = nullptr;
	
	setBy = -1;
	setHiBy = -1;	
	setLoBy = -1;		
}



mdField::~mdField() {

	for (int i = 0; i < setIfCount; i++) {
	
		delete[] setIfBy[i];
		delete[] setIfWhenSet[i];
	}
	
	delete[] setBitsBy;
	delete[] setBitsMask;
	delete[] setBitsClear;

	delete[] requiredBy;
	delete[] requiredWhenSet;
	delete[] setIfBy;
	delete[] setIfWhenSet;
	delete[] setIfByAny;
	delete[] setIfMask;
	delete[] setIfClear;
	delete[] setIfAlways;
}



void mdField::init(mdCommand *mdCmdList, int &mdCmdCount, string &fieldString, bool &verbose) {

	string temp = fieldString;
	
	if (count(temp.begin(), temp.end(), '(') < 2 || count(temp.begin(), temp.end(), '(') != count(temp.begin(), temp.end(), ')') ||
		(count(temp.begin(), temp.end(), '"') & 1)) throw ("Syntax error in field specification in " + fieldString);
	
	if (temp.find("WORD") == 0) isWord = true;
	else if (temp.find("BYTE") != 0) throw ("Unknown field type specified in " + fieldString);
	
	if (!isWord && temp.find("SET_HI") != string::npos) throw ("SET_HI is not permitted for BYTE fields in " + fieldString);
	
//	if (count(temp.begin(), temp.end(), ',') < 1) throw ("Insufficient arguments in " + fieldString);	
	if (temp.find("SET(") == string::npos && temp.find("SET_HI(") == string::npos && temp.find("SET_LO(") == string::npos 
		&& temp.find("SET_IF(") == string::npos) throw ("Field not set by any command in " + fieldString);
	
	temp.erase(0, temp.find_first_of('(') + 1);
	
	string tmp1 = temp;
// 	tmp1.erase(tmp1.find_first_of(','));
// 	
// 	if (getType(tmp1) == STRING) {
// 		
// 		defaultIsString = true;
// 		defaultString = tmp1;
// 	}
// 	else if (getType(tmp1) == DEC) defaultValue = stoi(tmp1, nullptr, 10);
// 	else if (getType(tmp1) == HEX) defaultValue = stoi(trimChars(tmp1, "$"), nullptr, 16);
// 	else throw ("Field default value type is invalid in " + fieldString);
// 	
// 	if (!isWord && defaultValue > 0xff) throw ("Field default value out of range in " + fieldString);
// 	
// 	temp.erase(0, temp.find_first_of(',') + 1);
	
	requiredBy = new bool[mdCmdCount];
	requiredWhenSet = new bool[mdCmdCount];
	
	for (int i = 0; i < mdCmdCount; i++) {
	
		requiredBy[i] = false;
		requiredWhenSet[i] = true;
	}
	
	
	
	if (temp.find("REQUIRED") != string::npos) {
	
		size_t begin = temp.find("REQUIRED");
	
		tmp1 = temp.substr(begin + 9, temp.find_first_of(',') - (begin + 9) - 1);
		
		size_t slen = tmp1.size() + 10;
		
		if (tmp1.find_first_not_of("()ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!&|") != string::npos) 
			throw ("Invalid argument found in REQUIRED expression in " + fieldString);
		
		if (tmp1.find('&') != string::npos) {
			
			requiredByAny = false;
			if (tmp1.find('|') != string::npos) throw ("Use of both & and | in REQUIRED expression in " + fieldString);
		}
		
		if (tmp1 == "") requiredAlways = true;
		
				
		if (tmp1 != "") {
			
			if (tmp1.size() > 1 && tmp1.substr(0,2) == "!(") {
			//handle global NOT
				for (int i = 0; i < mdCmdCount; i++) requiredWhenSet[i] = false;
				tmp1.erase(0,2);
				tmp1 = trimChars(tmp1, ")");
			}
			
			if (tmp1.find('(') != string::npos) throw ("REQUIRED expression too complex in " + fieldString);
			
			if (tmp1 == "NONE") {
			
				for (int j = 0; j < mdCmdCount; j++) {
				
					requiredWhenSet[j] = false;
					requiredBy[j] = true;
				}
			}
			else if (tmp1 == "ALL") {
			
				requiredByAny = false;
				for (int j = 0; j < mdCmdCount; j++) requiredBy[j] = true;
			}
			else if (tmp1 == "ANY") {
			
				for (int j = 0; j < mdCmdCount; j++) requiredBy[j] = true;
			}
			else {
			
				while (tmp1 != "") {
			
					bool setNot = false;
					string cmdString = "";
				
					if (tmp1.substr(0,1) == "!") {
						setNot = true;
						tmp1.erase(0,1);
					}
				
					cmdString = tmp1.substr(0, tmp1.find_first_of("&|"));

					int cmdNr = getCmdNr(mdCmdList, mdCmdCount, cmdString);
					if (cmdNr == -1) throw ("Unknown command \"" + cmdString + "\" found in REQUIRED expression in " + fieldString);
				
					requiredBy[cmdNr] = true;
					if (setNot) requiredWhenSet[cmdNr] = false;
				
					if (tmp1 != cmdString) tmp1.erase(0, tmp1.find_first_of("&|") + 1);	//or npos
					else tmp1 = "";
				}
			}
		}
		

		
		temp.erase(begin, slen);
	}
	
	
	if (temp.find("SET_HI(") != string::npos) {
		
		size_t begin = temp.find("SET_HI(");
		
		tmp1 = temp.substr(begin + 7, temp.size() - (begin + 7));
		tmp1.erase(tmp1.find_first_of(')'));
		
		int cmdNr = getCmdNr(mdCmdList, mdCmdCount, tmp1);
		if (cmdNr == -1) throw ("Unknown command \"" + tmp1 + "\" found in SET_HI expression in " + fieldString);
		if (mdCmdList[cmdNr].mdCmdType != BYTE) throw ("Non-byte-sized command \"" + tmp1 + "\" cannot be used for SET_HI in " + fieldString);
		
		requiredBy[cmdNr] = true;
		setHiBy = cmdNr;

		temp.erase(begin, tmp1.size() + 8);	
	}
	
	
	if (temp.find("SET_LO(") != string::npos) {
	
		size_t begin = temp.find("SET_LO(");
	
		tmp1 = temp.substr(begin + 7, temp.size() - (begin + 7));
		tmp1.erase(tmp1.find_first_of(')'));
		
		int cmdNr = getCmdNr(mdCmdList, mdCmdCount, tmp1);
		if (cmdNr == -1) throw ("Unknown command \"" + tmp1 + "\" found in SET_LO expression in " + fieldString);
		if (mdCmdList[cmdNr].mdCmdType != BYTE) throw ("Non-byte-sized command \"" + tmp1 + "\" cannot be used for SET_LO in " + fieldString);
		
		requiredBy[cmdNr] = true;
		setLoBy = cmdNr;
		
		temp.erase(begin, tmp1.size() + 8);
	}
	
	
	if (temp.find("SET(") != string::npos) {
	
		size_t begin = temp.find("SET(");
	
		tmp1 = temp.substr(begin + 4, temp.size() - (begin + 4));
		tmp1.erase(tmp1.find_first_of(')'));
		
		int cmdNr = getCmdNr(mdCmdList, mdCmdCount, tmp1);
		if (cmdNr == -1) throw ("Unknown command \"" + tmp1 + "\" found in SET expression in " + fieldString);
		if (mdCmdList[cmdNr].mdCmdType == BOOL) throw ("Boolean command \"" + tmp1 + "\" cannot be used for SET in " + fieldString);
		
		requiredBy[cmdNr] = true;
		setBy = cmdNr;
		
		temp.erase(begin, tmp1.size() + 5);
	}
	
	
	
	for (size_t pos = temp.find("SET_BITS("); pos != string::npos; pos = temp.find("SET_BITS(", pos + 9)) setBitsCount++;
	
	if (setBitsCount) {
	
		setBitsBy = new bool[mdCmdCount];
		fill_n(setBitsBy, mdCmdCount, false);
		setBitsMask = new int[mdCmdCount];
		fill_n(setBitsMask, mdCmdCount, 0);
		setBitsClear = new int[mdCmdCount];
		fill_n(setBitsClear, mdCmdCount, 0);
	}
	
	
	for (int i = 0; i < setBitsCount; i++) {
	
		size_t begin = temp.find("SET_BITS");
	
		tmp1 = temp.substr(begin + 9, temp.size() - (begin + 9));
		tmp1.erase(tmp1.find_first_of(','));
	
		string tmp2 = temp.substr(begin + 9 + tmp1.size() + 1, temp.size() - (begin + 9 + tmp1.size() + 1));
		tmp2.erase(tmp2.find_first_of(",)"));
		if (tmp2 == "") throw ("No bit mask specified in SET_BITS expression in " + fieldString);	//TODO does not reliably detect missing bitmask
		
		string tmp3 = temp.substr(begin + 9 + tmp1.size() + tmp2.size() + 2, temp.size() - (begin + 9 + tmp1.size() + tmp2.size() + 2));
		tmp3.erase(tmp3.find_first_of(")"));
	
		size_t slen = tmp1.size() + 10 + tmp2.size() + 1 + tmp3.size() + 1;
	
		int argtype = getType(tmp2);
		int cmdNr = getCmdNr(mdCmdList, mdCmdCount, tmp1);
	
		if (argtype == DEC) setBitsMask[cmdNr] = stoi(tmp2, nullptr, 10);
		else if (argtype == HEX) setBitsMask[cmdNr] = stoi(trimChars(tmp2, "$"), nullptr, 16);
		else throw ("\"" + tmp2 + "\" is not a valid bit mask in " + fieldString);
		
		//cout << "setBitsMask[" << cmdNr << "] = " << setBitsMask[cmdNr] << endl;	//DEBUG ok
	
		if (cmdNr == -1) throw ("Unknown command \"" + tmp1 + "\" found in SET_BITS expression in " + fieldString);
		if (mdCmdList[cmdNr].mdCmdType != BOOL) throw ("Non-boolean command \"" + tmp1 + "\" cannot be used for SET_BITS in " + fieldString);
	
		requiredBy[cmdNr] = true;
		//cout << "setBits by " << getCmdNr(mdCmdList, mdCmdCount, tmp1) << endl;	//DEBUG, ok
		setBitsBy[cmdNr] = true;
		
		if (tmp3 == "CLEAR") setBitsClear[cmdNr] = CLEAR_ALL;
		else if (tmp3 == "CLEAR_HI") setBitsClear[cmdNr] = CLEAR_HI;
		else if (tmp3 == "CLEAR_LO") setBitsClear[cmdNr] = CLEAR_LO;
		else if (tmp3 != "") throw ("Unknown flag \"" + tmp3 + "\" in SET_BITS expression in " + fieldString);

		temp.erase(begin, slen);
	
		cout << "SET_BITS by " << tmp1 << " with " << setBitsMask[cmdNr] << ", clear " << setBitsClear[cmdNr] << ", temp: " << temp << ", tmp3: " << tmp3 << endl;		//DEBUG
	}
	


	for (size_t pos = temp.find("SET_IF("); pos != string::npos; pos = temp.find("SET_IF(", pos + 7)) setIfCount++;
	
	if (setIfCount) {
	
		setIfBy = new bool*[setIfCount];
		setIfWhenSet = new bool*[setIfCount];
		setIfByAny = new bool[setIfCount];
		fill_n(setIfByAny, setIfCount, true);
		setIfMask = new int[setIfCount];
		fill_n(setIfMask, setIfCount, 0);
		setIfClear = new int[setIfCount];
		fill_n(setIfClear, setIfCount, 0);
		setIfAlways = new bool[setIfCount];
		fill_n(setIfAlways, setIfCount, false);
		
		for (int i = 0; i < setIfCount; i++) {
	
			setIfBy[i] = new bool[mdCmdCount];
			fill_n(setIfBy[i], mdCmdCount, false);
			setIfWhenSet[i] = new bool[mdCmdCount];
			fill_n(setIfWhenSet[i], mdCmdCount, true);
		}
	}
	
	
	for (int i = 0; i < setIfCount; i++) {
	
		size_t begin = temp.find("SET_IF");
	
		//tmp1 = temp.substr(begin + 9, temp.find_first_of(',') - (begin + 9));
		tmp1 = temp.substr(begin + 7, temp.size() - (begin + 7));
		tmp1.erase(tmp1.find_first_of(','));
		
		string tmp2 = temp.substr(begin + 7 + tmp1.size() + 1, temp.size() - (begin + 7 + tmp1.size() + 1));
		tmp2.erase(tmp2.find_first_of(",)"));
		if (tmp2 == "") throw ("No bit mask specified in SET_IF expression in " + fieldString);	//TODO does not reliably detect missing bitmask
		
		string tmp3 = temp.substr(begin + 7 + tmp1.size() + tmp2.size() + 2, temp.size() - (begin + 7 + tmp1.size() + tmp2.size() + 2));
		tmp3.erase(tmp3.find_first_of(",)"));
	
		size_t slen = tmp1.size() + 8 + tmp2.size() + 1 + tmp3.size() + 1;
		
		int argtype = getType(tmp2);
		
		if (argtype == DEC) setIfMask[i] = stoi(tmp2, nullptr, 10);
		else if (argtype == HEX) setIfMask[i] = stoi(trimChars(tmp2, "$"), nullptr, 16);
		else throw ("\"" + tmp2 + "\" is not a valid bit mask in " + fieldString);
		
		if (tmp3 == "CLEAR") setIfClear[i] = CLEAR_ALL;
		else if (tmp3 == "CLEAR_HI") setIfClear[i] = CLEAR_HI;
		else if (tmp3 == "CLEAR_LO") setIfClear[i] = CLEAR_LO;
		else if (tmp3 != "") throw ("Unknown flag \"" + tmp3 + "\" in SET_IF expression in " + fieldString);
		
 		if (tmp1.find_first_not_of("()ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!&|$") != string::npos) 
 			throw ("Invalid argument found in SET_IF expression in " + fieldString);
		
		if (tmp1.find('&') != string::npos) {
			
			setIfByAny[i] = false;
			if (tmp1.find('|') != string::npos) throw ("Use of both & and | in SET_IF expression in " + fieldString);
		}
		
		if (tmp1 == "") setIfAlways[i] = true;
		
				
		if (tmp1 != "") {
			
			if (tmp1.size() > 1 && tmp1.substr(0,2) == "!(") {
			//handle global NOT
				for (int j = 0; j < mdCmdCount; j++) setIfWhenSet[i][j] = false;
				tmp1.erase(0,2);
				tmp1 = trimChars(tmp1, ")");
			}
			
			if (tmp1.find('(') != string::npos) throw ("SET_IF expression too complex in " + fieldString);
			
			if (tmp1 == "NONE") {
			
				for (int j = 0; j < mdCmdCount; j++) {
				
					setIfWhenSet[i][j] = false;
					setIfBy[i][j] = true;
					setIfByAny[i] = false;
				}
			}
			else if (tmp1 == "ALL") {
			
				setIfByAny[i] = false;
				for (int j = 0; j < mdCmdCount; j++) setIfBy[i][j] = true;
			}
			else if (tmp1 == "ANY") {
			
				for (int j = 0; j < mdCmdCount; j++) setIfBy[i][j] = true;
			}
			else {
			
				while (tmp1 != "") {
			
					bool setNot = false;
					string cmdString = "";
				
					if (tmp1.substr(0,1) == "!") {
						setNot = true;
						tmp1.erase(0,1);
					}
				
					cmdString = tmp1.substr(0, tmp1.find_first_of("&|"));

					int cmdNr = getCmdNr(mdCmdList, mdCmdCount, cmdString);
					if (cmdNr == -1) throw ("Unknown command \"" + cmdString + "\" found in SET_IF expression in " + fieldString);
				
					setIfBy[i][cmdNr] = true;
					if (setNot) setIfWhenSet[i][cmdNr] = false;
				
					if (tmp1 != cmdString) tmp1.erase(0, tmp1.find_first_of("&|") + 1);	//or npos
					else tmp1 = "";
				}
			}
 		}
		
		temp.erase(begin, slen);
	}

	if (trimChars(temp, ",;)") != "") throw ("Could not evaluate field " + fieldString);	
}



int mdField::getCmdNr(mdCommand *mdCmdList, int &mdCmdCount, string &cmdString) {

	int cnr = -1;
	
	for (int i = 0; i < mdCmdCount; i++) if (cmdString == mdCmdList[i].mdCmdName) cnr = i;
		
	return cnr;
}


void mdField::getRequests(bool *requestList, const mdConfig &config, const int &row, bool seqBegin) {

	isRequiredNow = false;
	
	if ((requiredSeqBegin && seqBegin && row == 0) || (requiredPatBegin && row == 0) || requiredAlways
 		|| checkCondition(requiredBy, requiredWhenSet, requiredByAny, config)) isRequiredNow = true;
	if (setBy != -1 && (config.mdCmdList[setBy].mdCmdCurrentValString != "" || config.mdCmdList[setBy].mdCmdCurrentVal != -1))
		isRequiredNow = true;
	if (setHiBy != -1 && (config.mdCmdList[setHiBy].mdCmdCurrentValString != "" || config.mdCmdList[setHiBy].mdCmdCurrentVal != -1))
		isRequiredNow = true;
	if (setLoBy != -1 && (config.mdCmdList[setLoBy].mdCmdCurrentValString != "" || config.mdCmdList[setLoBy].mdCmdCurrentVal != -1))
		isRequiredNow = true;
		
 	for (int i = 0; i < config.mdCmdCount; i++) {
	
		//cout << "setBitsCount: " << setBitsCount << endl;	//DEBUG ok
	
		if (setBitsCount && setBitsBy[i] && config.mdCmdList[i].mdCmdCurrentVal > -1) {
 		
// 			cout << "SetBits requires... " << endl;		//DEBUG
			isRequiredNow = true;
			requestList[i] = true;
 		}
 	}
		
	if (!isRequiredNow) return;

	if (setBy != -1) requestList[setBy] = true;
	if (setHiBy != -1) requestList[setHiBy] = true;
	if (setLoBy != -1) requestList[setLoBy] = true;
	

	//cout << boolalpha << requestList[1] << endl;
}


string mdField::getFieldString(bool *requestList, const mdConfig &config) {

	//cout << boolalpha << requestList[1] << endl;
	
	string fstr = "";

	currentIsString = false;
	currentValue = -1;
	currentValueString = "";
	
// 	isRequiredNow = false;
// 	
// 	
// 	if ((requiredSeqBegin && seqBegin && row == 0) || (requiredPatBegin && row == 0) || requiredAlways
//  		|| checkCondition(requiredBy, requiredWhenSet, requiredByAny, config)) isRequiredNow = true;
// 	if (setBy != -1 && (config.mdCmdList[setBy].mdCmdCurrentValString != "" || config.mdCmdList[setBy].mdCmdCurrentVal != -1))
// 		isRequiredNow = true;
// 	if (setHiBy != -1 && (config.mdCmdList[setHiBy].mdCmdCurrentValString != "" || config.mdCmdList[setHiBy].mdCmdCurrentVal != -1))
// 		isRequiredNow = true;
// 	if (setLoBy != -1 && (config.mdCmdList[setLoBy].mdCmdCurrentValString != "" || config.mdCmdList[setLoBy].mdCmdCurrentVal != -1))
// 		isRequiredNow = true;

	
	
	if (!isRequiredNow) return fstr;
	
	//cout << "is required" << endl;
	
	bool hiWasSet = false;
	bool hiWasSetStr = false;
	
	if (setBy != -1) {
	
		currentValueString = config.mdCmdList[setBy].getValueString();
		currentValue = config.mdCmdList[setBy].getValue();
	
	}
	
	if (setHiBy != -1) {
	
		currentValueString = config.mdCmdList[setHiBy].getValueString();
		if (currentValueString != "") {
			currentValueString += "*256+";
			hiWasSetStr = true;
		}
		else {
			currentValue = config.mdCmdList[setHiBy].getValue() * 256;
			hiWasSet = true;
		}
	}
	
	if (setLoBy != -1) {
	
		currentValueString += config.mdCmdList[setLoBy].getValueString();
		
		if (config.mdCmdList[setLoBy].getValueString() == "") {
		
			if (hiWasSet) currentValue |= config.mdCmdList[setLoBy].getValue();
			else if (hiWasSetStr) {
				stringstream val;
				val << hex << config.mdCmdList[setLoBy].getValue();
				currentValueString += config.hexPrefix + val.str();
			}
			else currentValue = config.mdCmdList[setLoBy].getValue();
		}
	}
	
	
	for (int i = 0; i < config.mdCmdCount && setBitsCount; i++) {
	
		if (setBitsBy[i]) {
	
			if (currentValue == -1 || setBitsClear[i] == CLEAR_ALL) currentValue = 0;
			else if (setBitsClear[i] == CLEAR_HI) currentValue &= 0xff;
			else if (setBitsClear[i] == CLEAR_LO) currentValue &= 0xff00;
		
// 			cout << "Requesting " << i << ": " << config.mdCmdList[i].mdCmdName << endl;
// 			cout << "NC returns " << config.mdCmdList[i].getValue() << endl;		//DEBUG -> Dafuq? Returns A instead of NC
// 			cout << hex << "currentValue: " << currentValue << endl;
		
			if (config.mdCmdList[i].getValue() > 0) {
				//TODO: support STRING cmds -> same with setIf
				currentValue |= setBitsMask[i];
			}
			
//			cout << "setBitsMask: " << setBitsMask[i] << ", currentValue: " << currentValue << dec << endl;	
		}	
	}
	
	
	//TODO: still requires a solution for mixed string/int results
	
	for (int i = 0; i < setIfCount; i++) {
	
		
	
		//TODO: this needs a new function because we should check again requestList!!!
		if (checkSetifCondition(setIfBy[i], setIfWhenSet[i], setIfByAny[i], config, requestList) || setIfAlways[i]) {
		
//			cout << hex << "Set_If " << i << " check requested, condition true, " << setIfMask[i] << " masked in." << dec << endl;	//DEBUG
		
			if (currentValue == -1 || setIfClear[i] == CLEAR_ALL) currentValue = 0;
			else if (setIfClear[i] == CLEAR_HI) currentValue &= 0xff;
			else if (setIfClear[i] == CLEAR_LO) currentValue &= 0xff00;
			
			currentValue |= setIfMask[i];
		}
	}	
	
	if (currentValueString != "") fstr = currentValueString;
	else {

		stringstream val;
		val << hex << currentValue;
		fstr = config.hexPrefix + val.str();
	}	

	//cout << "ok\n";
	return fstr;
}



bool mdField::checkSetifCondition(const bool *by, const bool *whenSet, bool &byAny, const mdConfig &config, bool *requestList) {
	
	if (byAny) {
	
		for (int cmd = 0; cmd < config.mdCmdCount; cmd++) {
		
			if (by[cmd] && requestList[cmd] == whenSet[cmd]) return true;
		}
	
		return false;
	}
	else {
	
		for (int cmd = 0; cmd < config.mdCmdCount; cmd++) {

			if (by[cmd] && requestList[cmd] != whenSet[cmd]) return false;
		}
	
	}

	return true;
}


bool mdField::checkCondition(const bool *by, const bool *whenSet, bool &byAny, const mdConfig &config) {
	
	if (byAny) {
	
		for (int cmd = 0; cmd < config.mdCmdCount; cmd++) {
		
			if (by[cmd] && config.mdCmdList[cmd].mdCmdIsSetNow == whenSet[cmd]) return true;
		}
	
		return false;
	}
	else {
	
		for (int cmd = 0; cmd < config.mdCmdCount; cmd++) {

			if (by[cmd] && config.mdCmdList[cmd].mdCmdIsSetNow != whenSet[cmd]) return false;
		}
	
	}

	return true;
}