#include <iostream>
#include <string>
#include <algorithm>

#include "mdalc.h"

using namespace std;

mdCommand::mdCommand(): referenceBlkID(""), mdCmdDefaultValString(""), mdCmdAutoValString(""), mdCmdCurrentValString(""), mdCmdLastValString("") {

	mdCmdType = BOOL;
	mdCmdGlobalConst = false;
	
	useNoteNames = false;
	allowModifiers = false;

	mdCmdForceSubstitution = false;
	mdCmdForceString = false;
	mdCmdForceInt = false;
	mdCmdForceRepeat = false;
	mdCmdUseLastSet = false;
	mdCmdAuto = false;
	
	isBlkReference = false;
	defaultSubstitute = nullptr;

	mdCmdIsSetNow = false;
	
	limitRange = false;
	lowerRangeLimit = 0;
	upperRangeLimit = 0;
}

mdCommand::~mdCommand() {}


void mdCommand::init(const string &commandString, bool &verbose) {

	//primary validation
	
	string temp = commandString;
	string cmdStrCopy = commandString;
	bool isValid = true;
	
	if (count(temp.begin(), temp.end(), '(') < 1 || count(temp.begin(), temp.end(), ')') < 1 || count(temp.begin(), temp.end(), ',') < 1) isValid = false;
	if (count(temp.begin(), temp.end(), '(') != count(temp.begin(), temp.end(), ')')) isValid = false;
	if (count(temp.begin(), temp.end(), '"') < 2 || (count(temp.begin(), temp.end(), '"') & 1) == 1) isValid = false;
	if (!isValid) throw ("Faulty Syntax in CFG_COMMANDS: " + commandString);
	
	temp.erase(temp.find('('));	//TODO: also strip potential trailing whitespace
	if (temp == "BOOL") mdCmdType = BOOL;
	else if (temp == "BYTE") mdCmdType = BYTE;
	else if (temp == "WORD") mdCmdType = WORD;
	else throw("\"" + temp + "\" does not name a known command type in " + commandString);
	
	cmdStrCopy.erase(0,cmdStrCopy.find('(')+1);
	
	temp = cmdStrCopy;
	temp.erase(temp.find(','));
	if (temp.compare(0, 1, "\"") != 0) throw ("\"" + temp + "\" is not a valid command name in " + commandString);
	mdCmdName = trimChars(temp, " \"");
	//TODO: check for more reserved keywords
	if (mdCmdName == "NONE" || mdCmdName == "ANY" || mdCmdName == "ALL" || mdCmdName == "CONFIG") 
		throw ("Reserved keyword \"" + mdCmdName + "\" used as command name in " + commandString);
	
	cmdStrCopy.erase(0,cmdStrCopy.find(',')+1);
	temp = cmdStrCopy;
	if (cmdStrCopy.find(',') != string::npos) temp.erase(temp.find(','));
	else temp.erase(temp.find(')'));
	
	try {
		setDefault(temp);
	}
	catch(string &e) {
		throw (e + " in " + commandString);
	}
	
	
	if (cmdStrCopy.find("AUTO") != string::npos) {
		
		mdCmdAuto = true;
	
		temp = cmdStrCopy;
		temp.erase(0, temp.find("AUTO")+4);
		if (temp.find_first_of('(') == string::npos) throw ("AUTO enabled, but no auto replacement value specified in " + commandString);
		temp.erase(0, temp.find_first_not_of("(\n\t"));
		temp.erase(temp.find_first_of(')'));
		
		if (getType(temp) == BOOL && mdCmdType != BOOL) throw ("Non-Boolean parameter specified for BOOL AUTO command in " + commandString);
		mdCmdAutoValString = temp;
	}
	
	
	if (cmdStrCopy.find(',') != string::npos) cmdStrCopy.erase(0,temp.find(',')+1);		//not really necessary
	
	
	if (cmdStrCopy.find("FORCE_SUBSTITUTION") != string::npos) {
	
		mdCmdForceSubstitution = true;	//TODO: this one has sub-args
		
		temp = cmdStrCopy;
		temp.erase(0, temp.find("FORCE_SUBSTITUTION")+18);
		if (temp.find_first_of('(') == string::npos) throw ("FORCE_SUBSTITUTION enabled, but no substitution list specified in " + commandString);
		temp.erase(0, temp.find_first_not_of("(\n\t"));
		temp.erase(temp.find_first_of(')'));
		
		if (count(temp.begin(), temp.end(), '=') * 2 != count(temp.begin(), temp.end(), '"'))
			throw ("Invalid substitution list specification in " + commandString);
		
		int listlen = count(temp.begin(), temp.end(), ',') + 1;
		
		for (int i = 0; i < listlen; i++) {
		
			substitutionList.insert(make_pair(trimChars(temp.substr(0, temp.find('=')), "\""), 
				trimChars(temp.substr(temp.find('=') + 1, temp.find_first_of(",)") - (temp.find('=') + 1)), " \"")));
				
			//TODO check substitution parameter against FORCE_INT/STRING
				
			temp.erase(0, temp.find_first_of(",)") + 1);
		}	
	}
	
	if (cmdStrCopy.find("REFERENCE") != string::npos) {
	
		isBlkReference = true;
		
		temp = cmdStrCopy;
		temp.erase(0, temp.find("REFERENCE")+9);
		if (temp.find_first_of('(') == string::npos) throw ("Command specified as REFERENCE, but no reference block ID given in " + commandString);
		temp.erase(0, temp.find_first_not_of("(\n\t"));
		temp.erase(temp.find_first_of(')'));
		referenceBlkID = trimChars(temp, "\"");
	}
	
	
	size_t pos = cmdStrCopy.find("RANGE");
	if (pos != string::npos) {
	
		limitRange = true;
		temp = cmdStrCopy;
		temp.erase(0, pos+5);
		if (temp.find_first_of('(') != 0) throw ("RANGE enabled, but no limits specified in " + commandString);
		temp.erase(0, 1);
		string arg1 = temp.erase(temp.find_first_of(')'));
		string arg2 = arg1;
		
		if (arg1.find_first_of(',') != string::npos) arg1.erase(arg1.find_first_of(','));
		else throw ("Insufficient arguments for RANGE in " + commandString);
		arg2.erase(0, arg2.find_first_of(',') + 1);
		
		if (arg1 == "" || arg2 == "") throw ("Insufficient arguments for RANGE in " + commandString);
		
		if (getType(arg1) == DEC) lowerRangeLimit = stoi(arg1, nullptr, 10);
		else if (getType(arg1) == HEX) lowerRangeLimit = stoi(trimChars(arg1, "$"), nullptr, 16);
		else throw ("Lower RANGE limit is not a number in " + commandString);
		
		if (getType(arg2) == DEC) upperRangeLimit = stoi(arg2, nullptr, 10);
		else if (getType(arg2) == HEX) upperRangeLimit = stoi(trimChars(arg2, "$"), nullptr, 16);
		else throw ("Upper RANGE limit is not a number in " + commandString);
		
		//cout << "Range: " << lowerRangeLimit << ".." << upperRangeLimit << endl;
	
	}
	
	if (cmdStrCopy.find("USE_NOTE_NAMES") != string::npos) useNoteNames = true;
	if (cmdStrCopy.find("ALLOW_MODIFIERS") != string::npos) allowModifiers = true;
	if (cmdStrCopy.find("FORCE_STRING") != string::npos) mdCmdForceString = true;
	if (cmdStrCopy.find("FORCE_INT") != string::npos) mdCmdForceInt = true;
	if (cmdStrCopy.find("FORCE_REPEAT") != string::npos) mdCmdForceRepeat = true;
	if (cmdStrCopy.find("USE_LAST_SET") != string::npos) mdCmdUseLastSet = true;
	if (cmdStrCopy.find("GLOBAL_CONST") != string::npos) mdCmdGlobalConst = true;
	
	if (mdCmdForceString && mdCmdForceInt) throw ("FORCE_INT and FORCE_STRING are mutually exclusive in " + commandString);

	
	if (verbose) {
		cout << mdCmdName << ":\t Type is ";

		if (mdCmdType == BYTE) cout << "BYTE";
		else if (mdCmdType == BOOL) cout << "BOOL";
		else cout << "WORD";

		cout << ", default is ";
		if (defaultSubstitute == nullptr) cout << mdCmdDefaultValString;
		else cout << "substituted by " << defaultSubstitute->mdCmdName;

		//TODO formatting is odd with FORCE_SUBSTITUTION
		if (useNoteNames) cout << ", USE_NOTE_NAMES";
		if (allowModifiers) cout << ", ALLOW_MODIFIERS";
		if (mdCmdForceString) cout << ", FORCE_STRING";
		if (mdCmdForceInt) cout << ", FORCE_INT";
		if (mdCmdForceRepeat) cout << ", FORCE_REPEAT";
		if (mdCmdUseLastSet) cout << ", USE_LAST_SET";
		if (mdCmdGlobalConst) cout << ", GLOBAL_CONST";
		if (mdCmdAuto) cout << ", AUTO";
		if (limitRange) cout << ", RANGE: " << lowerRangeLimit << ".." << upperRangeLimit;
		if (isBlkReference) cout << ", REFERENCE to " << referenceBlkID;
		if (mdCmdForceSubstitution) {
		
			cout << ", FORCE_SUBSTITUTION: " << endl;
			for (auto&& it: substitutionList) cout << "\t" << it.first << " ==> " << it.second << endl;
		}
		else cout << endl;
	}
}


