#include "mcu.h"
#include "utils.h"
#include "lcd.h"
                                    	// 0     1     2     3     4     5     6     7     8     9   10     11    12    13    14    15    16    17
										// 0     1     2     3     4     5     6     7     8     9   space  P 
										
unsigned char const LCD_SEG_1[18]  = 	{0xF9, 0x50, 0xE5, 0xF4, 0x5C, 0xBC, 0xBD, 0xD0, 0xFD, 0xFC, 0x00,  0xCD};
unsigned char const LCD_SEG_2[11]  = 	{0x9F, 0x90, 0xCB, 0xDA, 0xD4, 0x5E, 0x5F, 0x98, 0xDF, 0xDE, 0x00};
unsigned char const LCD_SEG_3_4[11]   = {0xF6, 0x50, 0xE3, 0xF1, 0x55, 0xB5, 0xB7, 0xD0, 0xF7, 0xF5, 0x00};
/*
unsigned char const LCD_SEG_10_5[12] = {0xAF, 0xA0, 0xCB, 0xE9, 0xE4, 0x6D, 0x6F, 0xA8, 0xEF, 0xED, 0x00, 0x0F};
unsigned char const LCD_SEG_6[11]    = {0x7B, 0x0A, 0x37, 0x1F, 0x4E, 0x5D, 0x7D, 0x0B, 0x7F, 0x5F, 0x00};
*/
									//	0     1     2     3     4     5     6     7     8     9     space C     П     А     Р     E     H
//unsigned char const LCD_SEG[17]  =     {0x7D, 0x60, 0x3E, 0x7A, 0x63, 0x5B, 0x5F, 0x70, 0x7F, 0x7B, 0x00, 0x1D, 0x75, 0x77, 0x37, 0x1F, 0x67};									

void InitLCD(void)
{
  //LCD Control register LCR
  LCR_CSS = 0;  // 0= perclk1  1= clock selected by lecr/cksel 
  LCR_LCEN = 0; // 0= disable display in the timer mode    1 = Enable  
  LCR_VSEL = 1; // activate internal resistors
  LCR_BK = 0;   // 0 = enable display | 1 = Blank display
  
  LCR_MS0 = 1;  // 1/4 duty cycle  N=4
  LCR_MS1 = 1;
  
  LCR_FP0 = 1;  // CLKP1/(2^15 x N) @ pll 16MHz --> CLKP1 @ 16MHz
  LCR_FP1 = 0;

  //LCD common pin switching register
  LCDCMR_COMEN0 = 1; //Common pin x enable
  LCDCMR_COMEN1 = 1;
  LCDCMR_COMEN2 = 1;
  LCDCMR_COMEN3 = 1;
  LCDCMR_DTCH = 0;  // Bias control //  0=1/3 Bias //   1=Reserved

  //LCD extended control register LECR
  LECR_CKSEL = 1 ; //0 = sub clock CLKSC  1 = RC-Clock CLKRC
 
	LCDER0_SEG4 = 1;
	LCDER0_SEG6 = 1;
	LCDER1_SEG8 = 1;
	LCDER1_SEG9 = 1;
	LCDER1_SEG11 = 1;
	LCDER2_SEG21 = 1;
	LCDER4_SEG36 = 1;
	LCDER4_SEG37 = 1;
  
//  LCDER5 = 0x0F;
  //LCDER6 = 0xFF; //SEG48 - SEG63
  //LCDER7 = 0xFF;
  //LCDER8 = 0xFF; //SEG64 - SEG71

  //Voltage line enable register LCDVER
  LCDVER_V0 = 0;  //1 = External divide resistors 0 = Internal divide resistors
  LCDVER_V1 = 0;
  LCDVER_V2 = 0;
  LCDVER_V3 = 0;  // set V3 to 1 for external dimming
}

void segment1(unsigned char NB)
{
	VRAM18_DL0 = LCD_SEG_1[NB] & 0x01;
	VRAM18_DL2 = (LCD_SEG_1[NB]>>2) & 0x01;
	VRAM18_DL3 = (LCD_SEG_1[NB]>>3) & 0x01;
	VRAM18_DH = (LCD_SEG_1[NB]>>4) & 0x0F;
}

void segment2(unsigned char NB)
{
	VRAM10_DH0 = (LCD_SEG_2[NB]>>4) & 0x01;
	VRAM10_DH2 = (LCD_SEG_2[NB]>>6) & 0x01;
	VRAM10_DH3 = (LCD_SEG_2[NB]>>7) & 0x01;
	VRAM5_DH = LCD_SEG_2[NB] & 0x0F;
}

void segment3(unsigned char NB)
{
	VRAM4_DL0 = LCD_SEG_3_4[NB] & 0x01;
	VRAM4_DL1 = (LCD_SEG_3_4[NB]>>1) & 0x01;
	VRAM4_DL2 = (LCD_SEG_3_4[NB]>>2) & 0x01;
	VRAM4_DH = (LCD_SEG_3_4[NB]>>4) & 0x0F;
}

void segment4(unsigned char NB)
{
	VRAM3_DL0 = LCD_SEG_3_4[NB] & 0x01;
	VRAM3_DL1 = (LCD_SEG_3_4[NB]>>1) & 0x01;
	VRAM3_DL2 = (LCD_SEG_3_4[NB]>>2) & 0x01;
	VRAM2_DL = (LCD_SEG_3_4[NB]>>4) & 0x0F;
}

void NumToTopStr(unsigned long num)
{
	unsigned long num_str = 0;
	
	num_str = ToBCD(num);	
			
	segment4(num_str & 0xF);       // Вывод едениц     
	if (num > 9)                    // Вывод десяток
		segment3((num_str>>4)&0xF);
	else
		segment3(SPACE);
	
	if (num > 99)                   // Вывод сотен
		segment2((num_str>>8)&0xF);
	else
		segment2(SPACE);
		
	if (num > 999)                  // Вывод тысяч
		segment1((num_str>>12)&0xF);
	else
		segment1(SPACE);	
		
} 

void NumToTopStr3(unsigned long num)
{
	unsigned long num_str = 0;
	
	num_str = ToBCD(num);	
			
	segment3(num_str & 0xF);       // Вывод едениц     
    segment2((num_str>>4)&0xF);    // Вывод десяток
		
	if (num > 99)                   // Вывод сотен
		segment1((num_str>>8)&0xF);
	else
		segment1(SPACE);
	segment4(SPACE);
}

void NumToTopStr2(unsigned long num)
{
	unsigned long num_str = 0;
	
	if (num > 99)
		num = 99;
	num_str = ToBCD(num);
	segment4(SPACE);
	segment3(num_str & 0xF);       // Вывод едениц             
	segment2((num_str>>4)&0xF);     // Вывод десяток

}

void ClearTopLine(void)
{
	segment1(SPACE);
	segment2(SPACE);
	segment3(SPACE);
	segment4(SPACE);
}

void Enable_simbols(void)
{
	ENABLE_POINT = 1;
	SIMBOL_TIME = 1;
	SIMBOL_V = 1;
	SIMBOL_TEMP = 1;
	SIMBOL_MINUS = 1;	
}

void Disable_simbols(void)
{
	ENABLE_POINT = 0;
	SIMBOL_TIME = 0;
	SIMBOL_V = 0;
	SIMBOL_TEMP = 0;
	SIMBOL_MINUS = 0;
}
