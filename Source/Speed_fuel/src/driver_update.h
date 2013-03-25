#ifndef __DRIVER_UPDATE_H
#define __DRIVER_UPDATE_H

typedef enum en_gui_mode
{
    StandardView,
    Empty
} en_gui_mode_t;

void DriverInit(void);
void ButoonPress(unsigned char enState);

#endif