#include <iostream>

#include "mdal.h"

using namespace std;


mdSequence::mdSequence(): mdSequenceLength(0), sequenceString(""), mdSequenceLoopPosition(0) {

	mdSequenceArray = nullptr;
}

mdSequence::~mdSequence() {

	delete[] mdSequenceArray;
}


void mdSequence::read(string *sequenceBlock, const unsigned &sequenceBlockLength, const mdConfig &config) {

	
	for (unsigned i = 0; i < sequenceBlockLength; i++) {
		
		string inputString = sequenceBlock[static_cast<int>(i)];
		inputString.erase(0, inputString.find_first_not_of(" \t"));
		inputString.erase(inputString.find_last_not_of(" \t")+1); 
		
		if (inputString != "" && inputString != "[LOOP]") mdSequenceLength++;
	}
	
	
	if (!mdSequenceLength) throw (string("Sequence contains no patterns"));
	if (config.seqMaxLength && mdSequenceLength > config.seqMaxLength) throw (string("Maximum sequence length exceeded."));
	
	mdSequenceArray = new string[mdSequenceLength];
	int element = 0;
	
	for (unsigned i = 0; i < sequenceBlockLength; i++) {
	
		string inputString = sequenceBlock[static_cast<int>(i)];
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
		
	sequenceString = getSequenceString(config);
	
	return;
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
