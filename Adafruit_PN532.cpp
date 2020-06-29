/**************************************************************************
PHAN HOANG
1/1/2020
file for nfc532
**************************************************************************/
#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <SPI.h>

#include "Adafruit_PN532.h"

byte pn532ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
byte pn532response_firmwarevers[] = {0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03};

#define PN532_SPI_CLOCKDIV SPI_CLOCK_DIV16
#define PN532_PACKBUFFSIZ 64
byte pn532_packetbuffer[PN532_PACKBUFFSIZ];
#ifndef _BV
    #define _BV(bit) (1<<(bit))
#endif
/**************************************************************************/
/*!
    Instantiates a new PN532 class using software SPI.
    @param  clk       SPI clock pin (SCK)
    @param  miso      SPI MISO pin
    @param  mosi      SPI MOSI pin
    @param  ss        SPI chip select pin (CS/SSEL)
*/
/**************************************************************************/
Adafruit_PN532::Adafruit_PN532(uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t ss):
  _clk(clk),
  _miso(miso),
  _mosi(mosi),
  _ss(ss),
  _irq(0),
  _reset(0),
  _usingSPI(true),
  _hardwareSPI(false)
{
  pinMode(_ss, OUTPUT);
  digitalWrite(_ss, HIGH); 
  pinMode(_clk, OUTPUT);
  pinMode(_mosi, OUTPUT);
  pinMode(_miso, INPUT);
}
/**************************************************************************/
/*
    @brief  Setups the HW
*/
/**************************************************************************/
void Adafruit_PN532::begin() {
  if (_usingSPI) {
    // SPI initialization
    digitalWrite(_ss, LOW);
    delay(1000);
    pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION; //đồng bộ hoá
    sendCommandCheckAck(pn532_packetbuffer, 1);
    // ignore response!
    digitalWrite(_ss, HIGH);
  }
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

  if (! sendCommandCheckAck(pn532_packetbuffer, 1)) {
    return 0;
  }

  // read data packet
  readdata(pn532_packetbuffer, 12);

  // check some basic stuff
  if (0 != strncmp((char *)pn532_packetbuffer, (char *)pn532response_firmwarevers, 6)){
    return 0;
  }

  int offset = _usingSPI ? 6 : 7;  // Skip a response byte when using I2C to ignore extra data.
  response = pn532_packetbuffer[offset++];
  response <<= 8;
  response |= pn532_packetbuffer[offset++];
  response <<= 8;
  response |= pn532_packetbuffer[offset++];
  response <<= 8;
  response |= pn532_packetbuffer[offset++];
  return response;
}


/**************************************************************************/
/*!
    @brief  Sends a command and waits a specified period for the ACK

    @param  cmd       Pointer to the command buffer
    @param  cmdlen    The size of the command in bytes
    @param  timeout   timeout before giving up

    @returns  1 if everything is OK, 0 if timeout occured before an
              ACK was recieved
*/
/**************************************************************************/
// default timeout of one second
bool Adafruit_PN532::sendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen, uint16_t timeout) {
  uint16_t timer = 0;

  // write the command
  writecommand(cmd, cmdlen);

  // Wait for chip to say its ready!
  if (!waitready(timeout)) {
    return false;
  }
  // read acknowledgement
  if (!readack()) {
    return false;
  }
  // For SPI only wait for the chip to be ready again.
  // This is unnecessary with I2C.
  if (_usingSPI) {
    if (!waitready(timeout)) {
      return false;
    }
  }
  return true; // ack'd command
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
  readdata(pn532_packetbuffer, 8);
  int offset = _usingSPI ? 5 : 6;
  return  (pn532_packetbuffer[offset] == 0x15);
}

/**************************************************************************/
/*!
    Sets the MxRtyPassiveActivation byte of the RFConfiguration register

    @param  maxRetries    0xFF to wait forever, 0x00..0xFE to timeout
                          after mxRetries

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
//bool Adafruit_PN532::setPassiveActivationRetries(uint8_t maxRetries) {
//  pn532_packetbuffer[0] = PN532_COMMAND_RFCONFIGURATION;
//  pn532_packetbuffer[1] = 5;    // Config item 5 (MaxRetries)
//  pn532_packetbuffer[2] = 0xFF; // MxRtyATR (default = 0xFF)
//  pn532_packetbuffer[3] = 0x01; // MxRtyPSL (default = 0x01)
//  pn532_packetbuffer[4] = maxRetries;
//
//  if (! sendCommandCheckAck(pn532_packetbuffer, 5))
//    return 0x0;  // no ACK
//  return 1;
//}

/***** ISO14443A Commands ******/

