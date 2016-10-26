#include <iostream>
#include <string>
#include <algorithm>

#include "mdalc.h"

using namespace std;


mdCommand::mdCommand() {

	mdCmdType = BOOL;			//0 = BOOL, 1 = BYTE, 2 = WORD
	mdCmdGlobalConst = false;

	mdCmdForceSubstitution = false;
	mdCmdSubstitutionListLength = 0;
	mdCmdForceString = false;
	mdCmdForceRepeat = false;
	mdCmdUseLastSet = false;
	

	mdCmdDefaultVal = -1;
	mdCmdDefaultValString = "";
	
	mdCmdLastVal = -1;
	mdCmdLastValString = "";	
	
	mdCmdIsSetNow = false;
	mdCmdCurrentVal = -1;
	mdCmdCurrentValString = "";

	
	mdCmdSubstitutionNames = nullptr;
	mdCmdSubstitutionValues = nullptr;

}

mdCommand::~mdCommand() {
	delete[] mdCmdSubstitutionNames;
	delete[] mdCmdSubstitutionValues;
}


void mdCommand::init(string commandString, bool &verbose) {

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
	else throw("\"" + temp + "\" does not name a known type in " + commandString);
	
	cmdStrCopy.erase(0,cmdStrCopy.find('(')+1);
	
	temp = cmdStrCopy;
	temp.erase(temp.find(','));
	if (temp.find('"') != 0) throw ("\"" + temp + "\" is not a valid command name in " + commandString);
	mdCmdName = trimChars(temp, " \"");
	if (mdCmdName == "NONE" || mdCmdName == "ANY" || mdCmdName == "ALL" || mdCmdName == "CONFIG") 
		throw ("Reserved keyword \"" + mdCmdName + "\" used as command name in " + commandString);
	//cout << mdCmdName << endl;
	
	cmdStrCopy.erase(0,cmdStrCopy.find(',')+1);
	temp = cmdStrCopy;
	if (cmdStrCopy.find(',') != string::npos) temp.erase(temp.find(','));
	else temp.erase(temp.find(')'));
	
	
	if (getType(temp) == INVALID) throw ("\"" + temp + "\" is not a valid argument in " + commandString);
	
	if (getType(temp) == BOOL) {
	
		if (mdCmdType != BOOL) throw ("Default value does not match command type in " + commandString);
		
		if (temp == "true") mdCmdDefaultVal = 1;
		else mdCmdDefaultVal = 0;
	}
	else if (getType(temp) == STRING) mdCmdDefaultValString = trimChars(temp, "\"");
	else if (getType(temp) == DEC) mdCmdDefaultVal = static_cast<int>(stoul(trimChars(temp, " "), nullptr, 10));
	else mdCmdDefaultVal = static_cast<int>(stoul(trimChars(temp, " $"), nullptr, 16));

	
	if ((mdCmdType == WORD && mdCmdDefaultVal > 0xffff) || (mdCmdType == BYTE && mdCmdDefaultVal > 0xff))
		throw ("Default value out of range in " + commandString);
	
	
	if (cmdStrCopy.find(',') != string::npos) cmdStrCopy.erase(0,temp.find(',')+1);		//not really necessary
	
	
	if (cmdStrCopy.find("FORCE_SUBSTITUTION") != string::npos) {
	
		mdCmdForceSubstitution = true;	//TODO: this one has sub-args
		
		temp = cmdStrCopy;
		temp.erase(0, temp.find("FORCE_SUBSTITUTION")+18);
		if (temp.find_first_of('(') == string::npos) throw ("FORCE_SUBSTITUTION enabled, but no substitution list specified in " + commandString);
		temp.erase(0, temp.find_first_not_of("(\n\t"));
		temp.erase(temp.find_first_of(')'));
		
		mdCmdSubstitutionListLength = count(temp.begin(), temp.end(), ',') + 1;
		
		if ((count(temp.begin(), temp.end(), '"') != mdCmdSubstitutionListLength * 2) || 
			(count(temp.begin(), temp.end(), '=') * 2 != count(temp.begin(), temp.end(), '"')))
			throw ("Invalid substitution list specification in " + commandString);
		
		mdCmdSubstitutionNames = new string[mdCmdSubstitutionListLength];
		mdCmdSubstitutionValues = new int[mdCmdSubstitutionListLength];
		
		for (int i = 0; i < mdCmdSubstitutionListLength; i++) {
		
			mdCmdSubstitutionNames[i] = trimChars(temp.substr(0, temp.find('=')), "\"");	
			string tmp1 = trimChars(temp.substr(temp.find('=') + 1, temp.find_first_of(",)") - (temp.find('=') + 1)), " ");
			
			if (getType(tmp1) == DEC && mdCmdType != BOOL) mdCmdSubstitutionValues[i] = stoi(tmp1, nullptr, 10);
			else if (getType(tmp1) == HEX && mdCmdType != BOOL) mdCmdSubstitutionValues[i] = stoi(trimChars(tmp1, "$"), nullptr, 16);
			else if (getType(tmp1) == BOOL && mdCmdType == BOOL) {
// 				if (tmp1 == "true") mdCmdSubstitutionValues[i] = 1;
// 				else mdCmdSubstitutionValues[i] = 0;
				mdCmdSubstitutionValues[i] = (tmp1 == "true") ? 1 : 0;
			}
			else throw ("Substitution parameter is not a number in " + commandString);
			
			temp.erase(0, temp.find_first_of(",)") + 1);
		}
		
	}
	
	if (cmdStrCopy.find("FORCE_STRING") != string::npos) mdCmdForceString = true;
	if (cmdStrCopy.find("FORCE_REPEAT") != string::npos) mdCmdForceRepeat = true;
	if (cmdStrCopy.find("USE_LAST_SET") != string::npos) mdCmdUseLastSet = true;
	if (cmdStrCopy.find("GLOBAL_CONST") != string::npos) mdCmdGlobalConst = true;
	
	if (mdCmdForceSubstitution && mdCmdForceString) throw ("FORCE_SUBSTITUTION and FORCE_STRING are mutually exclusive in " + commandString);
	if (mdCmdGlobalConst && mdCmdForceString) throw ("CONST and FORCE_STRING are mutually exclusive in " + commandString);

	
	if (verbose) {
		cout << hex << boolalpha << showbase << mdCmdName << ":\t Type is ";
//		if (mdCmdType == BOOL) cout << "BOOL, default is " << mdCmdDefaultValBool;
		if (mdCmdType == BOOL) cout << "BOOL, default is " << static_cast<bool>(mdCmdDefaultVal);
		else if (mdCmdType == BYTE) cout << "BYTE";
		else cout << "WORD";	
//		if (mdCmdDefaultIsString) cout << ", default is \"" << mdCmdDefaultValString << "\"";
		if (mdCmdDefaultValString != "") cout << ", default is \"" << mdCmdDefaultValString << "\"";
		else if (mdCmdType != BOOL) cout << ", default is " << mdCmdDefaultVal;
		
		if (mdCmdForceString) cout << ", FORCE_STRING";
		if (mdCmdForceRepeat) cout << ", FORCE_REPEAT";
		if (mdCmdUseLastSet) cout << ", USE_LAST_SET";
		if (mdCmdGlobalConst) cout << ", CONST";
		if (mdCmdForceSubstitution) {
		
			cout << ", FORCE_SUBSTITUTION: ";
			for (int i = 0; i < mdCmdSubstitutionListLength; i++) {
				cout << mdCmdSubstitutionNames[i];
				if (mdCmdType != BOOL) cout << " = " << mdCmdSubstitutionValues[i];
				else cout << " = " << boolalpha << static_cast<bool>(mdCmdSubstitutionValues[i]);
				if (i < mdCmdSubstitutionListLength - 1) cout << ", ";
			}
		}
		cout << noboolalpha << dec << endl;
	}
}


