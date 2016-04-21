/**************************************************************************/
/*! 
    @file     Adafruit_PN532.cpp
    @author   Adafruit Industries
    @license  BSD (see license.txt)
    
    SPI Driver for NXP's PN532 NFC/13.56MHz RFID Transceiver

    This is a library for the Adafruit PN532 NFC/RFID breakout boards
    This library works with the Adafruit NFC breakout 
    ----> https://www.adafruit.com/products/364
    
    Check out the links above for our tutorials and wiring diagrams 
    These chips use SPI to communicate, 4 required to interface
    
    Adafruit invests time and resources providing this open source code, 
    please support Adafruit and open-source hardware by purchasing 
    products from Adafruit!


    @section  HISTORY

    v1.4 - Added setPassiveActivationRetries()
    
    v1.2 - Added writeGPIO()
         - Added readGPIO()

    v1.1 - Changed readPassiveTargetID() to handle multiple UID sizes
         - Added the following helper functions for text display
             static void PrintHex(const uint8_t * data, const uint32_t numuint8_ts)
             static void PrintHexChar(const uint8_t * pbtData, const uint32_t numuint8_ts)
         - Added the following Mifare Classic functions:
             bool mifareclassic_IsFirstBlock (uint32_t uiBlock)
             bool mifareclassic_IsTrailerBlock (uint32_t uiBlock)
             uint8_t mifareclassic_AuthenticateBlock (uint8_t * uid, uint8_t uidLen, uint32_t blockNumber, uint8_t keyNumber, uint8_t * keyData)
             uint8_t mifareclassic_ReadDataBlock (uint8_t blockNumber, uint8_t * data)
             uint8_t mifareclassic_WriteDataBlock (uint8_t blockNumber, uint8_t * data)
         - Added the following Mifare Ultalight functions:
             uint8_t mifareultralight_ReadPage (uint8_t page, uint8_t * buffer) 
*/
/**************************************************************************/

#include "Adafruit_PN532.h"

uint8_t pn532ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
uint8_t pn532response_firmwarevers[] = {0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03};

// Uncomment these lines to enable debug output for PN532(SPI) and/or MIFARE related code
// #define PN532DEBUG
// #define MIFAREDEBUG

Serial serial(USBTX, USBRX);
#define SERIAL_PRINT serial.printf

#define _BV(bit) (1 << (bit))
#define PN532_PACKBUFFSIZ 64
uint8_t pn532_packetbuffer[PN532_PACKBUFFSIZ];

void delay(int delayInMS) {
  wait(1.0 * delayInMS / 1000);
}

/**************************************************************************/
/*! 
    @brief  Instantiates a new PN532 class

    @param  clk       SPI clock pin (SCK)
    @param  miso      SPI MISO pin 
    @param  mosi      SPI MOSI pin
    @param  ss        SPI chip select pin (CS/SSEL)
*/
/**************************************************************************/
Adafruit_PN532::Adafruit_PN532(DigitalOut clk, DigitalIn miso,
                               DigitalOut mosi, DigitalOut ss)
    : _clk(clk), _miso(miso), _mosi(mosi), _ss(ss) {}

Adafruit_PN532::Adafruit_PN532(PinName clk_pin, PinName miso_pin,
                               PinName mosi_pin, PinName ss_pin)
    : _clk(DigitalOut(clk_pin)), _miso(DigitalIn(miso_pin)),
      _mosi(DigitalOut(mosi_pin)), _ss(DigitalOut(ss_pin)) {}

/**************************************************************************/
/*! 
    @brief  Setups the HW
*/
/**************************************************************************/
void Adafruit_PN532::begin() {
  _ss = 0;  
  delay(1000);

  // not exactly sure why but we have to send a dummy command to get synced up
  pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;
  sendCommandCheckAck(pn532_packetbuffer, 1);

  // ignore response!
}
 
/**************************************************************************/
/*! 
    @brief  Prints a hexadecimal value in plain characters

    @param  data      Pointer to the uint8_t data
    @param  numuint8_ts  Data length in uint8_ts
*/
/**************************************************************************/
void Adafruit_PN532::PrintHex(const uint8_t * data, const uint32_t numuint8_ts)
{
  uint32_t szPos;
  for (szPos=0; szPos < numuint8_ts; szPos++) 
  {
    SERIAL_PRINT("0x");
    // Append leading 0 for small values
    if (data[szPos] <= 0xF)
      SERIAL_PRINT("0");
    SERIAL_PRINT("%d", data[szPos]);
    if ((numuint8_ts > 1) && (szPos != numuint8_ts - 1))
    {
      SERIAL_PRINT(" ");
    }
  }
  SERIAL_PRINT("\n");
}

