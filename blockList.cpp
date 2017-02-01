
#include "mdalc.h"


mdBlockList::mdBlockList(string &blockTypeIdentifier) {

	blockTypeID = blockTypeIdentifier;
	referenceCount = 0;
}


mdBlockList::~mdBlockList() {

}


void mdBlockList::addReference(string &title, bool seqStart) {

	bool isUsed = false;

	for (auto&& it : uniqueReferences) {
	
		if (title == it) isUsed = true;
	}

	if (!isUsed) {
		
		uniqueReferences.push_back(title);		//TODO: redundant, can probably remove uniqueReferences altogether
		blocks.emplace_back(title, seqStart);
		referenceCount++;	
	}
}