void mdCommand::resetToDefault() {		//TODO: keep an eye on this to see if it really is useful

	if (!mdCmdGlobalConst) {
	
		mdCmdIsSetNow = false;
		mdCmdCurrentVal = -1;
		mdCmdCurrentValString = "";
		mdCmdLastVal = mdCmdDefaultVal;
		mdCmdLastValString = mdCmdDefaultValString;
		
		if (mdCmdForceRepeat) {
		
			mdCmdIsSetNow = true;
			
			mdCmdCurrentVal = mdCmdLastVal;
			mdCmdCurrentValString = mdCmdLastValString;
			
			//cout << "triggered\n";	//DEBUG
		}	
	}
}


void mdCommand::reset() {

	if (!mdCmdGlobalConst) {
	
		if (mdCmdUseLastSet || mdCmdForceRepeat) {
	
			if (mdCmdCurrentVal != -1 && mdCmdIsSetNow) {
				mdCmdLastVal = mdCmdCurrentVal;
				//cout << hex << mdCmdCurrentVal << " -> " << mdCmdLastVal << endl;	//DEBUG
			}
			if (mdCmdLastValString != "" && mdCmdIsSetNow) {
				mdCmdLastValString = mdCmdCurrentValString;
				//cout << hex << "String " << mdCmdCurrentValString << " -> " << mdCmdLastValString << endl;	//DEBUG
				
			}
		}

		if (mdCmdForceRepeat) {
		
			mdCmdIsSetNow = true;
			
			//cout << "reset to: " << mdCmdCurrentVal << endl;	//DEBUG: ok
			
			if (mdCmdLastVal == -1) mdCmdLastVal = mdCmdDefaultVal;
			if (mdCmdLastValString == "") mdCmdLastValString = mdCmdDefaultValString;
			
			mdCmdCurrentVal = mdCmdLastVal;
			mdCmdCurrentValString = mdCmdLastValString;
			
			
		}
		else {
		
			mdCmdIsSetNow = false;
			mdCmdCurrentVal = -1;
			mdCmdCurrentValString = "";		
		}
	}
}


