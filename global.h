#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <stdio.h>
#include <stdbool.h>

#define MASTER   1
#define USER     2

#define SET     1
#define READ    2

#define EEPROM_START   11
#define SIZE           6

#define ADD           'T'
#define DELETE        'D'

#define BLOCK         1 // from 1 --> 10
#define TIME_RELAY_ON        8//8s

extern uint8_t uid[7];//Buffer to store the returned UID

#endif
