/**
 *  @filename   :   function_nfc.cpp
 *  @author     :  Mr. HOANG
 *
*/

#include <stdlib.h>
//#include <SPI.h>
#include "Eeprom24Cxx.h"
#include "function_nfc.h"


static Eeprom24C eeprom(64,0x50);
extern unsigned char old_uid[7];//Save UID
extern unsigned char g_ucBuffRead[16];
extern uint8_t g_bUpdateData;
extern uint8_t ModeWork;
extern void beep(uint16_t time_delay_ms);
//----------------------------------------------------------------------------
//Ham cap nhat thong tin va du lieu trong the tag
  void FUN_NFC::updateDatafromCard (){
       if(g_bUpdateData){
       //Xac dinh xem the nay la thẻ master hay thẻ user
          if(strstr((char *)g_ucBuffRead, "MASTER") != NULL){
            g_ucModeCard = MASTER;count++;
            if(count == 1){
              if(g_ucBuffRead[6]=='T'){beep(100);delay(300);beep(1000);}
              else if(g_ucBuffRead[6]=='D'){beep(100);delay(300);beep(100);delay(300);beep(1000);}
              else {;}
            }
            else {beep(100);delay(100);beep(100);delay(100);beep(100);}
          }
          if((strstr((char *)g_ucBuffRead, "SLAVE") != NULL)&&(g_ucBuffRead[5]==BLOCK)){g_ucModeCard = USER; have_new_user = 1;}
       //Khi doc thay the master thi luu id/mode/mark/modeD/SttD/numdel-------------------
        if(count==1){
          if(g_ucModeCard == MASTER){
            ModeWork = SET;
           //chuc nang cua the marster
            cardNFCMaster.mode  = g_ucBuffRead[6];
           //Kieu xoa cua the master
            cardNFCMaster.modeD  = g_ucBuffRead[11];
           //So thu tu bat dau xoa
            cardNFCMaster.sttD.ui8[0]  = g_ucBuffRead[12];
            cardNFCMaster.sttD.ui8[1]  = g_ucBuffRead[13];
            AddrEepromDel = (cardNFCMaster.sttD.ui16-1)*SIZE+EEPROM_START;
           //So the muon xoa
            cardNFCMaster.numdel  = g_ucBuffRead[14];
          }
        }
        else {ModeWork = READ; count=0;have_new_user = 0;}
       //Khi doc thay the USER thi luu id/block/stt1/stt2/Data1/../Data4-------------------
           if(g_ucModeCard == USER){
             //ID
              cardNFCUser.id.ui8[0]  = old_uid[0];
              cardNFCUser.id.ui8[1]  = old_uid[1];
              cardNFCUser.id.ui8[2]  = old_uid[2];
              cardNFCUser.id.ui8[3]  = old_uid[3];           
             //BLOCK
              cardNFCUser.block = g_ucBuffRead[5];
             //STT
              cardNFCUser.stt.ui8[0]  = g_ucBuffRead[6];
              cardNFCUser.stt.ui8[1]  = g_ucBuffRead[7];
             //DATA
              cardNFCUser.data[0]  =  g_ucBuffRead[8];
              cardNFCUser.data[1]  =  g_ucBuffRead[9];
              cardNFCUser.data[2]  =  g_ucBuffRead[10];
              cardNFCUser.data[3]  =  g_ucBuffRead[11];
             //DIA CHI EEPROM
              AddrEeprom = (cardNFCUser.stt.ui16-1)*SIZE+EEPROM_START;
          }
       }
   }
//WRITE DATA IN CARD TO EEPROM
   bool FUN_NFC::writeDataEeprom (){                           
      //Save STT
      eeprom.write_1_byte(AddrEeprom,cardNFCUser.stt.ui8[0]); delay(5);
      eeprom.write_1_byte(AddrEeprom+1,cardNFCUser.stt.ui8[1]); delay(5);
      //Save ID
      eeprom.write_1_byte(AddrEeprom+2,cardNFCUser.id.ui8[0]); delay(5);
      eeprom.write_1_byte(AddrEeprom+3,cardNFCUser.id.ui8[1]); delay(5);
      eeprom.write_1_byte(AddrEeprom+4,cardNFCUser.id.ui8[2]); delay(5);
      eeprom.write_1_byte(AddrEeprom+5,cardNFCUser.id.ui8[3]); delay(5);
      //check data
      if(eeprom.read_1_byte(AddrEeprom)   != cardNFCUser.stt.ui8[0])  {return false;}
      if(eeprom.read_1_byte(AddrEeprom+1) != cardNFCUser.stt.ui8[1])  {return false;}
      if(eeprom.read_1_byte(AddrEeprom+2) != cardNFCUser.id.ui8[0])   {return false;}
      if(eeprom.read_1_byte(AddrEeprom+3) != cardNFCUser.id.ui8[1])   {return false;}
      if(eeprom.read_1_byte(AddrEeprom+4) != cardNFCUser.id.ui8[2])   {return false;}
      if(eeprom.read_1_byte(AddrEeprom+5) != cardNFCUser.id.ui8[3])   {return false;}
    return true;
   }
//DELETE EEPROM WITH ONLY "THE XOA"
bool FUN_NFC::DeleteDataEeprom (unsigned int Addr,unsigned int Num){
  uint16_t i;
    for(i=0;i<Num;i++){eeprom.write_1_byte(Addr+i,0x00);}               
    return true;
}
//READ DATA FROM EEPROM
   bool FUN_NFC::readDataEeprom ()
   {    
    //STT                       
     buff_STT_LOW =   eeprom.read_1_byte(AddrEeprom);
     buff_STT_HIGH =  eeprom.read_1_byte(AddrEeprom+1);
    //ID
     buff_Eeprom_UID[0] = eeprom.read_1_byte(AddrEeprom+2);
     buff_Eeprom_UID[1] = eeprom.read_1_byte(AddrEeprom+3);
     buff_Eeprom_UID[2] = eeprom.read_1_byte(AddrEeprom+4);
     buff_Eeprom_UID[3] = eeprom.read_1_byte(AddrEeprom+5);
    return true;
   }
//CHECK DATA FROM CARD AND COMPARE WITH  EEPROM
bool FUN_NFC::CompareData (){ 
     uint8_t count;                          
     if(cardNFCUser.id.ui8[0]==buff_Eeprom_UID[0]){count++;}else {return false;}
     if(cardNFCUser.id.ui8[1]==buff_Eeprom_UID[1]){count++;}else {return false;}
     if(cardNFCUser.id.ui8[2]==buff_Eeprom_UID[2]){count++;}else {return false;}
     if(cardNFCUser.id.ui8[3]==buff_Eeprom_UID[3]){count++;}else {return false;}
     if(cardNFCUser.stt.ui8[0]==buff_STT_LOW) {count++;}    else {return false;}
     if(cardNFCUser.stt.ui8[1]==buff_STT_HIGH){count++;}    else {return false;}
     if(count==6){return true;}
   }