/**************************************************************************/
/*! 
    @brief  Prints a hexadecimal value in plain characters, along with
            the char equivalents in the following format

            00 00 00 00 00 00  ......

    @param  data      Pointer to the uint8_t data
    @param  numuint8_ts  Data length in uint8_ts
*/
/**************************************************************************/
void Adafruit_PN532::PrintHexChar(const uint8_t * data, const uint32_t numuint8_ts)
{
  uint32_t szPos;
  for (szPos=0; szPos < numuint8_ts; szPos++) 
  {
    // Append leading 0 for small values
    if (data[szPos] <= 0xF)
      SERIAL_PRINT("0");
    SERIAL_PRINT("%x", data[szPos]);
    if ((numuint8_ts > 1) && (szPos != numuint8_ts - 1))
    {
      SERIAL_PRINT(" ");
    }
  }
  SERIAL_PRINT(" ");
  for (szPos=0; szPos < numuint8_ts; szPos++) 
  {
    if (data[szPos] <= 0x1F)
      SERIAL_PRINT(".");
    else
      SERIAL_PRINT("%c", data[szPos]);
  }
  SERIAL_PRINT("");
}
 
/**************************************************************************/
/*! 
    @brief  Checks the firmware version of the PN5xx chip

    @returns  The chip's firmware version and ID
*/
/**************************************************************************/
uint32_t Adafruit_PN532::getFirmwareVersion(void) {
  uint32_t response;

  pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;
  
  if (! sendCommandCheckAck(pn532_packetbuffer, 1))
    return 0;
  
  // read data packet
  readspidata(pn532_packetbuffer, 12);
  
  // check some basic stuff
  if (0 != strncmp((char *)pn532_packetbuffer, (char *)pn532response_firmwarevers, 6)) {
    return 0;
  }
  
  response = pn532_packetbuffer[6];
  response <<= 8;
  response |= pn532_packetbuffer[7];
  response <<= 8;
  response |= pn532_packetbuffer[8];
  response <<= 8;
  response |= pn532_packetbuffer[9];

  return response;
}


/**************************************************************************/
/*! 
    @brief  Sends a command and waits a specified period for the ACK

    @param  cmd       Pointer to the command buffer
    @param  cmdlen    The size of the command in uint8_ts 
    @param  timeout   timeout before giving up
    
    @returns  1 if everything is OK, 0 if timeout occured before an
              ACK was recieved
*/
/**************************************************************************/
// default timeout of one second
bool Adafruit_PN532::sendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen, uint16_t timeout) {
  uint16_t timer = 0;
  
  // write the command
  spiwritecommand(cmd, cmdlen);
  
  // Wait for chip to say its ready!
  while (readspistatus() != PN532_SPI_READY) {
    if (timeout != 0) {
      timer+=10;
      if (timer > timeout)  
        return false;
    }
    delay(10);
  }
  
  // read acknowledgement
  if (!spi_readack()) {
    return false;
  }
  
  timer = 0;
  // Wait for chip to say its ready!
  while (readspistatus() != PN532_SPI_READY) {
    if (timeout != 0) {
      timer+=10;
      if (timer > timeout)  
        return false;
    }
    delay(10);
  }
  
  return true; // ack'd command
}

