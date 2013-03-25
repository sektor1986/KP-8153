#include "mcu.h"
#include "utils.h"
#include "lcd.h"
                                    //  0     1     2     3     4     5     6     7     8     9     10    11    12    13    14    15    16    17
									//	0     1     2     3     4     5     6     7     8     9     space C     П     А     Р     E     H     h
/*
unsigned char const LCD_SEG_1_3[18]  = {0xAF, 0x06, 0xCB, 0x4F, 0x66, 0x6D, 0xED, 0x07, 0xEF, 0x6F, 0x00, 0xA9, 0xA7, 0xE7, 0xE3, 0xE9, 0xE6, 0xE4};
unsigned char const LCD_SEG_7_9[11]  = {0xFA, 0x60, 0xD6, 0xF4, 0x6C, 0xBC, 0xBE, 0xE0, 0xFE, 0xFC, 0x00};
unsigned char const LCD_SEG_4[11]    = {0x5F, 0x06, 0x6B, 0x2F, 0x36, 0x3D, 0x7D, 0x07, 0x7F, 0x3F, 0x00};
unsigned char const LCD_SEG_10_5[12] = {0xAF, 0xA0, 0xCB, 0xE9, 0xE4, 0x6D, 0x6F, 0xA8, 0xEF, 0xED, 0x00, 0x0F};
unsigned char const LCD_SEG_6[11]    = {0x7B, 0x0A, 0x37, 0x1F, 0x4E, 0x5D, 0x7D, 0x0B, 0x7F, 0x5F, 0x00};
*/
									//	0     1     2     3     4     5     6     7     8     9     space C     П     А     Р     E     H
unsigned char const LCD_SEG[17]  =     {0x7D, 0x60, 0x3E, 0x7A, 0x63, 0x5B, 0x5F, 0x70, 0x7F, 0x7B, 0x00, 0x1D, 0x75, 0x77, 0x37, 0x1F, 0x67};									

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
/* 
  LCDER5_SEG47 = 1;
  LCDER0_SEG3 = 1;
  LCDER0_SEG4 = 1; 
*/  
	LCDER0_SEG3 = 1;
	LCDER0_SEG4 = 1;
	LCDER0_SEG5 = 1;
	LCDER0_SEG6 = 1;
	LCDER1_SEG8 = 1;
	LCDER1_SEG9 = 1;
	LCDER1_SEG10 = 1;
	LCDER1_SEG11 = 1;
	LCDER2_SEG19 = 1;
	LCDER2_SEG20 = 1;
	LCDER2_SEG21 = 1;
	LCDER2_SEG23 = 1;
	LCDER3_SEG30 = 1;
	LCDER4_SEG36 = 1;
	LCDER4_SEG37 = 1;
	LCDER4_SEG38 = 1;
	LCDER4_SEG39 = 1;
	LCDER5_SEG42 = 1;
	LCDER5_SEG45 = 1;
	LCDER5_SEG47 = 1;
	LCDER6_SEG55 = 1;
	LCDER7_SEG56 = 1;
  
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
	VRAM18_DH0 = LCD_SEG[NB]>>4;
	VRAM18_DH1 = LCD_SEG[NB]>>5;
	VRAM18_DH2 = LCD_SEG[NB]>>6;
	VRAM18_DL = LCD_SEG[NB];
}

void segment2(unsigned char NB)
{
	VRAM10_DH0 = LCD_SEG[NB]>>4;
	VRAM10_DH1 = LCD_SEG[NB]>>5;
	VRAM10_DH2 = LCD_SEG[NB]>>6;
	VRAM5_DH = LCD_SEG[NB];
}

void segment3(unsigned char NB)
{
	VRAM4_DH0 = LCD_SEG[NB]>>4;
	VRAM4_DH1 = LCD_SEG[NB]>>5;
	VRAM4_DH2 = LCD_SEG[NB]>>6;
	VRAM4_DL = LCD_SEG[NB];
}

void segment4(unsigned char NB)
{
	VRAM2_DH0 = LCD_SEG[NB]>>4;
	VRAM2_DH1 = LCD_SEG[NB]>>5;
	VRAM2_DH2 = LCD_SEG[NB]>>6;
	VRAM3_DL = LCD_SEG[NB];
}

