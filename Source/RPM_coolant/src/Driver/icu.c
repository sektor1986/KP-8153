#include "mcu.h"
#include "icu.h"

unsigned long max_value;

volatile unsigned int ICU1_old;
volatile unsigned int ICU1_new;
volatile unsigned int FRT0_ovl_cnt;
volatile unsigned int FRT0_ovl_cnt_new;
volatile unsigned int FRT0_ovl_cnt_old;
volatile unsigned char flag_value_valid;
volatile unsigned long value = 0xFFFFFFFF;
volatile unsigned long valueTemper;

unsigned char flag_freq = 0;
unsigned char flag_temp = 0;

void InitFRTimer0(void)
{  
	TCCS0  |=0x49;		//div  2, interrupt enabled
}

void InitICU1(void)
{
	InitFRTimer0();
	PIER06_IE5 = 1;   // Enable ICU port input Pin 1 
	ICS01_EG1  = 1;   // Define start edge: 1:Rising  2:Falling  3:Both Edges
	ICS01_ICE1 = 1;   // Enable IRQ
}

void InitEI0(void)
{
	// Температура ОЖ INT0 P03_6
	PIER03_IE6 = 1;  
	ELVR0_LALB00 = 0;	// Rising edge
	ELVR0_LALB01 = 1;	
	EIRR0_ER0 = 0;  // reset interrupt request 
	ENIR0_EN0 = 1;  // enable interrupt request 
}
//------------------------------------------------------------------------------ 
// FRT0 (IRQ) 
//------------------------------------------------------------------------------ 
__interrupt void FRT0_IRQ (void)
{
	FRT0_ovl_cnt++;                      // Count the overflows 
	TCCS0_IVF = 0;
}

//------------------------------------------------------------------------------ 
// ICU1 (IRQ) 
//------------------------------------------------------------------------------ 
__interrupt void ICU1_IRQ (void)
{
	static unsigned int counter_imp = 0;
	static unsigned int counter_imp_s = 0;
	static unsigned int PassCounter = 0;
	
	if (PassCounter == 0)
	{
		ICU1_new = IPCP1;                     // Save current ICU value
		FRT0_ovl_cnt_new = FRT0_ovl_cnt;      // Save current FRT value
		flag_freq = 1;
	
		if (TCCS0_IVF && (IPCP1 < 0x8000))   // Check for FRT/ICU race condition 
		{                                     // In case that FRT was not yet handled 
			FRT0_ovl_cnt_new++;                 // then increase temp FRT counter
		}

		// Calculation of time period
		if (ICS01_EG1 == 1)                   // Define the ending edge: 1:Rising  2:Falling  3:Both Edges
		{
			value = abs(FRT0_ovl_cnt_new - FRT0_ovl_cnt_old) * 0x10000UL + ICU1_new - ICU1_old;
			flag_value_valid = 1;               // Flag to be used by main application.
		}
 
		ICU1_old = ICU1_new;                  // Save current ICU value as reference for next cycle
		FRT0_ovl_cnt_old = FRT0_ovl_cnt_new;  // Save current FRT value as reference for next cycle
		PassCounter = 1;
	}
	else
		PassCounter--;
	ICS01_ICP1 = 0; // Clear ICU Irq-flag
}

//Прерывание по входу от датчика температуры
__interrupt void EI0_IRQ (void)
{	
	static unsigned long EI0_old = 0;
	static unsigned long EI0_new = 0;
	static unsigned long FRT00_ovl_cnt_old = 0;
	static unsigned long FRT00_ovl_cnt_new = 0;
	
	
	if (EIRR0_ER0)
	{
		EIRR0_ER0 = 0;		// Сброс флага прерывания
		
		if (ELVR0_LALB00 == 0)
		{
			EI0_old = TCDT0;
			FRT00_ovl_cnt_old = FRT0_ovl_cnt;
			ELVR0_LALB00 = 1;
		}
		else
		{
			EI0_new = TCDT0;                     // Save current ICU value
		
			FRT00_ovl_cnt_new = FRT0_ovl_cnt;      // Save current FRT value

			if (TCCS0_IVF && (TCDT0 < 0x8000))   // Check for FRT/ICU race condition 
			{                                     // In case that FRT was not yet handled 
				FRT00_ovl_cnt_new++;               // then increase temp FRT counter
			}
			flag_temp = 1;	
			// Calculation of time period
			valueTemper = abs(FRT00_ovl_cnt_new - FRT00_ovl_cnt_old) * 0x10000UL + EI0_new - EI0_old;
			ELVR0_LALB00 = 0;
		}
	}
}
