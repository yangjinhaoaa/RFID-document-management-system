#ifndef __DS3231_H__
#define __DS3231_H__


#include "main.h"

#define DS3231_ADDRESS 0xD0




typedef struct {
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hour;
	uint8_t dayofweek;
	uint8_t dayofmonth;
	uint8_t month;
	uint8_t year;
} TIME;

// Convert normal decimal numbers to binary coded decimal
uint8_t decToBcd(int val);
// Convert binary coded decimal to normal decimal numbers
int bcdToDec(uint8_t val);



// function to set time

void Set_Time (uint8_t sec, uint8_t min, uint8_t hour, uint8_t dow, uint8_t dom, uint8_t month, uint8_t year);

TIME Get_Time (void);

float Get_Temp (void);
void force_temp_conv (void);


#endif /* __DS3231_H__ */