/**************************************************************************/
/*! 
    Writes an 8-bit value that sets the state of the PN532's GPIO pins
    
    @warning This function is provided exclusively for board testing and
             is dangerous since it will throw an error if any pin other
             than the ones marked "Can be used as GPIO" are modified!  All
             pins that can not be used as GPIO should ALWAYS be left high
             (value = 1) or the system will become unstable and a HW reset
             will be required to recover the PN532.
    
             pinState[0]  = P30     Can be used as GPIO
             pinState[1]  = P31     Can be used as GPIO
             pinState[2]  = P32     *** RESERVED (Must be 1!) ***
             pinState[3]  = P33     Can be used as GPIO
             pinState[4]  = P34     *** RESERVED (Must be 1!) ***
             pinState[5]  = P35     Can be used as GPIO
    
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool Adafruit_PN532::writeGPIO(uint8_t pinstate) {
  // Make sure pinstate does not try to toggle P32 or P34
  pinstate |= (1 << PN532_GPIO_P32) | (1 << PN532_GPIO_P34);
  
  // Fill command buffer
  pn532_packetbuffer[0] = PN532_COMMAND_WRITEGPIO;
  pn532_packetbuffer[1] = PN532_GPIO_VALIDATIONBIT | pinstate;  // P3 Pins
  pn532_packetbuffer[2] = 0x00;    // P7 GPIO Pins (not used ... taken by SPI)

  #ifdef PN532DEBUG
    SERIAL_PRINT("Writing P3 GPIO: "); SERIAL_PRINTln(pn532_packetbuffer[1], HEX);
  #endif

  // Send the WRITEGPIO command (0x0E)  
  if (! sendCommandCheckAck(pn532_packetbuffer, 3))
    return 0x0;
  
  // Read response packet (00 FF PLEN PLENCHECKSUM D5 CMD+1(0x0F) DATACHECKSUM 00)
  readspidata(pn532_packetbuffer, 8);

  #ifdef PN532DEBUG
    SERIAL_PRINT("Received: ");
    PrintHex(pn532_packetbuffer, 8);
    SERIAL_PRINTln("");
  #endif  
  
  return  (pn532_packetbuffer[5] == 0x0F);
}

/**************************************************************************/
/*! 
    Reads the state of the PN532's GPIO pins
    
    @returns An 8-bit value containing the pin state where:
    
             pinState[0]  = P30     
             pinState[1]  = P31     
             pinState[2]  = P32     
             pinState[3]  = P33     
             pinState[4]  = P34     
             pinState[5]  = P35     
*/
/**************************************************************************/
uint8_t Adafruit_PN532::readGPIO(void) {
  pn532_packetbuffer[0] = PN532_COMMAND_READGPIO;

  // Send the READGPIO command (0x0C)  
  if (! sendCommandCheckAck(pn532_packetbuffer, 1))
    return 0x0;
  
  // Read response packet (00 FF PLEN PLENCHECKSUM D5 CMD+1(0x0D) P3 P7 IO1 DATACHECKSUM 00)
  readspidata(pn532_packetbuffer, 11);

  /* READGPIO response should be in the following format:
  
    uint8_t            Description
    -------------   ------------------------------------------
    b0..5           Frame header and preamble
    b6              P3 GPIO Pins
    b7              P7 GPIO Pins (not used ... taken by SPI)
    b8              Interface Mode Pins (not used ... bus select pins) 
    b9..10          checksum */
  
  #ifdef PN532DEBUG
    SERIAL_PRINT("Received: ");
    PrintHex(pn532_packetbuffer, 11);
    SERIAL_PRINTln("");
    SERIAL_PRINT("P3 GPIO: 0x"); SERIAL_PRINTln(pn532_packetbuffer[6], HEX);
    SERIAL_PRINT("P7 GPIO: 0x"); SERIAL_PRINTln(pn532_packetbuffer[7], HEX);
    SERIAL_PRINT("IO GPIO: 0x"); SERIAL_PRINTln(pn532_packetbuffer[8], HEX);
    // Note: You can use the IO GPIO value to detect the serial bus being used
    switch(pn532_packetbuffer[8])
    {
      case 0x00:    // Using UART
        SERIAL_PRINTln("Using UART (IO = 0x00)");
        break;
      case 0x01:    // Using I2C 
        SERIAL_PRINTln("Using I2C (IO = 0x01)");
        break;
      case 0x02:    // Using SPI
        SERIAL_PRINTln("Using SPI (IO = 0x02)");
        break;
    }
  #endif

  return pn532_packetbuffer[6];
}

/**************************************************************************/
/*! 
    @brief  Configures the SAM (Secure Access Module)
*/
/**************************************************************************/
bool Adafruit_PN532::SAMConfig(void) {
  pn532_packetbuffer[0] = PN532_COMMAND_SAMCONFIGURATION;
  pn532_packetbuffer[1] = 0x01; // normal mode;
  pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
  pn532_packetbuffer[3] = 0x01; // use IRQ pin!
  
  if (! sendCommandCheckAck(pn532_packetbuffer, 4))
     return false;

  // read data packet
  readspidata(pn532_packetbuffer, 8);
  
  return  (pn532_packetbuffer[5] == 0x15);
}

