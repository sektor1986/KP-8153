#ifndef __LCD_H
#define __LCD_H

#define ENABLE_POINT	VRAM11_DH2
#define ENABLE_KM    	VRAM18_DH3
#define ENABLE_KM_H    	{VRAM18_DH3 = 1; VRAM19_DL3 = 1;}
#define DISABLE_KM_H    {VRAM18_DH3 = 0; VRAM19_DL3 = 0;}
#define CLOCK			VRAM5_DL3
#define SIMBOL_TIME     VRAM5_DL2
#define SIMBOL_EDITTIME	VRAM4_DH3

#define SPACE          10

void InitLCD (void);
void NumToTopStr(unsigned long num);
void NumToBottomStr(unsigned long num);

void segment1(unsigned char NB);
void segment2(unsigned char NB);
void segment3(unsigned char NB);
void segment4(unsigned char NB);
void segment5(unsigned char NB);
void segment6(unsigned char NB);
void segment7(unsigned char NB);
void segment8(unsigned char NB);
void segment9(unsigned char NB);
void segment10(unsigned char NB);

void ClearTopLine(void);
void ClearBottomLine(void);
void Disable_simbols(void);

#endif // __LCD_H
