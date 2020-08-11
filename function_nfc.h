/**
 *  @filename   :   function_nfc.h
 *  @brief      :   Header file for function_nfc.cpp
 *  @author     :   Mr. Hoang
 *  

 */

#ifndef FUNCTION_NFC_H
#define FUNCTION_NFC_H

#include "global.h"

class FUN_NFC{
 public: 
  //TU DINH NGHIA
     typedef union{
      unsigned char  ui8[4];
      unsigned int ui16[2];
      unsigned long ui32;
    } DWORD;
    
    //dung de luu STT
    typedef union{
      unsigned char  ui8[2];
      unsigned int ui16;
    } WORD;
    
    typedef struct {
      DWORD         id;
      unsigned char block;
      WORD          stt;
      unsigned char data[4];
    } cardTagUser;
    
    typedef struct {
      unsigned char mode;
      unsigned char modeD;
      WORD         sttD;
      unsigned char numdel;
    } cardTagMaster;
  //bien public
   unsigned char buff_Eeprom_UID[4];//Save UID
   unsigned char buff_STT_HIGH;//Byte cao
   unsigned char buff_STT_LOW;//Byte thap
   unsigned int AddrEeprom; 
   unsigned int AddrEepromDel; 
   
   cardTagMaster cardNFCMaster;
   cardTagUser   cardNFCUser;    
   unsigned char g_ucModeCard;
   bool have_new_user;
   //Nguyen mau ham
	 void updateDatafromCard ();
   bool writeDataEeprom ();
   bool readDataEeprom ();
   bool CompareData ();
   bool DeleteDataEeprom (unsigned int Addr,unsigned int Num);
   uint8_t have_new_master;
 private: 
};

#endif

/* END OF FILE */