/**************************************************************************/
/*! 
    Sets the MxRtyPassiveActivation uint8_t of the RFConfiguration register
    
    @param  maxRetries    0xFF to wait forever, 0x00..0xFE to timeout
                          after mxRetries
    
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool Adafruit_PN532::setPassiveActivationRetries(uint8_t maxRetries) {
  pn532_packetbuffer[0] = PN532_COMMAND_RFCONFIGURATION;
  pn532_packetbuffer[1] = 5;    // Config item 5 (MaxRetries)
  pn532_packetbuffer[2] = 0xFF; // MxRtyATR (default = 0xFF)
  pn532_packetbuffer[3] = 0x01; // MxRtyPSL (default = 0x01)
  pn532_packetbuffer[4] = maxRetries;

#ifdef MIFAREDEBUG
  SERIAL_PRINT("Setting MxRtyPassiveActivation to "); SERIAL_PRINT(maxRetries, DEC); SERIAL_PRINTln(" ");
#endif
  
  if (! sendCommandCheckAck(pn532_packetbuffer, 5))
    return 0x0;  // no ACK
  
  return 1;
}

/***** ISO14443A Commands ******/

/**************************************************************************/
/*! 
    Waits for an ISO14443A target to enter the field
    
    @param  cardBaudRate  Baud rate of the card
    @param  uid           Pointer to the array that will be populated
                          with the card's UID (up to 7 uint8_ts)
    @param  uidLength     Pointer to the variable that will hold the
                          length of the card's UID.
    
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool Adafruit_PN532::readPassiveTargetID(uint8_t cardbaudrate, uint8_t * uid, uint8_t * uidLength) {
  pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
  pn532_packetbuffer[1] = 1;  // max 1 cards at once (we can set this to 2 later)
  pn532_packetbuffer[2] = cardbaudrate;
  
  if (! sendCommandCheckAck(pn532_packetbuffer, 3))
    return 0x0;  // no cards read
  
  // read data packet
  readspidata(pn532_packetbuffer, 20);
  // check some basic stuff

  /* ISO14443A card response should be in the following format:
  
    uint8_t            Description
    -------------   ------------------------------------------
    b0..6           Frame header and preamble
    b7              Tags Found
    b8              Tag Number (only one used in this example)
    b9..10          SENS_RES
    b11             SEL_RES
    b12             NFCID Length
    b13..NFCIDLen   NFCID                                      */
  
#ifdef MIFAREDEBUG
    SERIAL_PRINT("Found "); SERIAL_PRINT(pn532_packetbuffer[7], DEC); SERIAL_PRINTln(" tags");
#endif
  if (pn532_packetbuffer[7] != 1) 
    return 0;
    
  uint16_t sens_res = pn532_packetbuffer[9];
  sens_res <<= 8;
  sens_res |= pn532_packetbuffer[10];
#ifdef MIFAREDEBUG
    SERIAL_PRINT("ATQA: 0x");  SERIAL_PRINTln(sens_res, HEX); 
    SERIAL_PRINT("SAK: 0x");  SERIAL_PRINTln(pn532_packetbuffer[11], HEX); 
#endif
  
  /* Card appears to be Mifare Classic */
  *uidLength = pn532_packetbuffer[12];
#ifdef MIFAREDEBUG
    SERIAL_PRINT("UID:"); 
#endif
  for (uint8_t i=0; i < pn532_packetbuffer[12]; i++) 
  {
    uid[i] = pn532_packetbuffer[13+i];
#ifdef MIFAREDEBUG
      SERIAL_PRINT(" 0x");SERIAL_PRINT(uid[i], HEX); 
#endif
  }
#ifdef MIFAREDEBUG
    SERIAL_PRINTln();
#endif

  uint8_t bTestDeselect[2];
  bTestDeselect[0] = PN532_COMMAND_INDESELECT;
  bTestDeselect[1] = pn532_packetbuffer[8];
  sendCommandCheckAck(bTestDeselect,2);

  return 1;
}


/***** Mifare Classic Functions ******/

/**************************************************************************/
/*! 
      Indicates whether the specified block number is the first block
      in the sector (block 0 relative to the current sector)
*/
/**************************************************************************/
bool Adafruit_PN532::mifareclassic_IsFirstBlock (uint32_t uiBlock)
{
  // Test if we are in the small or big sectors
  if (uiBlock < 128)
    return ((uiBlock) % 4 == 0);
  else
    return ((uiBlock) % 16 == 0);
}

