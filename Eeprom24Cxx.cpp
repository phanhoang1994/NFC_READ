/**************************************************************************
PHAN VAN HOANG
1/1/2020
file for Eeprom24Cxx 
**************************************************************************/
#include <Arduino.h>
#include "Eeprom24Cxx.h"

Eeprom24C::Eeprom24C(  unsigned int size_m, byte deviceAddress){
    m_deviceAddress = deviceAddress;
    size_memory=size_m;
    size_bytes=size_m*128; //  size_bytes=(size_m*1024)/8; 
    // tìm số byte mà ic có thể lưu trữ
    // ví dụ ic 24c04 có (4*1024)/8=512 bytes
    //byte đầu tiên là 0. byte cuối là 511
     select_eeprom=eeprom_on_ic;// chọn phương thức lưu trên ic
     Wire.begin();
}
Eeprom24C::Eeprom24C(){
   
     select_eeprom=eeprom_on_arduino;// chọn phương thức lưu trên arduino
}

void Eeprom24C::write_byte_eeprom_ic(unsigned int   address,byte  data){


    if((address>=size_bytes)|(read_1_byte(address)==data)){
        return;
    }
    if( size_memory<=2){
        //24c01 và 24c02
        Wire.beginTransmission(m_deviceAddress);
    Wire.write((byte)address);//địa chỉ 8 bit 
    
    }else if(size_memory<=16){ 
    Wire.beginTransmission((byte)(m_deviceAddress | ((address >> 8) & 0x07)));
    Wire.write(address & 0xFF);
    } else if(size_memory<=256){
        //24c32, 24c64, 24c128, 24c256

    Wire.beginTransmission(m_deviceAddress);
    Wire.write(address >> 8);
    Wire.write(address & 0xFF);
    }

    Wire.write(data);
    Wire.endTransmission();
    delay(10);
}
byte Eeprom24C::read_byte_eeprom_ic( unsigned int address){

if(address>=size_bytes){
    return 0;// thoát ngay
}

 byte data = 0;
   if( size_memory<=2){
    Wire.beginTransmission(m_deviceAddress);
    Wire.write(address);
    Wire.endTransmission();
    Wire.requestFrom(m_deviceAddress, (byte)1);
   }else if(size_memory<=16){
    Wire.beginTransmission((byte)(m_deviceAddress | ((address >> 8) & 0x07)));
    Wire.write(address & 0xFF);
    Wire.endTransmission();
    Wire.requestFrom((byte)(m_deviceAddress | ((address >> 8) & 0x07)), (byte)1);
   }else if(size_memory<=256){
    Wire.beginTransmission(m_deviceAddress);
    Wire.write(address >> 8);
    Wire.write(address & 0xFF);
    Wire.endTransmission();
    Wire.requestFrom(m_deviceAddress, (byte)1);
  }
   if (Wire.available()){ 
        data = Wire.read();
        delay(5);
        return data;
    }
}

void Eeprom24C::write_byte_eeprom_arduino(unsigned int   address,byte  data){  
  if(read_1_byte(address)==data){return;}
  EEPROM.write( address,data);
  delay(5);
}
byte Eeprom24C::read_byte_eeprom_arduino( unsigned int address){
  byte data = EEPROM.read( address);
  delay(5);
  return data;
}
void Eeprom24C::write_1_byte(unsigned int   address,byte  data){
  if(select_eeprom==eeprom_on_arduino){
      write_byte_eeprom_arduino(address, data);
  }
  else if( select_eeprom==eeprom_on_ic){
      write_byte_eeprom_ic(address, data);
  }
}
byte Eeprom24C::read_1_byte(   unsigned int address){
  if(select_eeprom==eeprom_on_arduino){return  read_byte_eeprom_arduino(address);}
  else if( select_eeprom==eeprom_on_ic){return read_byte_eeprom_ic(address);}
}
void Eeprom24C::write_2_byte( unsigned int address, uint16_t data){
    byte cao_8=((data>>8) & 0xFF);
    write_1_byte(address, cao_8);  
    byte thap_8=(data & 0xFF);   
    write_1_byte(address+1, thap_8); 
}
uint16_t Eeprom24C::read_2_byte(  unsigned int address){

    uint16_t doc_cao_8, doc_thap_8;
    doc_cao_8=read_1_byte(address);
    doc_thap_8=read_1_byte(address+1);
    uint16_t doc_data;
    doc_data=(doc_cao_8<<8);
    return (doc_data|doc_thap_8);
}
void Eeprom24C::write_4_byte( unsigned int address, uint32_t data){
    for( byte i=0; i<4; i++){
        write_1_byte(address+3-i, ((data>>(8*i) )& 0xFF));  
    }
}
uint32_t Eeprom24C::read_4_byte(  unsigned int address){
   uint32_t tam_bit_thu[4];   
   uint32_t doc_bit_thu[4];
    for( byte i=0; i<4; i++){tam_bit_thu[(3-i)]=read_1_byte( address+i);}
    for(byte i=0; i<4; i++){doc_bit_thu[i]=(tam_bit_thu[i]<<(i*8));}
    uint32_t doc_data;
    for( byte i=0; i<4; i++){doc_data=doc_data|doc_bit_thu[i];}
    return doc_data;
}

void Eeprom24C::write_8_byte( unsigned int address, uint64_t data){
    for( byte i=0; i<8; i++){write_1_byte(address+7-i, ((data>>(8*i) )& 0xFF));  }
}
uint64_t Eeprom24C::read_8_byte(  unsigned int address){ 
  uint64_t tam_bit_thu[8];   
  uint64_t doc_bit_thu[8];
  for( byte i=0; i<8; i++){tam_bit_thu[(7-i)]=read_1_byte( address+i);} 
for(byte i=0; i<8; i++){doc_bit_thu[i]=(tam_bit_thu[i]<<(i*8));}


uint64_t doc_data;
for( byte i=0; i<8; i++){doc_data=doc_data|doc_bit_thu[i];}
  return doc_data;
}
