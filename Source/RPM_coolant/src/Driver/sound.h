#ifndef __SOUND_H
#define __SOUND_H

#include "base_types.h"

enum en_sound_mode
{
	sound_brake_EBD = 0,
	sound_oil_pressure,
	sound_peregrev,
	sound_parking_brake,
	sound_open_door,
	sound_low_fuel,
	sound_remni,
	sound_gabarity,
	SOUND_MAX
};

typedef enum en_frequency
{
	_1kHz,
	_4kHz
}en_frequency_t;

typedef struct stc_sound
{
	uint8_t activ;
	const uint8_t prioryti;
	const uint8_t number_cycles;
	const uint8_t number_takt;
	const uint8_t takt_time_1;
	const uint8_t takt_time_2; 
	const uint8_t takt_time_3; 
	const uint8_t takt_time_4; 
	uint8_t curent_cycles;
	uint8_t curent_takt;
	const en_frequency_t frequency;
	uint8_t worked_out;
}stc_sound_t;

void InitSoundGen(void);
void Beep(void);
void UpdateSound(void);
void ActivateSound(uint8_t num);
void DeactivateSound(uint8_t num);
void DeactivateAllSound(void);
__interrupt void SoundGenInt(void);

#endif // __SOUND_H