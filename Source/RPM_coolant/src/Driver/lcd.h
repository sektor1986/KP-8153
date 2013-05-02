#ifndef __LCD_H
#define __LCD_H

#define ENABLE_POINT	VRAM10_DH1
#define SIMBOL_TIME     VRAM4_DL3
#define SIMBOL_V		VRAM3_DL3
#define SIMBOL_TEMP		VRAM18_DL1
#define SIMBOL_MINUS	VRAM10_DH2

#define SPACE          10

void InitLCD (void);
void NumToTopStr(unsigned long num);
void NumToTopStr2(unsigned long num);
void NumToTopStr3(unsigned long num);


void segment1(unsigned char NB);
void segment2(unsigned char NB);
void segment3(unsigned char NB);
void segment4(unsigned char NB);


void ClearTopLine(void);
void Disable_simbols(void);
void Enable_simbols(void);

#endif // __LCD_H
