#ifndef __DRIVER_UPDATE_H
#define __DRIVER_UPDATE_H

typedef enum en_gui_mode
{
    mode_Time,
    mode_Voltage,
    mode_Pressure,
    mode_Pressure2,
    Empty
} en_gui_mode_t;

void DriverInit(void);
extern void ButtonCallback(uint16_t u16ButtonId, en_button_state_t enState);

#endif