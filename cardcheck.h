/*********************************************
* Author: Lane Mills
*	Date  : 04/22/16
* 
*
*
*
*/


#include <stdint.h>

#include "mbed.h"


class cardcheck {
	public:
		//cardcheck(uint8_t masterUID[], uint8_t mstrUIDLength, uint8_t cardUsers[][7]);
		cardcheck();
		int cardType(uint8_t uid[], uint8_t uidLength);
		void cardOpAdd(uint8_t uid[], uint8_t uidLength);
		void cardOpRemove(uint8_t uid[], uint8_t uidLength);
	
	private:
		int verify(uint8_t uid[], uint8_t uidLength);
		bool isMaster(uint8_t uid[], uint8_t uidLength);
		bool isUser(uint8_t uid[], uint8_t uidLength);
		void removeUser(int index);
		void addUser(uint8_t uid[], int index);
		void clean();
		bool isEmpty(int index);
		int getIndex(uint8_t uid[], uint8_t uidLength);
		bool hasFreeSpace();
		int getFreeIndex();
	
};