/**************************************************************************/
/*!
    Waits for an ISO14443A target to enter the field
    @param  cardBaudRate  Baud rate of the card
    @param  uid           Pointer to the array that will be populated
                          with the card's UID (up to 7 bytes)
    @param  uidLength     Pointer to the variable that will hold the
                          length of the card's UID.

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
bool Adafruit_PN532::readPassiveTargetID(uint8_t cardbaudrate, uint8_t * uid, uint8_t * uidLength, uint16_t timeout) {
  pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
  pn532_packetbuffer[1] = 1;  // max 1 cards at once (we can set this to 2 later)
  pn532_packetbuffer[2] = cardbaudrate;

  if (!sendCommandCheckAck(pn532_packetbuffer, 3, 50))
  {
    return 0;  // no cards read
  }
  // read data packet
  readdata(pn532_packetbuffer, 20);
  if(pn532_packetbuffer[7] != 1){return 0;}
  uint16_t sens_res = pn532_packetbuffer[9];
  sens_res <<= 8;
  sens_res |= pn532_packetbuffer[10];
  /* Card appears to be Mifare Classic */
  *uidLength = pn532_packetbuffer[12];
  for (uint8_t i=0; i < pn532_packetbuffer[12]; i++)
  {
    uid[i] = pn532_packetbuffer[13+i];
  }
  return 1;
}

/**************************************************************************/
/*!
    Tries to authenticate a block of memory on a MIFARE card using the
    INDATAEXCHANGE command.  See section 7.3.8 of the PN532 User Manual
    for more information on sending MIFARE and other commands.

    @param  uid           Pointer to a byte array containing the card UID
    @param  uidLen        The length (in bytes) of the card's UID (Should
                          be 4 for MIFARE Classic)
    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  keyNumber     Which key type to use during authentication
                          (0 = MIFARE_CMD_AUTH_A, 1 = MIFARE_CMD_AUTH_B)
    @param  keyData       Pointer to a byte array containing the 6 byte
                          key value

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareclassic_AuthenticateBlock (uint8_t * uid, uint8_t uidLen, uint32_t blockNumber, uint8_t keyNumber, uint8_t * keyData)
{
  uint8_t len;
  uint8_t i;

  // Hang on to the key and uid data
  memcpy (_key, keyData, 6);
  memcpy (_uid, uid, uidLen);
  _uidLen = uidLen;
  // Prepare the authentication command //
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;   /* Data Exchange Header */
  pn532_packetbuffer[1] = 1;                              /* Max card numbers */
  pn532_packetbuffer[2] = (keyNumber) ? MIFARE_CMD_AUTH_B : MIFARE_CMD_AUTH_A;
  pn532_packetbuffer[3] = blockNumber;                    /* Block Number (1K = 0..63, 4K = 0..255 */
  memcpy (pn532_packetbuffer+4, _key, 6);
  for (i = 0; i < _uidLen; i++)
  {
    pn532_packetbuffer[10+i] = _uid[i];                /* 4 byte card ID */
  }

  if (! sendCommandCheckAck(pn532_packetbuffer, 10+_uidLen))
    return 0;

  // Read the response packet
  readdata(pn532_packetbuffer, 12);

  // check if the response is valid and we are authenticated???
  // for an auth success it should be bytes 5-7: 0xD5 0x41 0x00
  // Mifare auth error is technically byte 7: 0x14 but anything other and 0x00 is not good
  if (pn532_packetbuffer[7] != 0x00)
  {
    return 0;
  }
  return 1;
}

/**************************************************************************/
/*!
    Tries to read an entire 16-byte data block at the specified block
    address.

    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  data          Pointer to the byte array that will hold the
                          retrieved data (if any)

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareclassic_ReadDataBlock (uint8_t blockNumber, uint8_t * data)
{
  /* Prepare the command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;                      /* Card number */
  pn532_packetbuffer[2] = MIFARE_CMD_READ;        /* Mifare Read command = 0x30 */
  pn532_packetbuffer[3] = blockNumber;            /* Block Number (0..63 for 1K, 0..255 for 4K) */

  /* Send the command */
  if (! sendCommandCheckAck(pn532_packetbuffer, 4))
  {
    return 0;
  }

  /* Read the response packet */
  readdata(pn532_packetbuffer, 26);

  /* If byte 8 isn't 0x00 we probably have an error */
  if (pn532_packetbuffer[7] != 0x00)
  {
    return 0;
  }

  /* Copy the 16 data bytes to the output buffer        */
  /* Block content starts at byte 9 of a valid response */
  memcpy (data, pn532_packetbuffer+8, 16);
  /* Display data for debug if requested */
  return 1;
}

/**************************************************************************/
/*!
    Tries to write an entire 16-byte data block at the specified block
    address.

    @param  blockNumber   The block number to authenticate.  (0..63 for
                          1KB cards, and 0..255 for 4KB cards).
    @param  data          The byte array that contains the data to write.

    @returns 1 if everything executed properly, 0 for an error
*/
/**************************************************************************/
uint8_t Adafruit_PN532::mifareclassic_WriteDataBlock (uint8_t blockNumber, uint8_t * data)
{
  /* Prepare the first command */
  pn532_packetbuffer[0] = PN532_COMMAND_INDATAEXCHANGE;
  pn532_packetbuffer[1] = 1;                      /* Card number */
  pn532_packetbuffer[2] = MIFARE_CMD_WRITE;       /* Mifare Write command = 0xA0 */
  pn532_packetbuffer[3] = blockNumber;            /* Block Number (0..63 for 1K, 0..255 for 4K) */
  memcpy (pn532_packetbuffer+4, data, 16);          /* Data Payload */

  /* Send the command */
  if (! sendCommandCheckAck(pn532_packetbuffer, 20))
  {
    return 0;
  }
  delay(10);

  /* Read the response packet */
  readdata(pn532_packetbuffer, 26);

  return 1;
}

