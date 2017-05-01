#ifndef XMKIT__H__
#define XMKIT__H__

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>

#define MDALVERSION 0


std::string trimChars(const std::string& inputString, const char* chars);
int getType(const std::string& parameter);
std::string getArgument(std::string token, const std::vector<std::string> &moduleLines);


enum Type {BOOL, BYTE, WORD, DEC, HEX, STRING, INVALID};
enum BlockType {GENERIC, PATTERN, TABLE};
enum ConditionType {REQUIRED, SET_IF};
enum ClearFlags {CLEAR_HI = 1, CLEAR_LO, CLEAR_ALL};
enum DefaultCommandCount {DEFAULT_COMMAND_COUNT = 2};

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
	std::string currentValueString;

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
	void init(mdCommand *mdCmdList, const int &mdCmdCount, const std::string &fieldString);
	void getRequests(bool *requestList, const mdConfig &config, const int &row, bool seqBegin);
	std::string getFieldString(bool *requestList, const mdConfig &config);
	bool checkCondition(const bool *by, const bool *whenSet, bool &byAny, const mdConfig &config);
	bool checkSetifCondition(const bool *by, const bool *whenSet, bool &byAny, const mdConfig &config, bool *requestList);

private:
	int getCmdNr(mdCommand *mdCmdList, const int &mdCmdCount, const std::string &cmdString);

};

class mdConfig {

public:
	//global config parameters
	bool useSamples;
	std::string wordDirective;
	std::string byteDirective;
	std::string hexPrefix;
	std::string targetPlatform;
	
	//MDAL commands config parameters
	mdCommand* mdCmdList;
	int mdCmdCount;
	
	//sequence config parameters
	bool useSeqEnd;
	bool useSeqLoop;
	bool useSeqLoopPointer;
	std::string seqEndString;
	std::string seqLoopLabel;
	std::string seqLabel;
	int seqMaxLength;
	
	
	size_t blockTypeCount;
	std::vector<mdBlockConfig> blockTypes;


	mdConfig();
	~mdConfig();
	mdConfig(const mdConfig &config) = delete;
	void init(const std::string &configfile, bool &verbose);
	void reset();
	
	static bool isCompatible(const mdCommand &cmd1, const mdCommand &cmd2);

	std::string* cfgLines;
private:
	int linecount;
	
	int locateToken(const std::string &token, const int &blockStart, const int &blockEnd);
	std::string getArgumentString(std::string token, const int &blockStart, const int &blockEnd);
	std::string getArgument(const std::string &argString, int argNumber);
	int getBlockEnd(const int &blockStart);
	int countBlockLines(const int &blockStart, const int &blockEnd);
	int countFields(const int &blockStart, const int &blockEnd);
};


class mdSequence {

public:
	int mdSequenceLength;
	int uniquePtnCount;
	std::string sequenceString;
	int mdSequenceLoopPosition;
	std::string *uniquePtnList;
	std::string *mdSequenceArray;
	
	mdSequence();
	~mdSequence();
	void init(std::string* sequenceBlock, const unsigned &sequenceBlockLength, const mdConfig &config);
	
	
	friend std::ostream& operator<<(std::ostream& os, const mdSequence &seq);
	
private:
	std::string getSequenceString(const mdConfig &config);
};


class mdBlockList {

public: 
	std::string blockTypeID;
	int referenceCount;
	std::set<std::string> uniqueReferences;
	std::vector<mdBlock> blocks;
	
	mdBlockList(const std::string &blockTypeIdentifier);
	mdBlockList(const mdBlockList &lst);
	~mdBlockList();
	
	void addReference(const std::string &title, bool seqStart);
};


class mdModule {
	
public:
	std::string mdSequenceString;
	std::vector<mdBlockList> moduleBlocks;
	
	mdSequence seq;
	
	std::ostringstream MUSICASM;

	mdModule(const std::vector<std::string> &moduleLines, const mdConfig &config, bool &verbose);
	~mdModule();
	
	friend std::ostream& operator<<(std::ostream& os, const mdModule &mdf);
	
protected:
	std::string *rawDataBlock;

private:
	size_t linecount;
	
	unsigned locateToken(const std::string &token, const std::vector<std::string> &moduleLines);
	unsigned getBlockEnd(const unsigned &blockStart, const std::vector<std::string> &moduleLines);
};


class mdCommand {

public:
	std::string mdCmdName;
	int mdCmdType;		//BOOL|BYTE|WORD
	bool mdCmdAuto;
	
	bool useNoteNames;
	bool allowModifiers;
	
	bool mdCmdIsSetNow;		//in the current pattern row | !force-repeat
	int mdCmdCurrentVal;
	std::string mdCmdCurrentValString;
	
	bool limitRange;
	int lowerRangeLimit;
	int upperRangeLimit;
	
	std::string mdCmdDefaultValString;
	
	bool isBlkReference;
	std::string referenceBlkID;
	
	mdCommand *defaultSubstitute;
	
	int mdCmdDefaultVal;
	
	int mdCmdAutoVal;
	std::string mdCmdAutoValString;
	
	bool mdCmdForceString;
	bool mdCmdForceInt;
	bool mdCmdForceSubstitution;
	
	int mdCmdSubstitutionListLength;
	bool mdCmdForceRepeat;
	bool mdCmdUseLastSet;
	bool mdCmdGlobalConst;
	
	int mdCmdLastVal;
	std::string mdCmdLastValString;
	
	std::string* mdCmdSubstitutionNames;
	int* mdCmdSubstitutionValues;


	mdCommand();
	~mdCommand();
	mdCommand(const mdCommand &cmd) = delete;
	void init(const std::string &commandString, bool &verbose);
	void reset();
	void resetToDefault();
	void set(const int &currentVal, const std::string &currentValString);
	void setDefault(const std::string &param);
	
	int getValue();
	std::string getValueString();
	
private:
	int getDefaultVal();
	std::string getDefaultValString();	
};


class mdBlockConfig {
	
public:	
	std::string blockConfigID;
	int baseType;
	bool useBlkEnd;
	std::string blkEndString;
	bool initBlkDefaults;
	std::string blkLabelPrefix;
	int blkFieldCount;
	int blkMaxLength;
	mdField* blkFieldList;
	
	mdBlockConfig(const std::string &id);
	mdBlockConfig(const mdBlockConfig &blkCfg);
	~mdBlockConfig();
};


class mdBlock {

public:
	std::string blkName;
	int blkLength;
	bool *requestList;
	bool **lineCommands;
	int **lineCmdVals;
	std::string **lineCmdStrVals;
	
	bool firstInSequence;
	
	mdBlock(const std::string &name, bool seqStart);
	mdBlock(const mdBlock &blk);
	~mdBlock();
	
	void read(const std::string *rawData, const int blockLength, const mdConfig &config, const mdBlockConfig &blkConfig, 
		std::vector<mdBlockList> &moduleBlocks);

	friend std::ostream& operator<<(std::ostream& os, const mdBlock &blk);

private:
	std::string blkString;
};


#endif
