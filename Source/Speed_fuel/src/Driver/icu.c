#include "mcu.h"
#include "icu.h"

unsigned long max_value;

volatile unsigned int ICU1_old;
volatile unsigned int ICU1_new;
volatile unsigned int FRT0_ovl_cnt;
volatile unsigned int FRT0_ovl_cnt_new;
volatile unsigned int FRT0_ovl_cnt_old;
volatile unsigned char flag_value_valid;
volatile unsigned long value;

unsigned char flag_freq = 0;
float frequency;

volatile unsigned long value_temp = 0;

unsigned long km = 0;
unsigned long km_sut = 0;

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
		flag_value_valid = 1;               // Flag to be used by main application. UART output here might be too long
	}
 
	ICU1_old = ICU1_new;                  // Save current ICU value as reference for next cycle
	FRT0_ovl_cnt_old = FRT0_ovl_cnt_new;  // Save current FRT value as reference for next cycle

	ICS01_ICP1 = 0; // Clear ICU Irq-flag

	if (value > max_value) 
	{
		counter_imp++;
		if (counter_imp == 10)
		{
			km++;
			km_sut++;
			counter_imp = 0;
		}        
	}	
}