void mdCommand::resetToDefault() {		//TODO: keep an eye on this to see if it really is useful

	if (!mdCmdGlobalConst) {
	
		mdCmdIsSetNow = false;
		mdCmdCurrentValString = "";
		mdCmdLastValString = getDefaultValString();
		
		if (mdCmdForceRepeat) {
		
			mdCmdIsSetNow = true;
			mdCmdCurrentValString = mdCmdLastValString;
		}	
	}
}


void mdCommand::reset() {

	if (!mdCmdGlobalConst) {
	
		if ((mdCmdUseLastSet || mdCmdForceRepeat) && mdCmdIsSetNow) mdCmdLastValString = mdCmdCurrentValString;

		if (!mdCmdForceRepeat) {
		
			mdCmdIsSetNow = false;
			mdCmdCurrentValString = "";
		}
		else {
		
			mdCmdIsSetNow = true;
			if (mdCmdLastValString == "") mdCmdLastValString = getDefaultValString();
			mdCmdCurrentValString = mdCmdLastValString;
		}
	}
}


void mdCommand::set(const string &currentValString) {

	//TODO: if string is number, get value and check against BYTE/WORD range
	if (mdCmdGlobalConst) throw (string("Global constant redefined in block "));
	
	mdCmdIsSetNow = true;
	
	if (mdCmdAuto) {
		
		mdCmdCurrentValString = mdCmdAutoValString;
		return;
	}
	
	int paramType = getType(currentValString);
	
	if (mdCmdForceInt && (paramType != DEC && paramType != HEX))
		throw (string("String argument supplied for integer command "));
	//TODO: does not check for bool
	if (mdCmdForceString && isNumber(currentValString)) throw (string("Integer argument supplied for string command "));
	
	if (limitRange && isNumber(currentValString)) {
	
		if (strToNum(currentValString) < lowerRangeLimit || strToNum(currentValString) > upperRangeLimit) 
			throw (string("Argument out of range for command "));
	}
	
	mdCmdCurrentValString = currentValString;

	if (mdCmdForceSubstitution) {

		if (substitutionList.count(currentValString)) mdCmdCurrentValString = substitutionList[currentValString];
		//TODO paramType == STRING is unreliable, will pass bool but fail non-quote-enclosed strings
		else if (paramType == STRING) throw ("\"" + currentValString + "\" is not a valid parameter for command ");
	}
	
	mdCmdLastValString = mdCmdCurrentValString;
}


