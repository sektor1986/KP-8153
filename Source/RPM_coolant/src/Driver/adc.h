#ifndef __ADC_H
#define __ADC_H

#define ADC_IGNITION         0
#define ADC_OIL_PRESS_10     1
#define ADC_OIL_PRESS_6      4
#define ADC_COOLANT_TEMP     5


#define COUNT_ADC_CHANNEL    6

extern unsigned int adc_value[COUNT_ADC_CHANNEL];

void InitADC (void);
void ADC_IRQ(void);

#endif // __ADC_H