void segment5(unsigned char NB)
{
	VRAM19_DL0 = LCD_SEG[NB]>>4;
	VRAM19_DL1 = LCD_SEG[NB]>>5;
	VRAM19_DL2 = LCD_SEG[NB]>>6;
	VRAM19_DH = LCD_SEG[NB];
}

void segment6(unsigned char NB)
{

	VRAM1_DH0 = LCD_SEG[NB]>>4;
	VRAM1_DH1 = LCD_SEG[NB]>>5;
	VRAM1_DH2 = LCD_SEG[NB]>>6;
	VRAM2_DL = LCD_SEG[NB];

}

void segment7(unsigned char NB)
{

	VRAM22_DH0 = LCD_SEG[NB]>>4;
	VRAM22_DH1 = LCD_SEG[NB]>>5;
	VRAM22_DH2 = LCD_SEG[NB]>>6;
	VRAM23_DH = LCD_SEG[NB];

}

void segment8(unsigned char NB)
{
	VRAM28_DL0 = LCD_SEG[NB]>>4;
	VRAM28_DL1 = LCD_SEG[NB]>>5;
	VRAM28_DL2 = LCD_SEG[NB]>>6;
	VRAM27_DH = LCD_SEG[NB];
}

void segment9(unsigned char NB)
{
	VRAM15_DL0 = LCD_SEG[NB]>>4;
	VRAM15_DL1 = LCD_SEG[NB]>>5;
	VRAM15_DL2 = LCD_SEG[NB]>>6;
	VRAM10_DL = LCD_SEG[NB];
}

void segment10(unsigned char NB)
{
	VRAM9_DH0 = LCD_SEG[NB]>>4;
	VRAM9_DH1 = LCD_SEG[NB]>>5;
	VRAM9_DH2 = LCD_SEG[NB]>>6;
	VRAM21_DL = LCD_SEG[NB];
}
void NumToTopStr(unsigned long num)
{
	unsigned long num_str = 0;
	
	num_str = ToBCD(num);	
			
	segment5(num_str & 0xF);       // Вывод едениц     
	if (num > 9)                    // Вывод десяток
		segment6((num_str>>4)&0xF);
	else
		segment6(10);
	
	if (num > 99)                   // Вывод сотен
		segment7((num_str>>8)&0xF);
	else
		segment7(10);
		
	if (num > 999)                  // Вывод тысяч
		segment8((num_str>>12)&0xF);
	else
		segment8(10);	
		
	if (num > 9999)                 // Вывод десяти тысяч 
		segment9((num_str>>16)&0xF);
	else
		segment9(10);			
				
	if (num > 99999)                // Вывод ста тысяч
		segment10((num_str>>20)&0xF);
	else
		segment10(10);
} 

void NumToBottomStr(unsigned long num)
{
	unsigned long num_str = 0;
		
	num_str = ToBCD(num);	

	segment1(num_str & 0xF);        // Выыод десятых долей
	if (num > 9)                    // Вывод едениц 
		segment2((num_str>>4)&0xF);
	else
	{
		if (ENABLE_POINT == 1)
			segment2(0);
		else
			segment2(10);
	}
	
	if (num > 99)                   // Вывод десяток
		segment3((num_str>>8)&0xF);
	else
		segment3(10);
		
	if (num > 999)                  // Вывод сотен
		segment4((num_str>>12)&0xF);
	else
		segment4(10);	
}

void ClearTopLine(void)
{
	segment5(SPACE);
	segment6(SPACE);
	segment7(SPACE);
	segment8(SPACE);
	segment9(SPACE);	
	segment10(SPACE);
}

void ClearBottomLine(void)
{
	segment1(SPACE);
	segment2(SPACE);
	segment3(SPACE);
	segment4(SPACE);
}

void Disable_simbols(void)
{
	ENABLE_POINT = 0;
	DISABLE_KM_H;
	CLOCK = 0;
	SIMBOL_TIME = 0;
	SIMBOL_EDITTIME = 0;
}
