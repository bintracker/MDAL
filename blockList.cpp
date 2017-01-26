#include <string>

#include "mdalc.h"


mdBlockList::mdBlockList(string &blockIdentifier) {

	blockID = blockIdentifier;
	uniqueReferences = new vector<string>;
}


mdBlockList::~mdBlockList() {

	delete uniqueReferences;

}

void mdBlockList::addReference(string &title) {

	bool isUsed = false;

	for (auto it : *uniqueReferences) {
	
		if (title == it) isUsed = true;
	}

	if (!isUsed) uniqueReferences->push_back(title);
}