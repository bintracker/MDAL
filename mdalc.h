#ifndef XMKIT__H__
#define XMKIT__H__

#include <string>
#include <fstream>

#define MDALVERSION 0

using namespace std;


string trimChars(const string& inputString, const char* chars);
int getType(const string& parameter);


enum Type {BOOL, BYTE, WORD, DEC, HEX, STRING, INVALID};
enum ConditionType {REQUIRED, SET_IF};

class mdConfig;


class mdPattern {

public:
	int ptnLength;
	bool firstInSequence;
	bool *requestList;
	bool **lineCommands;
	int **lineCmdVals;
	string **lineCmdStrVals;
	
	mdPattern();
	~mdPattern();
	
	void read(const string *ptnBlock, const int patternNumber, const int blockLength, const mdConfig &config, const bool &verbose);

	friend ostream& operator<<(ostream& os, const mdPattern &ptn);
private:
	string ptnString;
};

class mdModule {

	
public:
	string mdSequenceString;
	mdPattern *modulePatterns;

	mdModule(string &infile, string &outfile, bool &verbose);
	~mdModule();
	
	friend ostream& operator<<(ostream& os, const mdModule &mdf);
	
protected:
	string *mdBlock;

private:
	int linecount;
	string* moduleLines;
	
	int locateToken(string token);
	int getBlockEnd(int blockStart);
	string getArgument(string token);
};


class mdCommand {

public:
	string mdCmdName;
	int mdCmdType;		//BOOL|BYTE|WORD
	
	bool mdCmdIsSetNow;		//in the current pattern row | !force-repeat
	int mdCmdCurrentVal;
	string mdCmdCurrentValString;
	
	
	mdCommand();
	~mdCommand();
	void init(string commandString, bool &verbose);
	void reset();
	void resetToDefault();
	void set(int &currentVal, string &currentValString);
	
	int getValue();
	string getValueString();
	
protected:
	
	
	int mdCmdDefaultVal;
	string mdCmdDefaultValString;
	bool mdCmdForceString;
	bool mdCmdForceSubstitution;
	string* mdCmdSubstitutionNames;
	int* mdCmdSubstitutionValues;
	int mdCmdSubstitutionListLength;
	bool mdCmdForceRepeat;
	bool mdCmdUseLastSet;
	bool mdCmdGlobalConst;
	int mdCmdLastVal;
	string mdCmdLastValString;

private:
	bool wasSet;			//to check against Const
	
};


class mdField {

public:
	bool isWord;		//false: is byte
	bool isRequiredNow;
	
	bool currentIsString;		//false: is int
	int currentValue;
	string currentValueString;

	bool requiredSeqBegin;
	bool requiredPatBegin;
	
	bool requiredAlways;
	bool* requiredBy;	//list of commands that trigger setting this -> this can become a BOOL list
	bool* requiredWhenSet;	//true = required by command set | false = required by command not set
	bool requiredByAny;	//false = required when any of the trigger conditions is fullfilled
				//true = required when all of the trigger conditions are fullfilled

	int setIfCount;
	bool** setIfBy;
	bool** setIfWhenSet;
	bool* setIfByAny;
	int* setIfMask;
	bool* setIfAlways;
	
	int setBitsCount;
	bool* setBitsBy;
	int* setBitsMask;
	
	int setBy;
	int setHiBy;	
	int setLoBy;

	

	mdField();
	~mdField();
	void init(mdCommand *mdCmdList, int &mdCmdCount, string &fieldString, bool &verbose);	//or per reference?
	void getRequests(bool *requestList, const mdConfig &config, const int &row, bool seqBegin);
	string getFieldString(bool *requestList, const mdConfig &config);
	bool checkCondition(const bool *by, const bool *whenSet, bool &byAny, const mdConfig &config);
	bool checkSetifCondition(const bool *by, const bool *whenSet, bool &byAny, const mdConfig &config, bool *requestList);

private:
	int getCmdNr(mdCommand *mdCmdList, int &mdCmdCount, string &cmdString);

};


class mdConfig {

public:
	//global config parameters
	bool useSequence;
	bool usePatterns;
	bool useTables;
	bool useSamples;
	string wordDirective;
	string byteDirective;
	string hexPrefix;
	
	//sequence config parameters
	bool useSeqEnd;
	bool useSeqLoop;
	bool useSeqLoopPointer;
	string seqEndString;
	string seqLoopLabel;
	
	//pattern config parameters
	bool usePtnEnd;
	bool initPtnDefaults;
	string ptnEndString;
	
	//MDAL commands config parameters
	mdCommand* mdCmdList;
	int mdCmdCount;
	
	//MDAL fields config parameters
	mdField* mdFieldList;
	int mdFieldCount;

	mdConfig(string &configname, bool &verbose);
	~mdConfig();

private:
	int linecount;
	string* cfgLines;
	
	int locateToken(string token, int blockStart, int blockEnd);
	string getArgumentString(string token, int blockStart, int blockEnd);
	int getArgumentCount(string argString);
	string getArgument(string argString, int argNumber);
	int getBlockEnd(int blockStart);
	int countBlockLines(int &blockStart, int &blockEnd);
	int countFieldBlockLines(int &blockStart, int &blockEnd);
};



class mdSequence {

public:
	int mdSequenceLength;
	int uniquePtnCount;
	string sequenceString;
	string *uniquePtnList;
	int mdSequenceLoopPosition;
	string *mdSequenceArray;
	mdConfig *config;
	
	
	mdSequence(string* sequenceBlock, int sequenceBlockLength, mdConfig &config, bool &verbose);
	~mdSequence();
	
	
	friend ostream& operator<<(ostream& os, const mdSequence &seq);
	
private:
	string getSequenceString(const mdConfig &config);
};



class mdTable {

public:
	string tabEndLabel;
	
	mdTable();
	~mdTable();
	bool mdGetPattern();
private:

};


#endif