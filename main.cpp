/**************************************************************************/
/*!

    @file          main.cpp
    @author        Adafruit Industries
    @modified_by   Ben Zhang <benzh@eecs.berkeley.edu>
    @license       BSD (see license.txt)
    
    This example will wait for any ISO14443A card or tag, and
    depending on the size of the UID will attempt to read from it.
   
    If the card has a 4-byte UID it is probably a Mifare
    Classic card, and the following steps are taken:
   
    - Authenticate block 4 (the first block of Sector 1) using
      the default KEYA of 0XFF 0XFF 0XFF 0XFF 0XFF 0XFF
    - If authentication succeeds, we can then read any of the
      4 blocks in that sector (though only block 4 is read here)
     
    If the card has a 7-byte UID it is probably a Mifare
    Ultralight card, and the 4 byte pages can be read directly.
    Page 4 is read by default since this is the first 'general-
    purpose' page on the tags.


This is an example sketch for the Adafruit PN532 NFC/RFID breakout boards
This library works with the Adafruit NFC breakout 
  ----> https://www.adafruit.com/products/364
 
Check out the links above for our tutorials and wiring diagrams 
These chips use SPI to communicate, 4 required to interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

*/
/**************************************************************************/

#include "mbed.h"
#include "Adafruit_PN532.h"
#include "cardcheck.h"
#include "delay.h"
#include "blinkLED.h"

#define SS   PTD0
// PTD1 is also LED_BLUE, it will blink during SPI communication.
#define SCK  PTD1
#define MOSI PTD2
#define MISO PTD3

//DigitalOut red(LED_RED);
//DigitalOut green(LED_GREEN);
//DigitalOut blue(LED_BLUE);

Serial pc(USBTX, USBRX);
Adafruit_PN532 nfc(SCK, MISO, MOSI, SS);

// This is a hard-coded UID from the card I will use as an "admin" card
uint8_t adminCard[] = {0xcd, 0x93, 0xc8, 0x23, 0, 0, 0,};
	
// Buffer to store each of the 5 possible security cards UIDs
//uint8_t cardUIDs[5][7] = 	{
//		{ 0, 0, 0, 0, 0, 0, 0 },
//		{ 0, 0, 0, 0, 0, 0, 0 },
//		{ 0, 0, 0, 0, 0, 0, 0 },
//		{ 0, 0, 0, 0, 0, 0, 0 },
//		{ 0, 0, 0, 0, 0, 0, 0 }
//};

// Count number of filled user spaces (limit is 5)
uint8_t savedUsers = 0;

// Declare blink class object
blinkLED b;
cardcheck cc;

// Potential default mifare classic 1k keya values for block authentication
uint8_t keyaa[9][6] = {
		{    0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF		},
		{    0XD3, 0XF7, 0XD3, 0XF7, 0XD3, 0XF7		},
		{    0XA0, 0XA1, 0XA2, 0XA3, 0XA4, 0XA5		},
		{    0XB0, 0XB1, 0XB2, 0XB3, 0XB4, 0XB5		},
		{    0X4D, 0X3A, 0X99, 0XC3, 0X51, 0XDD		},
		{    0X1A, 0X98, 0X2C, 0X7E, 0X45, 0X9A		},
		{    0XAA, 0XBB, 0XCC, 0XDD, 0XEE, 0XFF		},
		{    0X00, 0X00, 0X00, 0X00, 0X00, 0X00		},
		{    0XAB, 0XCD, 0XEF, 0X12, 0X34, 0X56		}
};

void loop(void);

int main() {
  pc.printf("Hello!\n");  
  // By default, no color
  //green = 1; red = 1, blue = 1;
	b.blink(1,1,1,100);

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    pc.printf("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  pc.printf("Found chip PN5%2X with Firmware ver. %d.%d\n",
            versiondata >> 24,
            (versiondata >> 16) & 0xFF,
            (versiondata >> 8) & 0xFF);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  
  pc.printf("Waiting for an ISO14443A Card ...\n");
  
  while(1) { loop(); }
}


void loop(void) {
	
	b.blink(1,1,1,100);
  // Turn back to no color 
  //green = 1; red = 1, blue = 1;
	// Delay class declaration
	 delay d;
	// Buffers
  uint8_t foundcard = 0x00;
	uint8_t cardIsAuth = 0x00;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Temp buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  foundcard = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
	
	// Enter this block if we have successfully found a card
	if (foundcard) {
		// Turn off to indicate Card swipe
    //green = 1; red = 1, blue = 1;
		b.blink(1,1,1,100);
		
		// Instantiate cardcheck object. Pass adminCard array, adminCard length, and userUIDs
		//cardcheck cc(adminCard, 4, cardUIDs);
		
		// See what type of card we have. Returns integer value
		int cardType = cc.cardType(uid, uidLength);
      
		switch(cardType) {
			
			// card is master	//
			case 3 :
				//blink blue
				b.blink(1,1,0,200);
				for(int i=0; i<5; i++) {
					
					uint8_t uidNew[] = { 0, 0, 0, 0, 0, 0, 0 };  // Temp buffer to store the returned UID
					uint8_t uidLengthNew;
					bool nextCard = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uidNew, &uidLengthNew);
					
					if(nextCard) {
						
						int cardTypeNew = cc.cardType(uidNew, uidLengthNew);
						
						switch(cardTypeNew) {
							
							// if master, break out of loop
							case 3:
								// blink 
								b.doubleBlink(0,1,0,200);
								i=5;
								break;
							
							// If user is introduced after master, remove it
							case 2:
								// blink green/blue
								//b.doubleBlink(1,0,0,200);
								cc.cardOpRemove(uidNew,uidLengthNew);
								i=5;
								break;
							
							// If new card, add them and break out of the loop.
							case 0:
								// Add user
								cc.cardOpAdd(uidNew,uidLengthNew);
								i=5;
								break;
							
						}
						
					}
					d.delayTime(500);
				}
				/*
				Enter "add user" mode here
				*/
				break;
			
			// card is user	//
			case 2 :
				//blink green
				b.blink(1,0,1,500);
				/*
				Activate motor here
				*/
				break;
			
			// card is nonuser	//
			case 0 :
				//blink red
				b.blink(0,1,1,500);
				/*
				Do nothing here
				*/
				break;
			
			default :
				// return something...?
				//double blink red
				b.doubleBlink(0,1,1,200);
				break;
		}
		wait(1);
		
	} else {
		// do something
	}
 
}