/**************************************************************************/
/*! 
      Indicates whether the specified block number is the sector trailer
*/
/**************************************************************************/
bool Adafruit_PN532::mifareclassic_IsTrailerBlock (uint32_t uiBlock)
{
  // Test if we are in the small or big sectors
  if (uiBlock < 128)
    return ((uiBlock + 1) % 4 == 0);
  else
    return ((uiBlock + 1) % 16 == 0);
}

/**************************************************************************/
/*! 
    Tries to authenticate a block of memory on a MIFARE card using the
    INDATAEXCHANGE command.  See section 7.3.8 of the PN532 User Manual
    for more information on sending MIFARE and other commands.

    @param  uid           Pointer to a uint8_t array containing the card UID
    @param  uidLen        The length (in uint8_ts) of the card's UID (Should
                          be 4 for MIFARE Classic)
    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  keyNumber     Which key type to use during authentication
                          (0 = MIFARE_CMD_AUTH_A, 1 = MIFARE_CMD_AUTH_B)
    @param  keyData       Pointer to a uint8_t array containing the 6 uint8_t
                          key value
    
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareclassic_AuthenticateBlock (uint8_t * uid, uint8_t uidLen, uint32_t blockNumber, uint8_t keyNumber, uint8_t * keyData)
{
  uint8_t i;
  
  // Hang on to the key and uid data
  memcpy (_key, keyData, 6); 
  memcpy (_uid, uid, uidLen); 
  _uidLen = uidLen;  

  #ifdef MIFAREDEBUG
  SERIAL_PRINT("Trying to authenticate card ");
  Adafruit_PN532::PrintHex(_uid, _uidLen);
  SERIAL_PRINT("Using authentication KEY ");SERIAL_PRINT(keyNumber ? 'B' : 'A');SERIAL_PRINT(": ");
  Adafruit_PN532::PrintHex(_key, 6);
  #endif
  
  // Prepare the authentication command //
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;   /* Data Exchange Header */
  pn532_packetbuffer[1] = 1;                              /* Max card numbers */
  pn532_packetbuffer[2] = (keyNumber) ? MIFARE_CMD_AUTH_B : MIFARE_CMD_AUTH_A;
  pn532_packetbuffer[3] = blockNumber;                    /* Block Number (1K = 0..63, 4K = 0..255 */
  memcpy (pn532_packetbuffer+4, _key, 6);
  for (i = 0; i < _uidLen; i++)
  {
    pn532_packetbuffer[10+i] = _uid[i];                /* 4 uint8_t card ID */
  }

  if (! sendCommandCheckAck(pn532_packetbuffer, 10+_uidLen))
    return 0;

  // Read the response packet
  readspidata(pn532_packetbuffer, 12);
  // check if the response is valid and we are authenticated???
  // for an auth success it should be uint8_ts 5-7: 0xD5 0x41 0x00
  // Mifare auth error is technically uint8_t 7: 0x14 but anything other and 0x00 is not good
  if (pn532_packetbuffer[7] != 0x00)
  {
    #ifdef PN532DEBUG
    SERIAL_PRINT("Authentification failed: ");
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 12);
    #endif
    return 0;
  }

  return 1;
}

/**************************************************************************/
/*! 
    Tries to read an entire 16-uint8_t data block at the specified block
    address.

    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  data          Pointer to the uint8_t array that will hold the
                          retrieved data (if any)
    
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareclassic_ReadDataBlock (uint8_t blockNumber, uint8_t * data)
{
  #ifdef MIFAREDEBUG
  SERIAL_PRINT("Trying to read 16 uint8_ts from block ");SERIAL_PRINTln(blockNumber);
  #endif
  
  /* Prepare the command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;                      /* Card number */
  pn532_packetbuffer[2] = MIFARE_CMD_READ;        /* Mifare Read command = 0x30 */
  pn532_packetbuffer[3] = blockNumber;            /* Block Number (0..63 for 1K, 0..255 for 4K) */

  /* Send the command */
  if (! sendCommandCheckAck(pn532_packetbuffer, 4))
  {
    #ifdef MIFAREDEBUG
    SERIAL_PRINTln("Failed to receive ACK for read command");
    #endif
    return 0;
  }

  /* Read the response packet */
  readspidata(pn532_packetbuffer, 26);

  /* If uint8_t 8 isn't 0x00 we probably have an error */
  if (pn532_packetbuffer[7] != 0x00)
  {
    //#ifdef MIFAREDEBUG
    SERIAL_PRINT("Unexpected response");
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
    //#endif
    return 0;
  }
    
  /* Copy the 16 data uint8_ts to the output buffer        */
  /* Block content starts at uint8_t 9 of a valid response */
  memcpy (data, pn532_packetbuffer+8, 16);

  /* Display data for debug if requested */
  #ifdef MIFAREDEBUG
    SERIAL_PRINT("Block ");
    SERIAL_PRINTln(blockNumber);
    Adafruit_PN532::PrintHexChar(data, 16);
  #endif

  return 1;  
}

