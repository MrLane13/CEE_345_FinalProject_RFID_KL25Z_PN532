/*********************************************
* Author: Lane Mills
*	Date  : 04/22/16
* 
*
*
*
*/

#include "cardcheck.h"
#include "blinkLED.h"
#include "delay.h"

// This is a hard-coded value for the purposes of demonstration
// Your master card may have a different UID value
uint8_t mCardMaster[7] = {0xcd, 0x93, 0xc8, 0x23, 0, 0, 0,};
uint8_t mCardUser[5][7] = 	{ 
{ 0, 0, 0, 0, 0, 0, 0 },
{ 0, 0, 0, 0, 0, 0, 0 },
{ 0, 0, 0, 0, 0, 0, 0 },
{ 0, 0, 0, 0, 0, 0, 0 },
{ 0, 0, 0, 0, 0, 0, 0 }};

int numberOfSavedUsers = 0;

blinkLED blink;
delay delay;

// Constructor
//cardcheck::cardcheck(uint8_t masterUID[], uint8_t mstrUIDLength, uint8_t cardUsers[][7]) {
//	
//	// Initialize variables
//	for(int i=0; i<mstrUIDLength; i++) {
//		mCardMaster[i] = masterUID[i];
//	}
//	for(int i=0; i<5; i++) {
//		for(int j=0; j<7; j++) {
//			mCardUser[i][j] = cardUsers[i][j];
//		}
//	}
//	
//}

// Empty constructor
cardcheck::cardcheck() {
	
}

// Public method that returns an int for the type of card found
int cardcheck::cardType(uint8_t uid[], uint8_t uidLength) {
	// check if it's classic
	if(uidLength == 4) 
		return verify(uid, uidLength);
	// check if it's light
	else if(uidLength ==7)
		return verify(uid, uidLength);
	// return false
	else
		return 0;
	
}

// Public method that adds a uid to our user array
void cardcheck::cardOpAdd(uint8_t uid[], uint8_t uidLength) {
	
	if(hasFreeSpace()) {
		for(int i=0; i<3; i++) {
			if(i==2) {
				int index = getFreeIndex();
				addUser(uid,index);
				blink.blink(1,0,1,500);
			}
			blink.blink(1,1,0,300);
			delay.delayTime(500);
		}
	}
	else {
		// Not added
		blink.doubleBlink(0,1,1,500);
	}
}

// Public method to remove a user from the mCardUser
void cardcheck::cardOpRemove(uint8_t uid[], uint8_t uidLength) {

	if(isUser(uid,uidLength)) {
		for(int i=0; i<3; i++) {
			if(i==2) {
				int temp = getIndex(uid,uidLength);
				removeUser(temp);
				//clean();
				// Blink red
				blink.blink(0,1,1,500);
			}
			else {
				blink.blink(1,0,1,300);
				delay.delayTime(500);
			}
		}
	}
}


// Verifies which type of card we have, returns an int for that type
int cardcheck::verify(uint8_t uid[], uint8_t uidLength) {
	if(isMaster(uid, uidLength))
			return 3;
		else if(isUser(uid, uidLength))
			return 2;
		else
			return 0;
}

// See if the uid we were given matches the master uid
bool cardcheck::isMaster(uint8_t uid[], uint8_t uidLength) {
	if( uid[0]==mCardMaster[0] & uid[1]==mCardMaster[1] &
		uid[2]==mCardMaster[2] & uid[3]==mCardMaster[3] &
		uid[4]==mCardMaster[4] & uid[5]==mCardMaster[5] &
		uid[6]==mCardMaster[6] ) {
		return true;
	}
	else 
		return false;
}

// See if the uid we were given matches any of our user uids
bool cardcheck::isUser(uint8_t uid[], uint8_t uidLength) {
	
	// check each of the five user slots we have
	for(int i=0; i<5; i++) {
		// return true on the first one that matches
		if( uid[0]==mCardUser[i][0] & uid[1]==mCardUser[i][1] &
			uid[2]==mCardUser[i][2] & uid[3]==mCardUser[i][3] &
			uid[4]==mCardUser[i][4] & uid[5]==mCardUser[i][5] &
			uid[6]==mCardUser[i][6] ) {
			return true;
			}
	}
	// otherwise return false
	return false;
}

// Private function:: removes user from our mCardUser array
void cardcheck::removeUser(int index) {
	for(int i=0; i<7; i++) {
		// Replace all entries of the array at 'index' with zeros
		mCardUser[index][i] = 0x00;
	}
}

void cardcheck::addUser(uint8_t uid[], int index) {
	for(int i=0; i<7; i++) {
		mCardUser[index][i] = uid[i];
	}
}


// Used to reorder
void cardcheck::clean() {
	for(int i=0; i<4; i++) {
		if(isEmpty(i) & !isEmpty(i+1)) {
			for(int j=0; j<7; j++) {
				mCardUser[i][j] = mCardUser[i+1][j];
			}
			removeUser(i+1);
		}
	}
}

// Check to see if a particular index in our mCardUser array is empty
bool cardcheck::isEmpty(int index) {
	if(mCardUser[index][0] == 0x00 & mCardUser[index][1] == 0x00 &
		mCardUser[index][2] == 0x00 & mCardUser[index][3] == 0x00 &
		mCardUser[index][4] == 0x00 & mCardUser[index][5] == 0x00 &
		mCardUser[index][6] == 0x00) {
			return true;
		}
		else {
			return false;
		}
}

// Get the index of a particular user in our mCardUser array
int cardcheck::getIndex(uint8_t uid[], uint8_t uidLength) {
	for(int i=0; i<5; i++) {
		if(uid[0]==mCardUser[i][0] & uid[1]==mCardUser[i][1] &
			uid[2]==mCardUser[i][2] & uid[3]==mCardUser[i][3] &
			uid[4]==mCardUser[i][4] & uid[5]==mCardUser[i][5] &
			uid[6]==mCardUser[i][6] ) {
				return i;
		}
	}
	return -1;
}

// See if we have any free spaces in our user array
bool cardcheck::hasFreeSpace() {
	for(int i=0; i<5; i++) {
		if(mCardUser[i][0] == 0x00 & mCardUser[i][1] == 0x00 &
			mCardUser[i][2] == 0x00 & mCardUser[i][3] == 0x00 &
			mCardUser[i][4] == 0x00 & mCardUser[i][5] == 0x00 &
			mCardUser[i][6] == 0x00 ) {
				return true;
			}
	}
	return false;
}

// PREREQUISITE:: This method MUST ONLY be called after hasFreeSpace()
// because the user may assume they will be returned a VALID and EMPTY array index
int cardcheck::getFreeIndex() {
	for(int i=0; i<5; i++) {
		if(mCardUser[i][0] == 0x00 & mCardUser[i][1] == 0x00 &
			mCardUser[i][2] == 0x00 & mCardUser[i][3] == 0x00 &
			mCardUser[i][4] == 0x00 & mCardUser[i][5] == 0x00 &
			mCardUser[i][6] == 0x00 ) {
				return i;
			}
	}
	return -1;
}


