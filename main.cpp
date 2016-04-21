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

#define SS   PTD0
// PTD1 is also LED_BLUE, it will blink during SPI communication.
#define SCK  PTD1
#define MOSI PTD2
#define MISO PTD3

DigitalOut red(LED_RED);
DigitalOut green(LED_GREEN);
DigitalOut blue(LED_BLUE);

Serial pc(USBTX, USBRX);
Adafruit_PN532 nfc(SCK, MISO, MOSI, SS);

// This is a hard-coded UID from the card I will use as an "admin" card
uint8_t adminCard[] = {0xcd, 0x93, 0xc8, 0x23, 0, 0, 0,};
	
// Buffer to store each of the 5 possible security cards UIDs
uint8_t cardUIDs[5][7] = 	{
		{ 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0 }
};

uint8_t savedCards = 0;

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
  green = 1; red = 1, blue = 1;

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
  // Turn back to no color 
  green = 1; red = 1, blue = 1;

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
    green = 1; red = 1, blue = 1;
      
		
		// If we have a MIFARE CLASSIC card
		if (uidLength == 4) {
			
			// Check to see if it's the admin card
			if(uid[0]==adminCard[0] & uid[1]==adminCard[1] & uid[2]==adminCard[2] & uid[3]==adminCard[3]) {
				blue =0;
			}
			else {
				red = 0;
			}
			
		}
		
		// If we have a MIFARE LIGHT card
		else if (uidLength == 7) {
			
			// Check to see if it's the admin card
			if(uid[0]==adminCard[0] & uid[1]==adminCard[1] & uid[2]==adminCard[2] & uid[3]==adminCard[3]) {
				blue =0;
			}
			else {
				red = 0;
			}
			
		}
		
	} else {
		// do something
	}
  
	
	
	
	
	
	
	
	
	
	
	
	
	
	
//  if (success) {
//    // Turn off to indicate Card swipe
//    green = 1; red = 1, blue = 1;
//    // Display some basic information about the card
//    pc.printf("\n\nFound an ISO14443A card\n");
//    pc.printf("  UID Length: %d bytes", uidLength);
//    pc.printf("  UID Value: ");
//    nfc.PrintHex(uid, uidLength);
//    pc.printf("\n");
//    
//    if (uidLength == 4)
//    {
//      // We probably have a Mifare Classic card ... 
//      pc.printf("Seems to be a Mifare Classic card (4 byte UID)\n");
//      
//      // Now we need to try to authenticate it for read/write access
//      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
//      pc.printf("Trying to authenticate block 4 with default KEYA value\n");
//			
//			// Potential default mifare classic 1k keya values for block authentication
//			uint8_t keyaa[9][6] = {
//																{    0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF		},
//																{    0XD3, 0XF7, 0XD3, 0XF7, 0XD3, 0XF7		},
//																{    0XA0, 0XA1, 0XA2, 0XA3, 0XA4, 0XA5		},
//																{    0XB0, 0XB1, 0XB2, 0XB3, 0XB4, 0XB5		},
//																{    0X4D, 0X3A, 0X99, 0XC3, 0X51, 0XDD		},
//																{    0X1A, 0X98, 0X2C, 0X7E, 0X45, 0X9A		},
//																{    0XAA, 0XBB, 0XCC, 0XDD, 0XEE, 0XFF		},
//																{    0X00, 0X00, 0X00, 0X00, 0X00, 0X00		},
//																{    0XAB, 0XCD, 0XEF, 0X12, 0X34, 0X56		}
//			};
//      
//			// Custom code to attempt each of the potential default mifare classic 1k keya values
//			for(int i=0; i<sizeof(keyaa)&&i<10; i++) {
//				if(!success) {
//					success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keyaa[i]);
//				}
//			}
//      
//      if (success)
//      {
//        // Turn colors to indicate which tag
//        //green = 0; red = 1;
//        uint8_t tag_1 = 0xCD;	// Hard coded admin card
//        uint8_t tag_2 = 0x52;
//				
//				// If we have the admin card
//        if (uid[0] == tag_1) {
//          blue = 0;
//					// check for next card to appear
//					
//				}
//				
//        else if (uid[0] == tag_2) {
//						green = 0;
//				}
//				else 
//					red = 0;
//        
//        pc.printf("Sector 1 (Blocks 4..7) has been authenticated\n");
//        uint8_t data[16];
//        
//        // If you want to write something to block 4 to test with, remove
//        // the definition of data above, and uncomment the following lines
//        /************************************************************************
//        uint8_t data[16] = { 'b', 'e', 'n', ' ', 'z', 'h', 'a', 'n', 'g' };
//        success = nfc.mifareclassic_WriteDataBlock (4, data);
//        if (success)
//        {
//          // Data seems to have been read ... spit it out
//          pc.printf("Data Written to Block 4:\n\t");
//          nfc.PrintHexChar(data, 16);
//          pc.printf("\n");
//          
//          // Wait a bit before reading the card again
//          wait(1);
//        }
//        else
//        {
//          pc.printf("Ooops ... unable to write the requested block.\n");
//        }
//        return;
//        ************************************************************************/
//        
//        // Try to read the contents of block 4
//        /*success = nfc.mifareclassic_ReadDataBlock(4, data);
//        
//        if (success)
//        {
//          // Data seems to have been read ... spit it out
//          pc.printf("Reading Block 4:\n\t");
//          nfc.PrintHexChar(data, 16);
//          pc.printf("\n");
//          
//          // Wait a bit before reading the card again
//          wait(1);
//        }
//        else
//        {
//          pc.printf("Ooops ... unable to read the requested block.  Try another key?\n");
//        }*/
//      }
//      else
//      {
//        pc.printf("Ooops ... authentication failed: Try another key?\n");
//      }
//    }
//    
//    if (uidLength == 7)
//    {
//      // We probably have a Mifare Ultralight card ...
//      pc.printf("Seems to be a Mifare Ultralight tag (7 byte UID)\n");
//      
//      // Try to read the first general-purpose user page (#4)
//      pc.printf("Reading page 4\n");
//      uint8_t data[32];
//      success = nfc.mifareultralight_ReadPage (4, data);
//      if (success)
//      {
//        // Data seems to have been read ... spit it out
//        nfc.PrintHexChar(data, 4);
//        pc.printf("\n");
//        
//        // Wait a bit before reading the card again
//        wait(1);
//      }
//      else
//      {
//        pc.printf("Ooops ... unable to read the requested page!?\n");
//      }
//    }
//  }
}