/**************************************************************************/
/*! 
    Tries to write an entire 16-uint8_t data block at the specified block
    address.

    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  data          The uint8_t array that contains the data to write.
    
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareclassic_WriteDataBlock (uint8_t blockNumber, uint8_t * data)
{
  #ifdef MIFAREDEBUG
  SERIAL_PRINT("Trying to write 16 uint8_ts to block ");SERIAL_PRINTln(blockNumber);
  #endif
  
  /* Prepare the first command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;                      /* Card number */
  pn532_packetbuffer[2] = MIFARE_CMD_WRITE;       /* Mifare Write command = 0xA0 */
  pn532_packetbuffer[3] = blockNumber;            /* Block Number (0..63 for 1K, 0..255 for 4K) */
  memcpy (pn532_packetbuffer+4, data, 16);          /* Data Payload */

  /* Send the command */
  if (! sendCommandCheckAck(pn532_packetbuffer, 20))
  {
    #ifdef MIFAREDEBUG
    SERIAL_PRINTln("Failed to receive ACK for write command");
    #endif
    return 0;
  }  
  delay(10);
  
  /* Read the response packet */
  readspidata(pn532_packetbuffer, 26);

  return 1;  
}

/**************************************************************************/
/*! 
    Formats a Mifare Classic card to store NDEF Records 
    
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareclassic_FormatNDEF (void)
{
  uint8_t sectorbuffer1[16] = {0x14, 0x01, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1};
  uint8_t sectorbuffer2[16] = {0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1, 0x03, 0xE1};
  uint8_t sectorbuffer3[16] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0x78, 0x77, 0x88, 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  // Write block 1 and 2 to the card
  if (!(mifareclassic_WriteDataBlock (1, sectorbuffer1)))
    return 0;
  if (!(mifareclassic_WriteDataBlock (2, sectorbuffer2)))
    return 0;
  // Write key A and access rights card
  if (!(mifareclassic_WriteDataBlock (3, sectorbuffer3)))
    return 0;

  // Seems that everything was OK (?!)
  return 1;
}

/**************************************************************************/
/*! 
    Writes an NDEF URI Record to the specified sector (1..15)
    
    Note that this function assumes that the Mifare Classic card is
    already formatted to work as an "NFC Forum Tag" and uses a MAD1
    file system.  You can use the NXP TagWriter app on Android to
    properly format cards for this.

    @param  sectorNumber  The sector that the URI record should be written
                          to (can be 1..15 for a 1K card)
    @param  uriIdentifier The uri identifier code (0 = none, 0x01 = 
                          "http://www.", etc.)
    @param  url           The uri text to write (max 38 characters).
    
    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareclassic_WriteNDEFURI (uint8_t sectorNumber, uint8_t uriIdentifier, const char * url)
{
  // Figure out how long the string is
  uint8_t len = strlen(url);
  
  // Make sure we're within a 1K limit for the sector number
  if ((sectorNumber < 1) || (sectorNumber > 15))
    return 0;
  
  // Make sure the URI payload is between 1 and 38 chars
  if ((len < 1) || (len > 38))
    return 0;
    
  // Setup the sector buffer (w/pre-formatted TLV wrapper and NDEF message)
  uint8_t sectorbuffer1[16] = {0x00, 0x00, 0x03, len+5, 0xD1, 0x01, len+1, 0x55, uriIdentifier, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t sectorbuffer2[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t sectorbuffer3[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t sectorbuffer4[16] = {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7, 0x7F, 0x07, 0x88, 0x40, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  if (len <= 6)
  {
    // Unlikely we'll get a url this short, but why not ...
    memcpy (sectorbuffer1+9, url, len);
    sectorbuffer1[len+9] = 0xFE;
  }
  else if (len == 7)
  {
    // 0xFE needs to be wrapped around to next block
    memcpy (sectorbuffer1+9, url, len);
    sectorbuffer2[0] = 0xFE;
  }
  else if ((len > 7) || (len <= 22))
  {
    // Url fits in two blocks
    memcpy (sectorbuffer1+9, url, 7);
    memcpy (sectorbuffer2, url+7, len-7);
    sectorbuffer2[len-7] = 0xFE;
  }
  else if (len == 23)
  {
    // 0xFE needs to be wrapped around to final block
    memcpy (sectorbuffer1+9, url, 7);
    memcpy (sectorbuffer2, url+7, len-7);
    sectorbuffer3[0] = 0xFE;
  }
  else
  {
    // Url fits in three blocks
    memcpy (sectorbuffer1+9, url, 7);
    memcpy (sectorbuffer2, url+7, 16);
    memcpy (sectorbuffer3, url+23, len-24);
    sectorbuffer3[len-22] = 0xFE;
  }
  
  // Now write all three blocks back to the card
  if (!(mifareclassic_WriteDataBlock (sectorNumber*4, sectorbuffer1)))
    return 0;
  if (!(mifareclassic_WriteDataBlock ((sectorNumber*4)+1, sectorbuffer2)))
    return 0;
  if (!(mifareclassic_WriteDataBlock ((sectorNumber*4)+2, sectorbuffer3)))
    return 0;
  if (!(mifareclassic_WriteDataBlock ((sectorNumber*4)+3, sectorbuffer4)))
    return 0;

  // Seems that everything was OK (?!)
  return 1;
}

/***** Mifare Ultralight Functions ******/