string mdCommand::getValueString() {

	if (mdCmdIsSetNow) return mdCmdCurrentValString;
	if (mdCmdUseLastSet && mdCmdLastValString != "") return mdCmdLastValString;
	return getDefaultValString();
}


string mdCommand::getDefaultValString() {

	return (defaultSubstitute == nullptr) ? mdCmdDefaultValString : defaultSubstitute->mdCmdDefaultValString;
}


void mdCommand::setDefault(const string &param) {

	//TODO is substitution on this completed?
	if (param.find("SUBSTITUTE_FROM") != string::npos) return;
	
	int paramType = getType(param);

	if (paramType == INVALID) throw ("\"" + param + "\" is not a valid argument ");
	if (paramType == BOOL && mdCmdType != BOOL) throw (string("Non-Boolean default parameter specified for BOOL command"));
	
	mdCmdDefaultValString = trimChars(param, "\"");
	
	int paramVal = -1;
	
	if (paramType == DEC) paramVal = static_cast<int>(stoul(trimChars(param, " "), nullptr, 10));
	else if (paramType == HEX) paramVal = static_cast<int>(stoul(trimChars(param, " $"), nullptr, 16));

	
	if ((mdCmdType == WORD && paramVal > 0xffff) || (mdCmdType == BYTE && paramVal > 0xff))
		throw (string("Default value out of range "));
}