/**************************************************************************/
/*!
    @brief  Tries to read the SPI or I2C ACK signal
*/
/**************************************************************************/
bool Adafruit_PN532::readack() {
  uint8_t ackbuff[6];

  readdata(ackbuff, 6);
  return (0 == strncmp((char *)ackbuff, (char *)pn532ack, 6));
}

/**************************************************************************/
/*!
    @brief  Return true if the PN532 is ready with a response.
*/
/**************************************************************************/
bool Adafruit_PN532::isready() {
  if (_usingSPI) {
    // SPI read status and check if ready.
    digitalWrite(_ss, LOW);
    delay(2);
    spi_write(PN532_SPI_STATREAD);
    // read byte
    uint8_t x = spi_read();
    digitalWrite(_ss, HIGH);
    // Check if status is ready.
    return x == PN532_SPI_READY;
  }
}

/**************************************************************************/
/*!
    @brief  Waits until the PN532 is ready.

    @param  timeout   Timeout before giving up
*/
/**************************************************************************/
bool Adafruit_PN532::waitready(uint16_t timeout) {
  uint16_t timer = 0;
  while(!isready()) {
    if (timeout != 0) {
      timer += 10;
      if (timer > timeout) {
        return false;
      }
    }
    delay(10);
  }
  return true;
}

/**************************************************************************/
/*!
    @brief  Reads n bytes of data from the PN532 via SPI

    @param  buff      Pointer to the buffer where data will be written
    @param  n         Number of bytes to be read
*/
/**************************************************************************/
void Adafruit_PN532::readdata(uint8_t* buff, uint8_t n) {
  if (_usingSPI) {
    // SPI write.
    digitalWrite(_ss, LOW);
    delay(2);
    spi_write(PN532_SPI_DATAREAD);
    for (uint8_t i=0; i<n; i++) {
      delay(1);
      buff[i] = spi_read();
    }
    digitalWrite(_ss, HIGH);
  }
}

/**************************************************************************/
/*!
    @brief  Writes a command to the PN532, automatically inserting the
            preamble and required frame details (checksum, len, etc.)

    @param  cmd       Pointer to the command buffer
    @param  cmdlen    Command length in bytes
*/
/**************************************************************************/
void Adafruit_PN532::writecommand(uint8_t* cmd, uint8_t cmdlen) {
  if (_usingSPI) {
    // SPI command write.
    uint8_t checksum;
    cmdlen++;

    digitalWrite(_ss, LOW);
    delay(2);     // or whatever the delay is for waking up the board
    spi_write(PN532_SPI_DATAWRITE);

    checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2;
    spi_write(PN532_PREAMBLE);
    spi_write(PN532_PREAMBLE);
    spi_write(PN532_STARTCODE2);

    spi_write(cmdlen);
    spi_write(~cmdlen + 1);

    spi_write(PN532_HOSTTOPN532);
    checksum += PN532_HOSTTOPN532;
    for (uint8_t i=0; i<cmdlen-1; i++) {
      spi_write(cmd[i]);
      checksum += cmd[i];
    }

    spi_write(~checksum);
    spi_write(PN532_POSTAMBLE);
    digitalWrite(_ss, HIGH);
  }
}
/************** low level SPI */

/**************************************************************************/
/*!
    @brief  Low-level SPI write wrapper

    @param  c       8-bit command to write to the SPI bus
*/
/**************************************************************************/
void Adafruit_PN532::spi_write(uint8_t c) {
    // Software SPI write.
    int8_t i;
    digitalWrite(_clk, HIGH);

    for (i=0; i<8; i++) {
      digitalWrite(_clk, LOW);
      if (c & _BV(i)) {
        digitalWrite(_mosi, HIGH);
      } else {
        digitalWrite(_mosi, LOW);
      }
      digitalWrite(_clk, HIGH);
    }
}
/**************************************************************************/
/*!
    @brief  Low-level SPI read wrapper

    @returns The 8-bit value that was read from the SPI bus
*/
/**************************************************************************/
uint8_t Adafruit_PN532::spi_read(void) {
  int8_t i, x;
  x = 0;
    // Software SPI read.
    digitalWrite(_clk, HIGH);

    for (i=0; i<8; i++) {
      if (digitalRead(_miso)) {
        x |= _BV(i);
      }
      digitalWrite(_clk, LOW);
      digitalWrite(_clk, HIGH);
    }
  return x;
}