/**************************************************************************/
/*! 
    Tries to read an entire 4-uint8_t page at the specified address.

    @param  page        The page number (0..63 in most cases)
    @param  buffer      Pointer to the uint8_t array that will hold the
                        retrieved data (if any)
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareultralight_ReadPage (uint8_t page, uint8_t * buffer)
{
  if (page >= 64)
  {
    #ifdef MIFAREDEBUG
    SERIAL_PRINTln("Page value out of range");
    #endif
    return 0;
  }

  #ifdef MIFAREDEBUG
    SERIAL_PRINT("Reading page ");SERIAL_PRINTln(page);
  #endif

  /* Prepare the command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;                   /* Card number */
  pn532_packetbuffer[2] = MIFARE_CMD_READ;     /* Mifare Read command = 0x30 */
  pn532_packetbuffer[3] = page;                /* Page Number (0..63 in most cases) */

  /* Send the command */
  if (! sendCommandCheckAck(pn532_packetbuffer, 4))
  {
    #ifdef MIFAREDEBUG
    SERIAL_PRINTln("Failed to receive ACK for write command");
    #endif
    return 0;
  }
  
  /* Read the response packet */
  readspidata(pn532_packetbuffer, 26);
  #ifdef MIFAREDEBUG
    SERIAL_PRINTln("Received: ");
    Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
  #endif

  /* If uint8_t 8 isn't 0x00 we probably have an error */
  if (pn532_packetbuffer[7] == 0x00)
  {
    /* Copy the 4 data uint8_ts to the output buffer         */
    /* Block content starts at uint8_t 9 of a valid response */
    /* Note that the command actually reads 16 uint8_t or 4  */
    /* pages at a time ... we simply discard the last 12  */
    /* uint8_ts                                              */
    memcpy (buffer, pn532_packetbuffer+8, 4);
  }
  else
  {
    #ifdef MIFAREDEBUG
      SERIAL_PRINTln("Unexpected response reading block: ");
      Adafruit_PN532::PrintHexChar(pn532_packetbuffer, 26);
    #endif
    return 0;
  }

  /* Display data for debug if requested */
  #ifdef MIFAREDEBUG
    SERIAL_PRINT("Page ");SERIAL_PRINT(page);SERIAL_PRINTln(":");
    Adafruit_PN532::PrintHexChar(buffer, 4);
  #endif

  // Return OK signal
  return 1;
}



/************** high level SPI */


/**************************************************************************/
/*! 
    @brief  Tries to read the SPI ACK signal
*/
/**************************************************************************/
bool Adafruit_PN532::spi_readack() {
  uint8_t ackbuff[6];
  
  readspidata(ackbuff, 6);
  
  return (0 == strncmp((char *)ackbuff, (char *)pn532ack, 6));
}

/************** mid level SPI */

/**************************************************************************/
/*! 
    @brief  Reads the SPI status register (to know if the PN532 is ready)
*/
/**************************************************************************/
uint8_t Adafruit_PN532::readspistatus(void) {
  _ss = 0;
  delay(2);
  spiwrite(PN532_SPI_STATREAD);
  // read uint8_t
  uint8_t x = spiread();
  
  _ss = 1;
  return x;
}

