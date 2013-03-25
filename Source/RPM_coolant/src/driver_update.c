#include "mcu.h"
#include "button.h"
#include "driver_update.h"
#include "timer.h"
#include "flash.h"
#include "smc.h"
#include "adc.h"
#include "icu.h"
#include "j1939.h"
#include "extern_i2c.h"
#include "lcd.h"
#include "rtc.h"
#include "lcd.h"
#include "sound.h"

#define I2C_ENABLE 0

#define VERS					0			// Версия программы 
#define USAGE_MEMORY			10			// Количество байт памяти используемых для настроек

#define EDC						PDR01_P0
#define PARKING_BRAKE			PDR12_P6
#define ERROR_OIL_PRESSURE		PDR02_P2
#define GABARITY				PDR01_P3
#define PEREGREV				PDR06_P6
#define EFU						PDR04_P0
#define REMNI_BEZOP				PDR03_P3
#define KMPSUD					PDR04_P1

static en_gui_mode_t    m_enGuiMode = mode_Time;

unsigned char blink = 0;                        // Мерцание
unsigned int TimerBlink = 0;                    // Переменная таймера мерцания
float Voltage = 0.0;                            // Значение показываемого напряжения 
float Pressure = 0.0;                           // Значение показываемого давления
int OutTemperature = 0;

static void DriverUpdate(void);
static void Timer_1s(void);
static void Timer_60s(void);
static unsigned int UpdateStateEngine(void);
static void UpdateTime(void);

// Функции сохранения параметров

void LoadAllParams(void);
unsigned int SaveAllParams(void);
unsigned int SaveAllParamsSecondary(void);

void DriverInit(void)
{	
	DDR01_D1 = 1;	// SENS_P
	DDR01_D0 = 1;	// EDC
	DDR12_D6 = 0;	// PARKING_BRAKE
	DDR02_D2 = 1;	// ERROR_OIL_PRESSURE
	DDR01_D3 = 0;	// GABARITY
	DDR06_D6 = 1;	// PEREGREV
	DDR04_D0 = 1;	// EFU
	DDR03_D3 = 1;	// REMNI_BEZOP
	DDR04_D1 = 1;	// KMPSUD
	
	InitLCD();
//	InitSoundGen();
	LoadAllParams();	// Чтение параметров из flash
	Timer_Start(TIMER_ID_MAIN, 20, TRUE, DriverUpdate);	
	Timer_Start(TIMER_1S, 1000, TRUE, Timer_1s);
	Timer_Start(TIMER_60S, 60000, TRUE, Timer_60s);
	#if (I2C_ENABLE == 1)
		init_setting_i2c();
	#endif
	InitMode(mode_Time);
}

void InitMode(en_gui_mode_t mode)
{
	Disable_simbols();
	m_enGuiMode = mode;
	
	switch (mode)
	{
		case mode_Time:
			SIMBOL_TIME = 1;
			break;
		
		case mode_Voltage:
			SIMBOL_V = 1;
			ENABLE_POINT = 1;
			break;
		
		case mode_Pressure:
			segment1(11);		//P
			ENABLE_POINT = 1;
			break;
		
		case mode_Temperature:
			SIMBOL_TEMP = 1;	
			break;
		
		case Empty:
			ClearTopLine();
			break;
		
		default:
			break;
	}
}

static void UdateLCD(void)
{	
	switch (m_enGuiMode)
	{
		case mode_Time:
			UpdateTime();
			break;
			
		case mode_Voltage:
			NumToTopStr((unsigned long)(Voltage*10.0));
			break;
		
		case mode_Pressure:
			Pressure = -0.01544*adc_value[ADC_OIL_PRESS_6] + 6.59703;
			if (Pressure < 0.0)
				Pressure = 0;
			NumToTopStr3((unsigned long)(Pressure*10.0));
			break;
		
		case mode_Temperature:
			NumToTopStr((unsigned long)OutTemperature);
			break;
				
		case Empty:
			break;	
								
		default: 
			break;
	}	
}

static void UpdateSMC(void)
{
	unsigned int rpm_value = 0;
	
	
	//Тахометр
	if (J1939CtrBufer[RPM].Available)
	{
		rpm_value = J1939CtrBufer[RPM].CanMessage.stcData.aucData[0]*40; ////!!!!!!!!!!!!!!!!!!!!!!
	}
	else
	{
		rpm_value = 0;
	}
	
	// 1 градус 512 шагов, 1prm = 240 / 6000  = 0.04 гр; 1rpm = 0.04*512 = 20.48 шага; max = (1+0.1)*61440 = 50688
	SMCpos[SMC_RPM].smc_inp = (long)(20.48 * rpm_value);
	
}

static void DriverUpdate(void)
{
	// Получение данных с контроллера
	#if (I2C_ENABLE == 1)
		receive_data_i2c();
	#endif
	//Проверка состояния зажигания
	if 	(UpdateStateEngine())
	{
		// Обновление покзаний стрелочных приборов
		UpdateSMC();
		// Обновление показаний дисплея	
		UdateLCD();	
	}
	
}

// Функция провеки состояния напряжения зажигания
static unsigned int UpdateStateEngine(void)
{
	unsigned int result = 0;
	
	result = 1;
	
	return result;
}

static void Timer_1s(void)		
{
	PARKING_BRAKE = ~PARKING_BRAKE;
}