void mdCommand::set(int &currentVal, string &currentValString) {

	//cout << "cval: " << currentVal << " cvalstr: " << currentValString << endl;


	if (mdCmdGlobalConst) throw (string("Global constant redefined in pattern "));
	
	mdCmdIsSetNow = true;
	
	if (mdCmdForceString) {
		
		if (currentValString == "") throw (string("Integer argument supplied for string command "));
		
		mdCmdCurrentValString = currentValString;
		mdCmdLastValString = currentValString;
	}
	else {
	
		mdCmdCurrentValString = currentValString;
		mdCmdCurrentVal = currentVal;
		
		
		if (mdCmdForceSubstitution && mdCmdCurrentVal == -1) {
		
			//cout << "force substitution, cvalstr = " << currentValString << endl;		//DEBUG ok

			bool foundSubst = false;
			
			for (int i = 0; i < mdCmdSubstitutionListLength && foundSubst == false; i++) {
			
				if (currentValString == mdCmdSubstitutionNames[i]) {
			
					foundSubst = true;
					mdCmdCurrentVal = mdCmdSubstitutionValues[i];
					mdCmdCurrentValString = "";
					
					//cout << "mdCmdCurrentVal: " << mdCmdCurrentVal << endl;	//DEBUG ok
				}
			}
			
			if (!foundSubst) throw ("\"" + currentValString + "\" is not a valid parameter for command ");
		}
		
		mdCmdLastValString = mdCmdCurrentValString;
		mdCmdLastVal = mdCmdCurrentVal;
	}
	//cout << "command set to " << mdCmdCurrentVal << " | " << mdCmdCurrentValString << endl;
}


int mdCommand::getValue() {

	if (mdCmdIsSetNow) return mdCmdCurrentVal;
	if (mdCmdUseLastSet && mdCmdLastVal != -1) return mdCmdLastVal;
	return mdCmdDefaultVal;
}



string mdCommand::getValueString() {

	if (mdCmdIsSetNow) return mdCmdCurrentValString;
	if (mdCmdUseLastSet && mdCmdLastValString != "") return mdCmdLastValString;
	return mdCmdDefaultValString;
}