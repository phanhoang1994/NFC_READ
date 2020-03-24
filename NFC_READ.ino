/**************************************************************************
PHAN VAN HOANG
1/1/2020
NFC READ THIEN NAM
**************************************************************************/
#include <SPI.h>
#include "Adafruit_PN532.h"
#include "Eeprom24Cxx.h"
#include "function_nfc.h"
//DEFINE-----------------------------------------------------------------------------------
#define PN532_SCK  (13)
#define PN532_MOSI (11)
#define PN532_SS   (10)
#define PN532_MISO (12)

#define MASTER   1
#define SLAVE    2
//KHAI BAO BIEN-----------------------------------------------------------------------------
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
FUN_NFC fun_nfc;
const int SCL_PIN = A5;  // The pin number of the clock pin.
const int SDO_PIN = A4;  // The pin number of the data pin.
int NOISE = 9;

unsigned char uid[7];//Buffer to store the returned UID
unsigned char old_uid[7];//Save UID
uint8_t uidLength;//Length of the UID (4 or 7 bytes depending on ISO14443A card type
uint8_t keya[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; //mark for read data in block
unsigned char ucUART1FrameTx[18];
uint8_t saveEeprom;//
uint8_t block;//
uint8_t g_bHaveNewCard,g_bUpdateData,ModeWork;
unsigned char g_ucBuffRead[16];
//nguyen mau ham
void beep(uint16_t time_delay_ms);
//SETUP--------------------------------------------------------------------------
void setup(void) {
  Serial.begin(9600,SERIAL_8E1); 
  //Serial.begin(9600); //use for debug
  pinMode(NOISE,OUTPUT);   
  digitalWrite(NOISE,LOW);  
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata){
    while (1);
  }
  nfc.SAMConfig();
  ucUART1FrameTx[0] = 0x7E; //ID
  beep(200);
}
//MAIN-----------------------------------------------------------------------------
void loop(void) {
  uint8_t i;
  uint8_t temp;
 //DETECT NFC----------------------------------------------------------------------
 if(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)==true){
  g_bHaveNewCard=0;
  for(i=0;i<uidLength;i++){if(uid[i] != old_uid[i]){g_bHaveNewCard=1;}}
    if(g_bHaveNewCard){
      for(i=0;i<uidLength;i++){old_uid[i] = uid[i];}
      if(nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keya)){
        g_bUpdateData = nfc.mifareclassic_ReadDataBlock (4, g_ucBuffRead);
        nfc.PrintHex(g_ucBuffRead,16);
        nfc.PrintHexChar(g_ucBuffRead,16);
        fun_nfc.updateDatafromCard();
      }
    }
 }
  else{for(i=0;i<7;i++){old_uid[i] = 0;};}
  delay(100);
  //-------------------------------------------------------------------------------
  switch (ModeWork){
    case SET:{      
      switch (fun_nfc.cardNFCMaster.mode){
        case ADD:{
          if(fun_nfc.have_new_user == 1){
            if(fun_nfc.writeDataEeprom()){beep(200);}
            else {;}
            fun_nfc.have_new_user = 0;
          }
          break;
        }
        case DELETE:{
          switch(fun_nfc.cardNFCMaster.modeD){
           case 1:{if(fun_nfc.DeleteDataEeprom(fun_nfc.AddrEepromDel,SIZE)){beep(500);fun_nfc.cardNFCMaster.modeD = 0; };break;}
           case 2:{if(fun_nfc.DeleteDataEeprom(fun_nfc.AddrEepromDel,(fun_nfc.cardNFCMaster.numdel)*SIZE)){beep(500);fun_nfc.cardNFCMaster.modeD = 0;};break;}
           case 3:{if(fun_nfc.DeleteDataEeprom(EEPROM_START,EEPROM_FINISH)){beep(500);fun_nfc.cardNFCMaster.modeD = 0;};break;}
           default:{break;} 
          }
          break;
        }
        default:{break;}
      }
      break;
    }
    case READ:{
      if(fun_nfc.g_ucModeCard == USER){
          fun_nfc.readDataEeprom();
          delay(10);
          if(fun_nfc.CompareData()){
            ucUART1FrameTx[4] = fun_nfc.cardNFCUser.data[0];
            ucUART1FrameTx[5] = fun_nfc.cardNFCUser.data[1];
            ucUART1FrameTx[6] = fun_nfc.cardNFCUser.data[2];
            ucUART1FrameTx[7] = fun_nfc.cardNFCUser.data[3];
            /*send--> sce*/
           temp = 0xFF;
           for(i=2;i<16;i++){temp ^= ucUART1FrameTx[i];}
           ucUART1FrameTx[16]=temp;
           temp = 0;
           for(i=2;i<17;i++){temp += ucUART1FrameTx[i];}
           ucUART1FrameTx[17]=temp;
           Serial.write(ucUART1FrameTx,18);
           beep(500);
           fun_nfc.g_ucModeCard = 0;
        }
      }
      break;
    }
    default:{break;}
  }
//-----------------------------------------------------------------------------
}
void beep(uint16_t time_delay_ms){digitalWrite(NOISE,HIGH);delay(time_delay_ms);digitalWrite(NOISE,LOW);}
