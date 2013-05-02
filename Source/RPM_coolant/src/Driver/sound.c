#include "mcu.h"
#include "sound.h"

/*	0 activ;
	1 prioryti;
	2 number_cycles;
	3 number_takt;
	4 takt_time_1;
	5 takt_time_2; 
	6 takt_time_3; 
	7 takt_time_4; 
	8 curent_cycles;
	9 curent_takt;
	10 frequency
	11 worked_out
*/

//Длительность такта равна takt_time*250ms
stc_sound_t sound[SOUND_MAX] = {//	0	1	2	3	4	5	6	7	8	9	10,		11
									{0,	0,	5,	2,	2,	2,	0,	0,	1,	1,	_1kHz,	0},
									{0,	1,	1,	1,	20,	0,	0,	0,	1,	1,	_4kHz,	0},
									{0,	2,	1,	1,	20,	0,	0,	0,	1,	1,	_4kHz,	0},
									{0,	3,	4,	4,	2,	1,	1,	1,	1,	1,	_1kHz,	0},
									{0,	4,	5,	2,	2,	2,	0,	0,	1,	1,	_4kHz,	0},
									{0,	5,	1,	4,	2,	1,	1,	1,	1,	1,	_4kHz,	0},
									{0,	6,	4,	4,	2,	1,	1,	1,	1,	1,	_4kHz,	0},
									{0,	7,	6,	4,	2,	1,	1,	1,	1,	1,	_4kHz,	0},
								};

void InitSoundGen(void)		
{
  DDR00_D7 = 1;     // make Portpin as output (SGO)
  //SGCR0 = 0x1D4;     // auto-decrement, 1/8clock, mixed,                                
  SG0_SGCR0 = 0x0014;
  //one PWM cycles = 1000000/16000000*384 = 24mks
  
//  SG0_SGFR0 = 0x0C;		// Frequency
  SG0_SGFR0 = 0x04;			// Frequency  1000000 / (24 * (4+1))/2 = 4167
//  SG0_SGAR0 = 0xA0;			// start amplitude 
  SG0_SGAR0 = 0xBF;			// start amplitude 
  SG0_SGDR0 = 0x09;			// Decrement Grade register 
//  SG0_SGTR0 = 0x57;			// Tone count register 
  SG0_SGTR0 = 0x63;			// Tone count register 
}

void Beep(void)
{
	if (SG0_SGCR0_BUSY) return;
		SG0_SGCR0_ST = 1;	
}

void ActivateSound(uint8_t num)
{
	if (sound[num].worked_out == 0)
	{
		sound[num].activ = 1;
		sound[num].worked_out = 1;
	}
}

void DeactivateSound(uint8_t num)
{
	sound[num].activ = 0;
	sound[num].worked_out = 0;
	sound[num].curent_cycles = 1;
	sound[num].curent_takt = 1;
}

void DeactivateAllSound(void)
{
	uint8_t i = 0;
	
	for (i=0; i<SOUND_MAX; i++)
		DeactivateSound(i);
	SG0_SGTR0 = 0x00;			// Tone count register 
	SG0_SGDR0 = 0x00;			// Decrement Grade register 
}

void UpdateSound(void)
{
	uint8_t i = 0;
	static uint8_t delay_time = 0;
	static uint8_t curent_sound = 0;
	static uint8_t count_cycles_one_takt = 0;
	
	//Пауза 1,5 с между сигналами
	if (delay_time)
	{
		delay_time--;
		return;
	}
	
	// Поиск следующего звукового сигнала
	for (i=curent_sound; i<SOUND_MAX; i++)
	{
		if (sound[i].activ)
		{
			curent_sound = i;
			break;
		}
		if (i == (SOUND_MAX-1))
		{
			curent_sound = 0;
			return;
		}
	}
	
	if (sound[curent_sound].activ)
	{
		if (sound[curent_sound].curent_cycles <= sound[curent_sound].number_cycles)
		{
			switch (sound[curent_sound].curent_takt)
			{
				case 1: 
					//Установка необходимой частоты
					if (sound[curent_sound].frequency == _1kHz)
					{
						SG0_SGFR0 = 20;
						SG0_SGDR0 = SG0_SGDR0 = 0x09;
						SG0_SGTR0 = (sound[curent_sound].takt_time_1 * 16) - 1;
					}
					else if (sound[curent_sound].frequency == _4kHz)
					{
						SG0_SGFR0 = 4;
						SG0_SGDR0 = (sound[curent_sound].takt_time_1 * 10) - 1;
						SG0_SGTR0 = 0x63;
					}
							
					Beep();
					sound[curent_sound].curent_takt++;
					count_cycles_one_takt = sound[curent_sound].takt_time_1 + sound[curent_sound].takt_time_2;
					break;
				case 2:
					count_cycles_one_takt--;
					if (count_cycles_one_takt == 0)
					{
						if ((sound[curent_sound].number_takt == 2) || (sound[curent_sound].number_takt == 1))
						{
							sound[curent_sound].curent_takt = 1;
							sound[curent_sound].curent_cycles++;
						}
						else
							sound[curent_sound].curent_takt++;
					}
					break;
				case 3:
					if (sound[curent_sound].frequency == _1kHz)
					{
						SG0_SGDR0 = SG0_SGDR0 = 0x09;
						SG0_SGTR0 = (sound[curent_sound].takt_time_3 * 16) - 1;
					}
					else if (sound[curent_sound].frequency == _4kHz)
					{
						SG0_SGDR0 = (sound[curent_sound].takt_time_3 * 10) - 1;
						SG0_SGTR0 = 0x63;
					}
					Beep();
					sound[curent_sound].curent_takt++;
					count_cycles_one_takt = sound[curent_sound].takt_time_3 + sound[curent_sound].takt_time_4;
					break;
				case 4:
					count_cycles_one_takt--;
					if (count_cycles_one_takt == 0)
					{
						sound[curent_sound].curent_takt = 1;
						// Если обрабатывается звуковой сигнал включения габаритов, то передается непрерывно
						if (curent_sound == sound_gabarity)
							sound[curent_sound].curent_cycles = 1;
						else
							sound[curent_sound].curent_cycles++;
					}
						
					break;
				default:
					break;
			}
		}
		else
		{
			sound[curent_sound].curent_cycles = 1;
			sound[curent_sound].activ = 0;
			curent_sound++;
			if (curent_sound == SOUND_MAX)
				curent_sound = 0;
			delay_time = 6;
		}
	}	
}

__interrupt void SoundGenInt(void)
{    
	SG0_SGCR0_ST = 0;
	SG0_SGCR0_INT = 0;            // clear irq - Flag
}
