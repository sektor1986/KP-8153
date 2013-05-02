#ifndef __ICU_H
#define __ICU_H

extern volatile unsigned long value;
extern volatile unsigned long valueTemper;
extern unsigned char flag_freq;
extern unsigned char flag_temp;

void InitFRTimer0(void);
void InitICU1(void);
void InitEI0(void);
__interrupt void EI0_IRQ (void);
__interrupt void ICU1_IRQ (void);
__interrupt void FRT0_IRQ (void);

#endif // __ICU_H