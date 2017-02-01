#ifndef XMKIT__H__
#define XMKIT__H__

#include <string>
#include <fstream>
#include <vector>

#define MDALVERSION 0

using namespace std;


string trimChars(const string& inputString, const char* chars);
int getType(const string& parameter);


enum Type {BOOL, BYTE, WORD, DEC, HEX, STRING, INVALID};
enum BlockType {GENERIC, PATTERN, TABLE};
enum ConditionType {REQUIRED, SET_IF};
enum ClearFlags {CLEAR_HI = 1, CLEAR_LO, CLEAR_ALL};

class mdConfig;
class mdBlock;


class mdBlockList {

public: 
	string blockTypeID;
	int referenceCount;
	vector<string> uniqueReferences;
	vector<mdBlock> blocks;
	
	mdBlockList(string &blockTypeIdentifier);
	~mdBlockList();
	
	void addReference(string &title, bool seqStart);
};


class mdModule {

	
public:
	string mdSequenceString;
	vector<mdBlockList> moduleBlocks;

	mdModule(string &infile, string &outfile, bool &verbose);
	~mdModule();
	
	friend ostream& operator<<(ostream& os, const mdModule &mdf);
	
protected:
	string *rawDataBlock;

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
	bool mdCmdAuto;
	
	bool mdCmdIsSetNow;		//in the current pattern row | !force-repeat
	int mdCmdCurrentVal;
	string mdCmdCurrentValString;
	
	bool limitRange;
	int lowerRangeLimit;
	int upperRangeLimit;
	
	string mdCmdDefaultValString;
	
	bool isBlkReference;
	string referenceBlkID;
	
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
	
	int mdCmdAutoVal;
	string mdCmdAutoValString;
	
	bool mdCmdForceString;
	bool mdCmdForceInt;
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
	bool requiredBlkBegin;
	
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
	int* setIfClear;
	bool* setIfAlways;
	
	int setBitsCount;
	bool* setBitsBy;
	int* setBitsMask;
	int* setBitsClear;
	
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



class mdBlockConfig {
	
public:	
	string blockConfigID;
	int baseType;
	bool useBlkEnd;
	string blkEndString;
	bool initBlkDefaults;
	string blkLabelPrefix;
	mdField* blkFieldList;
	int blkFieldCount;
	int blkMaxLength;
	
	mdBlockConfig(string id);
	~mdBlockConfig();
};


class mdConfig {

public:
	//global config parameters
	bool usePatterns;
	bool useTables;
	bool useSamples;
	string wordDirective;
	string byteDirective;
	string hexPrefix;
	
	//MDAL commands config parameters
	mdCommand* mdCmdList;
	bool* cmdIsTablePointer;
	int mdCmdCount;
	
	//sequence config parameters
	bool useSeqEnd;
	bool useSeqLoop;
	bool useSeqLoopPointer;
	string seqEndString;
	string seqLoopLabel;
	string seqLabel;
	int seqMaxLength;
	
	
	int blockTypeCount;
	vector<mdBlockConfig> blockTypes;


	mdConfig(string &configname, bool &verbose);
	~mdConfig();

	string* cfgLines;
private:
	int linecount;
	
	
	int locateToken(string token, int blockStart, int blockEnd);
	string getArgumentString(string token, int blockStart, int blockEnd);
	int getArgumentCount(string argString);
	string getArgument(string argString, int argNumber);
	int getBlockEnd(int blockStart);
	int countBlockLines(int &blockStart, int &blockEnd);
	int countFields(int &blockStart, int &blockEnd);
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
	
	
	mdSequence(string* sequenceBlock, int sequenceBlockLength, mdConfig &config);
	~mdSequence();
	
	
	friend ostream& operator<<(ostream& os, const mdSequence &seq);
	
private:
	string getSequenceString(const mdConfig &config);
};



class mdBlock {

public:
	string blkName;
	int blkLength;
	bool *requestList;
	bool **lineCommands;
	int **lineCmdVals;
	string **lineCmdStrVals;
	
	bool firstInSequence;
	
	mdBlock(string name, bool seqStart);
	~mdBlock();
	
	void read(const string *rawData, const int blockLength, const mdConfig &config, const mdBlockConfig &blkConfig, vector<mdBlockList> &moduleBlocks, const bool &verbose);

	friend ostream& operator<<(ostream& os, const mdBlock &blk);

private:
	string blkString;
};


#endif