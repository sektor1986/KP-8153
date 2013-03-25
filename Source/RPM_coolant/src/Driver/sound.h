#ifndef __SOUND_H
#define __SOUND_H

/*
typedef enum en_sound_mode
{
	
}en_sound_mode_t;
*/

void InitSoundGen(void)	;
void Beep(void);
__interrupt void SoundGenInt(void);

#endif // __SOUND_H