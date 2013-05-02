#include "mcu.h"
#include "backlight.h"
#include "adc.h"

unsigned long Backlight = 0; //0..250

#define STATE_BACKLIGHT PDR03_P5 //Pin 62 MB96670

void InitBacklight(void)
{
	DDR03_D5 = 1;
	STATE_BACKLIGHT = 0;
	TMR1  = 10;                /* set reload value in [us] 2*x    */ 
	TMCSR1  = 0x81B;            /* prescaler 2us at 32MHz         */ 
}

void UpdateBacklight(void)
{
	static unsigned int count = 0;
	
	Backlight &= 0xFF;
	if (count > Backlight)
		STATE_BACKLIGHT = 0;
	
	if (count == 255)
	{
		count = 0;
		if (Backlight != 0)
			STATE_BACKLIGHT = 1;
	}
	count++;
	
}

void SetValueBacklight(void)
{
	unsigned long new_backlight = 0;
	
	if ((adc_value[ADC_IGNITION] > 320) && (adc_value[ADC_IGNITION] < 745))
	{
		if (adc_value[ADC_BACKLIGHT] > 500)
			new_backlight = 125;
		else 
			new_backlight = 200;
//			if (adc_value[ADC_BACKLIGHT] < 60)       // от 04.06.2012
//				new_backlight = 200;
//			else 
//				new_backlight = (adc_value[ADC_BACKLIGHT]) >> 2;		
	}	
	else
		new_backlight = 0;
	
	if (new_backlight == 0)
		Backlight = 0;
	else
	{
		if (new_backlight > Backlight)
			Backlight++;
		else if (new_backlight < Backlight)
			Backlight--;
	}
}

__interrupt void Backlight_IRQ(void)
{   
	UpdateBacklight();			//Подсветка
	TMCSR1_UF = 0;    /* reset underflow interrupt request flag    */ 
}

