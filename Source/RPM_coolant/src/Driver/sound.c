#include "mcu.h"
#include "sound.h"

void InitSoundGen(void)		
{
  DDR00_D7 = 1;     // make Portpin as output (SGO)
  //SGCR0 = 0x1D4;     // auto-decrement, 1/8clock, mixed,                                
  SG0_SGCR0 = 0x0014;
  SG0_SGFR0 = 0x0C;		// Frequency 
  SG0_SGAR0 = 0xA0;		// start amplitude 
  SG0_SGDR0 = 0x01;      // Decrement Grade register 
  SG0_SGTR0 = 0x57;      // Tone count register 
}

void Beep(void)
{
	if (SG0_SGCR0_BUSY) return;
		SG0_SGCR0_ST = 1;	
}

__interrupt void SoundGenInt(void)
{    
	SG0_SGCR0_ST = 0;
	SG0_SGCR0_INT = 0;            // clear irq - Flag
}
