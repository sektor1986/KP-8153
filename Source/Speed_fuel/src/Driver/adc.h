#ifndef __ADC_H
#define __ADC_H

#define ADC_IGNITION         0
#define ADC_BAT              1
#define ADC_BACKLIGHT        4
#define ADC_FUEL             5


#define COUNT_ADC_CHANNEL    6

extern unsigned int adc_value[COUNT_ADC_CHANNEL];

void InitADC (void);
void ADC_IRQ(void);

#endif // __ADC_H
