#include <iostream>

#include "mdalc.h"

using namespace std;

mdSequence::mdSequence(string *sequenceBlock, int sequenceBlockLength, mdConfig &config) {

	mdSequenceArray = nullptr;
	uniquePtnList = nullptr;
	mdSequenceLength = 0;
	uniquePtnCount = 0;
	mdSequenceLoopPosition = 0;
	sequenceString = "";

	
	for (int i = 0; i < sequenceBlockLength; i++) {
		
		string inputString = sequenceBlock[i];
		inputString.erase(0, inputString.find_first_not_of(" \t"));
		inputString.erase(inputString.find_last_not_of(" \t")+1); 
		
		if (inputString != "" && inputString != "[LOOP]") mdSequenceLength++;
	}
	
	
	if (!mdSequenceLength) throw (string("Sequence contains no patterns"));
	if (config.seqMaxLength && mdSequenceLength > config.seqMaxLength) throw (string("Maximum sequence length exceeded."));
	
	mdSequenceArray = new string[mdSequenceLength];
	int element = 0;
	
	for (int i = 0; i < sequenceBlockLength; i++) {
	
		string inputString = sequenceBlock[i];
		inputString.erase(0, inputString.find_first_not_of(" \t"));
		inputString.erase(inputString.find_last_not_of(" \t")+1); 
		
		if (inputString != "" && inputString != "[LOOP]") {
			
			mdSequenceArray[element] = inputString;
			element++;
		}
		
		if (inputString == "[LOOP]") mdSequenceLoopPosition = element;
	}
	
	if (mdSequenceLoopPosition >= mdSequenceLength) {
	
		cout << "Warning: [LOOP] position is invalid, falling back to default." << endl;
		mdSequenceLoopPosition = 0;
	}
	
	
	for (int i = 0; i < mdSequenceLength; i++) {
	
		bool unique = true;
		
		for (int j = 0; j < i; j++) {
		
			if (mdSequenceArray[j] == mdSequenceArray[i]) unique = false;
		}
		
		if (unique) uniquePtnCount++;
	}
	
	uniquePtnList = new string[uniquePtnCount];
	int iUnique = 0;
	
	for (int spos = 0; spos < mdSequenceLength; spos++) {
	
		bool unique = true;
	
		for (int i = 0; i < uniquePtnCount; i++) {
			
			if (uniquePtnList[i] == mdSequenceArray[spos]) unique = false;
		}
		
		if (unique) {
			
			uniquePtnList[iUnique] = mdSequenceArray[spos];
			iUnique++;
		}
	}
	
	
	sequenceString = getSequenceString(config);
	
	return;
}

mdSequence::~mdSequence() {

	delete[] mdSequenceArray;
	delete[] uniquePtnList;
}

string mdSequence::getSequenceString(const mdConfig &config) {

	string seqString = config.seqLabel + "\n";
	
	for (int i = 0; i < mdSequenceLength; i++) {
	
		if (i == mdSequenceLoopPosition && config.useSeqLoop) seqString = seqString + "\n" + config.seqLoopLabel;
	
		seqString = seqString + "\n\t" + config.wordDirective + " " + config.blockTypes.at(0).blkLabelPrefix + mdSequenceArray[i];
		//TODO: temporary solution, permanent solution must auto-detect correct block type
	}
	
	seqString = seqString + "\n\t" + config.seqEndString;
	if (config.useSeqLoopPointer) seqString = seqString + "\n\t" + config.wordDirective + " " + config.seqLoopLabel;
	
	return seqString;
}


ostream& operator<<(ostream& os, const mdSequence &seq) {
	
	os << seq.sequenceString << endl << endl;
	return os;
}