#include <iostream>
#include <string>
#include <fstream>

#include "mdalc.h"

using namespace std;


mdModule::mdModule(vector<string> &moduleLines, bool &verbose) {

	MUSICASM.str(string());
	string tempstr;
	rawDataBlock = nullptr;
	linecount = moduleLines.size();

	string configname = getArgument(string("CONFIG"), moduleLines);		//TODO: occurs only once, make inline
	
	mdConfig config(configname, verbose);
	
	
	
	try {
	
		for (auto&& it : config.blockTypes) moduleBlocks.emplace_back(it.blockConfigID);		
		moduleBlocks.shrink_to_fit();
		
		
	
		if (verbose) cout << endl << "MODULE DATA\n===========" << endl;
	
		int blockStart = locateToken(string(":SEQUENCE"), moduleLines);
		int blockEnd = getBlockEnd(blockStart, moduleLines);
	
		if (blockStart > blockEnd) throw (string("Sequence contains no patterns."));
	
		rawDataBlock = new string[blockEnd - blockStart];
	
		for (int i = blockStart + 1; i <= blockEnd; i++) rawDataBlock[i - blockStart - 1] = moduleLines[i];

		mdSequence seq(rawDataBlock, blockEnd - blockStart, config);
	
		delete[] rawDataBlock;
		rawDataBlock = nullptr;
		
		int seqStart = true;		//TODO: temporary solution which requires Pattern block type to be configured first, needs to be replaced
		
		for (int i = 0; i < seq.mdSequenceLength; i++) {
			
			moduleBlocks.at(0).addReference(seq.mdSequenceArray[i], seqStart);
			seqStart = false;
		}
		
	
		if (verbose) cout << seq << endl;
		MUSICASM << seq << endl;

	
		//add default references
		for (int i = 0; i < config.mdCmdCount; i++) {
		
			if (config.mdCmdList[i].isBlkReference) {
			
				for (auto&& it: moduleBlocks) {
				
					if (it.blockTypeID == config.mdCmdList[i].referenceBlkID) it.addReference(config.mdCmdList[i].mdCmdDefaultValString, false);
					//TODO: temporary solution for flagging sequence start, see above
				}
			}	
		}
		
		
		int blockType = 0;
		
		for (auto&& it : moduleBlocks) {
			
			int blockNr = 0;
		
			while (it.referenceCount) {
			
				blockStart = locateToken(":" + it.blocks.at(blockNr).blkName, moduleLines);
				blockEnd = getBlockEnd(blockStart, moduleLines);

		
				if (blockStart >= linecount - 1) throw ("Block \"" + it.blocks.at(blockNr).blkName + "\" is not defined.");
				if (blockStart >= blockEnd) throw ("Block \"" + it.blocks.at(blockNr).blkName + "\" contains no data");
				//TODO: does not reliably detect empty patterns.
		
				rawDataBlock = new string[blockEnd - blockStart];
		
				for (int j = blockStart + 1; j <= blockEnd; j++) rawDataBlock[j - blockStart - 1] = moduleLines[j];
		
				try {	
					it.blocks.at(blockNr).read(rawDataBlock, blockEnd - blockStart, config, config.blockTypes.at(blockType), moduleBlocks, verbose);
				}
				catch(string &e) {
					throw ("In pattern \"" + it.blocks.at(blockNr).blkName + "\": " + e);
				}
		
				delete[] rawDataBlock;
				rawDataBlock = nullptr;

		
				MUSICASM << it.blocks.at(blockNr) << endl;
				if (verbose) cout << it.blocks.at(blockNr) << endl;
				
				blockNr++;
				it.referenceCount--;
			}
			
			blockType++;
		}
		
		return;		
	}
	catch(string &e) {
		throw (e + "\nModule validation failed.");
	}
}


mdModule::~mdModule() {
	
	delete[] rawDataBlock;
//	delete[] moduleLines;
}


int mdModule::getBlockEnd(int blockStart, vector<string> &moduleLines) {
	
	int line;
	size_t pos = string::npos;
	
	for (line = blockStart + 1; line < linecount && pos == string::npos; line++) pos = moduleLines[line].find(":");
	
	if (line == linecount) return line - 1;
	return line - 2;
}

//TODO: throw error if token not found
int mdModule::locateToken(string token, vector<string> &moduleLines) {

	int line;
//	size_t pos = string::npos;
	string tempstr = "";
	string tempstr2;
		
	for (line = 0; line < linecount && tempstr2 != token; line++) {		//used to be line <= linecount

		tempstr2 = moduleLines[line];
		if (tempstr2.find('/') != string::npos) tempstr2.erase(tempstr2.find_first_of('/'));
		tempstr2 = trimChars(tempstr2, " \t");
	}

	if (tempstr2 == token) line--;

	return line;
}

string mdModule::getArgument(string token, vector<string> &moduleLines) {

	string tempstr = "";
	int line;
	
	for (line = 0; line < linecount && tempstr == ""; line++) {
	
		size_t pos = moduleLines[line].find(token.data());
		if (pos != string::npos) tempstr = trimChars(moduleLines[line].substr(pos + token.size()), " =");
	}
	
	if (line == linecount) throw ("No " + token + " statement found.");
	
	return tempstr;
}

ostream& operator<<(ostream &os, const mdModule &mdf) {

	os << mdf.MUSICASM.str() << endl;	
	return os;
}