/**************************************************************************/
/*! 
    @brief  Reads n uint8_ts of data from the PN532 via SPI

    @param  buff      Pointer to the buffer where data will be written
    @param  n         Number of uint8_ts to be read
*/
/**************************************************************************/
void Adafruit_PN532::readspidata(uint8_t* buff, uint8_t n) {
  _ss = 0;
  delay(2); 
  spiwrite(PN532_SPI_DATAREAD);

#ifdef PN532DEBUG
  SERIAL_PRINT("Reading: ");
#endif
  for (uint8_t i=0; i<n; i++) {
    delay(1);
    buff[i] = spiread();
#ifdef PN532DEBUG
    SERIAL_PRINT(" 0x");
    SERIAL_PRINT(buff[i], HEX);
#endif
  }

#ifdef PN532DEBUG
  SERIAL_PRINTln();
#endif

  _ss = 1;
}

/**************************************************************************/
/*! 
    @brief  Writes a command to the PN532, automatically inserting the
            preamble and required frame details (checksum, len, etc.)

    @param  cmd       Pointer to the command buffer
    @param  cmdlen    Command length in uint8_ts 
*/
/**************************************************************************/
void Adafruit_PN532::spiwritecommand(uint8_t* cmd, uint8_t cmdlen) {
  uint8_t checksum;

  cmdlen++;
  
#ifdef PN532DEBUG
  SERIAL_PRINT("\nSending: ");
#endif

  _ss = 0;
  delay(2);     // or whatever the delay is for waking up the board
  spiwrite(PN532_SPI_DATAWRITE);

  checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2;
  spiwrite(PN532_PREAMBLE);
  spiwrite(PN532_PREAMBLE);
  spiwrite(PN532_STARTCODE2);

  spiwrite(cmdlen);
  spiwrite(~cmdlen + 1);
 
  spiwrite(PN532_HOSTTOPN532);
  checksum += PN532_HOSTTOPN532;

#ifdef PN532DEBUG
  SERIAL_PRINT(" 0x"); SERIAL_PRINT(PN532_PREAMBLE, HEX);
  SERIAL_PRINT(" 0x"); SERIAL_PRINT(PN532_PREAMBLE, HEX);
  SERIAL_PRINT(" 0x"); SERIAL_PRINT(PN532_STARTCODE2, HEX);
  SERIAL_PRINT(" 0x"); SERIAL_PRINT(cmdlen, HEX);
  SERIAL_PRINT(" 0x"); SERIAL_PRINT(~cmdlen + 1, HEX);
  SERIAL_PRINT(" 0x"); SERIAL_PRINT(PN532_HOSTTOPN532, HEX);
#endif

  for (uint8_t i=0; i<cmdlen-1; i++) {
   spiwrite(cmd[i]);
   checksum += cmd[i];
#ifdef PN532DEBUG
   SERIAL_PRINT(" 0x"); SERIAL_PRINT(cmd[i], HEX);
#endif
  }
  
  spiwrite(~checksum);
  spiwrite(PN532_POSTAMBLE);
  _ss = 1;

#ifdef PN532DEBUG
  SERIAL_PRINT(" 0x"); SERIAL_PRINT(~checksum, HEX);
  SERIAL_PRINT(" 0x"); SERIAL_PRINT(PN532_POSTAMBLE, HEX);
  SERIAL_PRINTln();
#endif
} 
/************** low level SPI */

/**************************************************************************/
/*! 
    @brief  Low-level SPI write wrapper

    @param  c       8-bit command to write to the SPI bus
*/
/**************************************************************************/
void Adafruit_PN532::spiwrite(uint8_t c) {
  int8_t i;
  _clk = 1;
  
  for (i=0; i<8; i++) {
    _clk = 0;
    if (c & _BV(i)) {
      _mosi = 1;
    } else {
      _mosi = 0;
    }
    _clk = 1;
  }
}

/**************************************************************************/
/*! 
    @brief  Low-level SPI read wrapper

    @returns The 8-bit value that was read from the SPI bus
*/
/**************************************************************************/
uint8_t Adafruit_PN532::spiread(void) {
  int8_t i, x;
  x = 0;
  _clk = 1;

  for (i=0; i<8; i++) {
    if (_miso.read()) {
      x |= _BV(i);
    }
    _clk = 0;
    _clk = 1;
  }
  return x;
}