static void Timer_60s(void)
{

}

void ButtonCallback(uint16_t u16ButtonId, en_button_state_t enState)
{
	char result = 0;
	unsigned char temp_min = 0;
	unsigned char temp_sec = 0;
	
	if (enState == StateLow)
	{
		return;
	}
    
	if (enState == StateHigh)
	{
		switch (u16ButtonId)
		{
			case BUTTON_ID_B1:
				if (m_enGuiMode < mode_Temperature)
					m_enGuiMode++;
				else
					m_enGuiMode = mode_Time;
				InitMode(m_enGuiMode);
				break;
				
			case BUTTON_ID_B2:
				switch (m_enGuiMode)
				{
					case mode_Time:
						temp_min = WTMR;
						temp_sec = WTSR;
						if (WTHR < 23)
							WTHR++;
						else
							WTHR = 0;
						WTMR = temp_min;
						WTSR = temp_sec;
						WTCR_UPDT = 1;
						break;
					default:
						break;
				}
				break;
			
			case BUTTON_ID_B3:
				switch (m_enGuiMode)
				{
					case mode_Time:
						if (WTMR < 59)
							WTMR++;
						else
							WTMR = 0;
						WTSR = 0;
						WTCR_UPDT = 1;
						break;
					default:
						break;
				}
				break;
        
			default:
				break;
		}	
    }    
    
	if (enState == StateLong)  
	{      
		switch (u16ButtonId)
		{
			case BUTTON_ID_B1:
				break;
				
			case BUTTON_ID_B2:
				switch (m_enGuiMode)
				{
					case mode_Time:
						if (WTHR < 23)
							WTHR++;
						else
							WTHR = 0;
						WTCR_UPDT = 1;
						break;
					default:
						break;
				}
				break;
				
			case BUTTON_ID_B3:
				switch (m_enGuiMode)
				{
					case mode_Time:
						if (WTMR < 59)
							WTMR++;
						else
							WTMR = 0;
						WTSR = 0;
						WTCR_UPDT = 1;
						break;
					default:
						break;
				}
				break;
        
			default:
				break;
		}
    }	
}

static void UpdateTime(void)
{
	unsigned char hour = 0;
	unsigned char min  = 0;
	
	TimerBlink++;
	if (TimerBlink > 25)
	{
		blink = ~blink;
		TimerBlink = 0;
	}
	SIMBOL_TIME = blink;
	
	segment1(WTHR / 10);
	segment2(WTHR % 10);
	segment3(WTMR / 10);
	segment4(WTMR % 10);	
}

void LoadAllParams(void)
{
	unsigned char data[USAGE_MEMORY];
	unsigned char ReadOk = 0;
	unsigned int temp = 0;
	unsigned char CRC = 0;
	unsigned char i = 0;
	
	for (i = 0; i < (USAGE_MEMORY/2); i++)
	{
		temp = *(unsigned int __far*)(SA2+i*2);	
		data[i*2] = (temp >> 8) & 0xFF;
		data[i*2+1] = temp & 0xFF;		
	}	
	
	for (i = 0; i < (USAGE_MEMORY-1); i++)
		CRC = CRC + data[i];
	
	CRC = ~CRC;
	// Если контрольная сумма совпадает 
	if (CRC == data[USAGE_MEMORY-1])
	{
		ReadOk = 1;
	}
	else
	{
		// Если ошибка чтения то считываем дублирающие данные
		for (i = 0; i < (USAGE_MEMORY/2); i++)
		{
			temp = *(unsigned int __far*)(SA3+i*2);	
			data[i*2] = (temp >> 8) & 0xFF;
			data[i*2+1] = temp & 0xFF;		
		} 
		ReadOk= 0;
	}

	if (ReadOk)
		//Сохраняем значения
		SaveAllParamsSecondary();
	Data_flash_SectorErase(SA2);
}

unsigned int SaveAllParams(void)
{
	unsigned int result = 0;
	unsigned char data[USAGE_MEMORY];
	unsigned char CRC = 0;
	unsigned int temp = 0;
	unsigned char i = 0;
	
	//CRC
	for (i = 0; i < (USAGE_MEMORY-1); i++)
		CRC = CRC + data[i];
		
	data[USAGE_MEMORY-1] = ~CRC;	
					
	for (i = 0; i < (USAGE_MEMORY/2); i++)
	{
		temp = ((unsigned int)data[2*i] << 8) | (unsigned int)data[2*i+1];
		Data_Flash_write(SA2+i*2, temp);
		while (temp != *(unsigned int __far*)(SA2+i*2));	
	}
		
	return result;
}

unsigned int SaveAllParamsSecondary(void)
{
	unsigned int result = 0;
	unsigned char data[USAGE_MEMORY];
	unsigned char CRC = 0;
	unsigned int temp = 0;
	unsigned char i = 0;

	//CRC
	for (i = 0; i < (USAGE_MEMORY-1); i++)
		CRC = CRC + data[i];
	data[USAGE_MEMORY-1] = ~CRC;	

	Data_flash_SectorErase(SA3);					
	for (i = 0; i < (USAGE_MEMORY/2); i++)
	{
		temp = ((unsigned int)data[2*i] << 8) | (unsigned int)data[2*i+1];
		Data_Flash_write(SA3+i*2, temp);
		while (temp != *(unsigned int __far*)(SA3+i*2));	
	}
		
	return result;
}
