/**************************************************************************
PHAN VAN HOANG
1/1/2020
file for Eeprom24Cxx 
**************************************************************************/
#include <Wire.h>
#include <Arduino.h>
#include <EEPROM.h>

#define eeprom_on_arduino 0
// chọn eeprom của vi điều khiển
#define eeprom_on_ic 1
// chọn eeprom của ic ngoại vi
class Eeprom24C 
{
    public:
      Eeprom24C( unsigned int size_m, byte deviceAddress);
      Eeprom24C();
      
      void write_1_byte( unsigned int address, byte data);
      byte read_1_byte(  unsigned int address);
      uint16_t read_2_byte(  unsigned int address);
      void write_2_byte( unsigned int address, uint16_t data);
      void  write_4_byte( unsigned int address, uint32_t data);
      uint32_t read_4_byte(  unsigned int address);
      void write_8_byte( unsigned int address, uint64_t data);
      uint64_t read_8_byte(  unsigned int address);

    private:
      void write_byte_eeprom_ic(unsigned int address,byte  data);
      byte read_byte_eeprom_ic( unsigned int address);
      void write_byte_eeprom_arduino(unsigned int address,byte  data);
      byte read_byte_eeprom_arduino( unsigned int address);
      byte select_eeprom;
      unsigned int size_memory;
      unsigned long size_bytes;
      byte m_deviceAddress;

};

static Eeprom24C AVR_EEPROM;
