#ifndef __BACKLIGHT_H
#define __BACKLIGHT_H

extern unsigned long Backlight; 

void InitBacklight(void);
void SetValueBacklight(void);
__interrupt void Backlight_IRQ(void);


#endif
