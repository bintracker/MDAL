#include <string>

#include "mdalc.h"


mdBlockList::mdBlockList(string &blockIdentifier) {

	blockID = blockIdentifier;
}


mdBlockList::~mdBlockList() {

}


void mdBlockList::addReference(string &title) {

	bool isUsed = false;

	for (auto&& it : uniqueReferences) {
	
		if (title == it) isUsed = true;
	}

	if (!isUsed) uniqueReferences.push_back(title);
}