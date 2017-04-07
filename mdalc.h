#ifndef XMKIT__H__
#define XMKIT__H__

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>

#define MDALVERSION 0

using namespace std;


string trimChars(const string& inputString, const char* chars);
int getType(const string& parameter);
string getArgument(string token, const vector<string> &moduleLines);


enum Type {BOOL, BYTE, WORD, DEC, HEX, STRING, INVALID};
enum BlockType {GENERIC, PATTERN, TABLE};
enum ConditionType {REQUIRED, SET_IF};
enum ClearFlags {CLEAR_HI = 1, CLEAR_LO, CLEAR_ALL};

class mdConfig;
class mdBlock;
class mdBlockConfig;
class mdCommand;


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
	
	bool* useCmd;

	

	mdField();
	~mdField();
	void init(mdCommand *mdCmdList, const int &mdCmdCount, const string &fieldString);
	void getRequests(bool *requestList, const mdConfig &config, const int &row, bool seqBegin);
	string getFieldString(bool *requestList, const mdConfig &config);
	bool checkCondition(const bool *by, const bool *whenSet, bool &byAny, const mdConfig &config);
	bool checkSetifCondition(const bool *by, const bool *whenSet, bool &byAny, const mdConfig &config, bool *requestList);

private:
	int getCmdNr(mdCommand *mdCmdList, const int &mdCmdCount, const string &cmdString);

};

class mdConfig {

public:
	//global config parameters
	bool useSamples;
	string wordDirective;
	string byteDirective;
	string hexPrefix;
	string targetPlatform;
	
	//MDAL commands config parameters
	mdCommand* mdCmdList;
	int mdCmdCount;
	
	//sequence config parameters
	bool useSeqEnd;
	bool useSeqLoop;
	bool useSeqLoopPointer;
	string seqEndString;
	string seqLoopLabel;
	string seqLabel;
	int seqMaxLength;
	
	
	size_t blockTypeCount;
	vector<mdBlockConfig> blockTypes;


	mdConfig();
	~mdConfig();
	mdConfig(const mdConfig &config) = delete;
	void init(const string &configfile, bool &verbose);
	void reset();

	string* cfgLines;
private:
	int linecount;
	
	int locateToken(const string &token, const int &blockStart, const int &blockEnd);
	string getArgumentString(string token, const int &blockStart, const int &blockEnd);
	string getArgument(const string &argString, int argNumber);
	int getBlockEnd(const int &blockStart);
	int countBlockLines(const int &blockStart, const int &blockEnd);
	int countFields(const int &blockStart, const int &blockEnd);
};


class mdSequence {

public:
	int mdSequenceLength;
	int uniquePtnCount;
	string sequenceString;
	int mdSequenceLoopPosition;
	string *uniquePtnList;
	string *mdSequenceArray;
	
	mdSequence();
	~mdSequence();
	void init(string* sequenceBlock, const unsigned &sequenceBlockLength, const mdConfig &config);
	
	
	friend ostream& operator<<(ostream& os, const mdSequence &seq);
	
private:
	string getSequenceString(const mdConfig &config);
};


class mdBlockList {

public: 
	string blockTypeID;
	int referenceCount;
	set<string> uniqueReferences;
	vector<mdBlock> blocks;
	
	mdBlockList(const string &blockTypeIdentifier);
	mdBlockList(const mdBlockList &lst);
	~mdBlockList();
	
	void addReference(const string &title, bool seqStart);
};


class mdModule {

	
public:
	string mdSequenceString;
	vector<mdBlockList> moduleBlocks;
	
	mdSequence seq;
	
	ostringstream MUSICASM;

	mdModule(const vector<string> &moduleLines, const mdConfig &config, bool &verbose);
	~mdModule();
	
	friend ostream& operator<<(ostream& os, const mdModule &mdf);
	
protected:
	string *rawDataBlock;

private:
	size_t linecount;
	
	unsigned locateToken(const string &token, const vector<string> &moduleLines);
	unsigned getBlockEnd(const unsigned &blockStart, const vector<string> &moduleLines);
};


class mdCommand {

public:
	string mdCmdName;
	int mdCmdType;		//BOOL|BYTE|WORD
	bool mdCmdAuto;
	
	bool useNoteNames;
	bool allowModifiers;
	
	bool mdCmdIsSetNow;		//in the current pattern row | !force-repeat
	int mdCmdCurrentVal;
	string mdCmdCurrentValString;
	
	bool limitRange;
	int lowerRangeLimit;
	int upperRangeLimit;
	
	string mdCmdDefaultValString;
	
	bool isBlkReference;
	string referenceBlkID;
	
	mdCommand *defaultSubstitute;
	
	int mdCmdDefaultVal;
	
	int mdCmdAutoVal;
	string mdCmdAutoValString;
	
	bool mdCmdForceString;
	bool mdCmdForceInt;
	bool mdCmdForceSubstitution;
	
	int mdCmdSubstitutionListLength;
	bool mdCmdForceRepeat;
	bool mdCmdUseLastSet;
	bool mdCmdGlobalConst;
	
	int mdCmdLastVal;
	string mdCmdLastValString;
	
	string* mdCmdSubstitutionNames;
	int* mdCmdSubstitutionValues;


	mdCommand();
	~mdCommand();
	mdCommand(const mdCommand &cmd) = delete;
	void init(const string &commandString, bool &verbose);
	void reset();
	void resetToDefault();
	void set(const int &currentVal, const string &currentValString);
	void setDefault(const string &param);
	
	int getValue();
	string getValueString();
	
private:
	int getDefaultVal();
	string getDefaultValString();	
};


class mdBlockConfig {
	
public:	
	string blockConfigID;
	int baseType;
	bool useBlkEnd;
	string blkEndString;
	bool initBlkDefaults;
	string blkLabelPrefix;
	int blkFieldCount;
	int blkMaxLength;
	mdField* blkFieldList;
	
	mdBlockConfig(const string &id);
	mdBlockConfig(const mdBlockConfig &blkCfg);
	~mdBlockConfig();
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
	
	mdBlock(const string &name, bool seqStart);
	mdBlock(const mdBlock &blk);
	~mdBlock();
	
	void read(const string *rawData, const int blockLength, const mdConfig &config, const mdBlockConfig &blkConfig, vector<mdBlockList> &moduleBlocks);

	friend ostream& operator<<(ostream& os, const mdBlock &blk);

private:
	string blkString;
};


#endif
