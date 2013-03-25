#ifndef __ICU_H
#define __ICU_H

#define K                6100  

extern volatile unsigned long value;
extern unsigned char flag_freq;
extern unsigned long max_value;
extern unsigned long km;
extern unsigned long km_sut;

void InitICU1(void);
__interrupt void ICU1_IRQ (void);
__interrupt void FRT0_IRQ (void);

#endif // __ICU_H