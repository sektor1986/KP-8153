#include "mcu.h"
#include "button.h"
#include "driver_update.h"
#include "timer.h"
#include "flash.h"
#include "adc.h"
#include "icu.h"
#include "lcd.h"
#include "smc.h"
#include "geometry.h"
#include "extern_i2c.h"

#define I2C_ENABLE 0

#define VERS         0                // Версия программы 
#define USAGE_MEMORY        10        // Количество байт памяти используемых для настроек

#define LOW_FUEL        PDR17_P0 
#define BRAKE_ERROR     PDR03_P4
#define OPEN_DOOR       PDR03_P6

#define MAX_SPEED 200
#define SPEED_NO_CALC (MAX_SPEED + 50)

static en_gui_mode_t    m_enGuiMode = StandardView;

unsigned long value_fr_old = 0;                 // Переменная для хренения предыдущего значения счетчика частоты
unsigned int TimerFr = 0;                       // Переменная таймера обнуленя частоты
unsigned int TIME_OBNUL_FR = 0;                 // Расчетное время для обнуления

float Speed = 0.0;

unsigned char test_mode = 0;
unsigned int time_test = 0;

unsigned char flag_smcUpdate = 1;
unsigned int timer_mode_empty = 0;
unsigned char save_params_flag = 0;

static void DriverUpdate(void);
static void Timer_1s(void);
static void Timer_60s(void);
static unsigned int UpdateStateEngine(void);
static void CalcSpeed(void);

static void StartTestMode(void);
static void StopTestMode(void);

// Функции сохранения параметров
void LoadAllParams(void);
unsigned int SaveAllParams(void);
unsigned int SaveAllParamsSecondary(void);

void DriverInit(void)
{	
	float MAX_FREQUENSY = 0;
	
	LoadAllParams();		// Чтение параметров из flash
	DDR17_D0 = 1;	// SENS_P
	DDR03_D4 = 1;	// BRAKE_ERROR
	DDR03_D6 = 1;	// OPEN_DOOR
	TIME_OBNUL_FR = (36600/K)*10;  // Per = 3660*1000/K/10 
	MAX_FREQUENSY = ((float)SPEED_NO_CALC/ 3660.0 * (float)K);
	max_value = (16000000.0 / MAX_FREQUENSY);
	InitLCD();
	ENABLE_POINT = 1;
	ENABLE_KM = 1;
	#if (I2C_ENABLE == 1)
		init_setting_i2c();	// Инциализация интерфейса i2c
	#endif
	Timer_Start(TIMER_ID_MAIN, 20, TRUE, DriverUpdate);	
	Timer_Start(TIMER_1S, 1000, TRUE, Timer_1s);
	Timer_Start(TIMER_60S, 60000, TRUE, Timer_60s);
	if (adc_value[ADC_IGNITION] > 320)
		StartTestMode();
}

static void UdateLCD(void)
{	
	switch (m_enGuiMode)
	{
		case StandardView:                           // Режим вывода пробега (общий, суточный)
			//NumToTopStr((unsigned long)km/(K/10));
			//NumToTopStr((unsigned long)adc_value[ADC_BACKLIGHT]);
			NumToTopStr((double)km*10/K);
			NumToBottomStr((unsigned long)km_sut*100/K);			
			break;
				
		case Empty:
			ClearTopLine();
			ClearBottomLine();
			ENABLE_POINT = 0;
			ENABLE_KM = 0;
			break;	
								
		default: 
			break;
	}	
}

static void CalcSpeed(void)
{
	float frequency = 0.0;
	
	/* Указатель скорости */
	if (TimerFr > TIME_OBNUL_FR)
	{
		if (flag_freq == 0)
		{
			value = 0;
		}
		flag_freq = 0;
		TimerFr = 0;
	}
	else
	{
		TimerFr++;
	}
	
	// Расчет частоты
	frequency = 16000000.0/(float)value;
	// Расчет скорости с заданным коэфф.
	Speed = frequency*3660.0 / (float)K;
}

static void UpdateSMC(void)
{
	unsigned long temp_sms_inp = 0;

	float r = 0.0;
	float temp_float = 0.0;

	CalcSpeed();
	
	//Введение плюсовой погрешности
	if (Speed == 0)
		Speed = 0;
	else if (Speed <= 60)
		Speed += 1.6;
	else if (Speed <= 80)
		Speed += 2.0;
	else if (Speed <= 100)
		Speed += 2.4;
	else if (Speed <= 120)
		Speed += 2.8;
	else if (Speed <= 140)
		Speed += 3.2;
	else if (Speed <= 160)
		Speed += 3.6;
	else if (Speed <= 180)
		Speed += 4.0;
	else
		Speed += 4.4;
		
	temp_sms_inp = Speed * 614.4;   // 1 градус 512 шагов, 1км/ч = 240 / 200  = 1,2 гр; 1 км/ч = 1,2*512 = 614,4 шага; max = (205)*614.4 = 125952
	if (temp_sms_inp > 125952)
		temp_sms_inp = 125952;
	SMCpos[SPEEDOMETR].smc_inp = temp_sms_inp;
	/*Конец обрабoтки указателя обороотв*/
	
	/* Указатель уровня топлива */
	// Сопротивление	Показание прибора 	ADC 
	// 310				0					455
	// 237				0,125				373
	// 185				0,25				317
	// 118				0,5					227
	// 65				0,75				139
	// 7				1					17
	//y = = 0,000940x2 + 0,282763x + 3,844566
	
	r = 0.00094 * adc_value[ADC_FUEL] * adc_value[ADC_FUEL] + 0.282763 * adc_value[ADC_FUEL] + 3.844566;

	if (r > 237)
		temp_float = ValueLine(r, 237, 310, 0.125, 0);	
	else if (r > 185)
		temp_float = ValueLine(r, 185, 237, 0.25, 0.125);
	else if (r > 118)
		temp_float = ValueLine(r, 118, 185, 0.5, 0.25);
	else if (r > 65)
		temp_float = ValueLine(r, 65, 118, 0.75, 0.5);		
	else
		temp_float = ValueLine(r, 7, 65, 1.0,   0.75);
	
	if (temp_float > 1.0)
		temp_float = 1.0;
	else if (temp_float < 0)
		temp_float = 0;
		
	if (temp_float < 0.125)
		LOW_FUEL = 1;
	else
		LOW_FUEL = 0;
		
	// 1 градус 512 шагов, 1гр = 90 / 1  = 90 гр; 1гр = 90*512 = 46080 шага; max = (1+0.1)*61440 = 50688
	//SMCpos[FUEL_LEVEL].smc_inp = (long)(temp_float * 46080 + 5*512);
	SMCpos[FUEL_LEVEL].smc_inp = (long)(temp_float * 48000 + 5*512);
	/*Конец обрабoтки указателя уровня топлива */
}

static void DriverUpdate(void)
{
	if (test_mode)
	{
		time_test--;
		if (time_test)
		{
			//Проверяем наличие включенного зажигания и отсутствие скорости
			if ((adc_value[ADC_IGNITION] > 320) && (Speed < 3.0))
			{
				//Если прошла половина времени теста возвращаем стрелки назад
				if (time_test == 200)
				{
					SMCpos[SPEEDOMETR].smc_inp = 0;
					SMCpos[FUEL_LEVEL].smc_inp = 0;
				}
			}
			else
			{
				StopTestMode();
			}
		}
		else
		{
			StopTestMode();
		}
		return;
	}
	//Установка значения подсветки
	SetValueBacklight();
	// Отправка данных на второй контроллер
	#if (I2C_ENABLE == 1)
		send_data_i2c(); // Инциализация интерфейса i2c
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
	if ((adc_value[ADC_IGNITION] > 320)  && (adc_value[ADC_IGNITION] < 745))
	{	
		if ((m_enGuiMode == Empty) && (adc_value[ADC_IGNITION] > 340))
		{
			m_enGuiMode = StandardView;
			// Включение сигнализаторов на 3 сек после включения зажигания
			LOW_FUEL = 1;
			BRAKE_ERROR = 1;
			OPEN_DOOR = 1;
			Timer_Wait(TIMER_TIMEOUT, 2000, TRUE);
			LOW_FUEL = 0;
			BRAKE_ERROR = 0;
			OPEN_DOOR = 0;
			//************************************************************
			ENABLE_POINT = 1;
			ENABLE_KM = 1;
			SmcNormalParams();
			timer_mode_empty = 0;
		}
		result = 1;
		save_params_flag = 1;
	}
	else
	{	
		if (save_params_flag)
		{
			SaveAllParams();	
			save_params_flag = 0;
		}
		if (timer_mode_empty < 10)
			timer_mode_empty++;
		else
		{
			SmcParamsForReturn();
			if (m_enGuiMode != Empty)	
			{
				m_enGuiMode = Empty;
				ClearTopLine();
				ClearBottomLine();
				ENABLE_POINT = 0;
				ENABLE_KM = 0;
				LOW_FUEL = 0;
			}
			result = 0;
			Speed = 0.0;	
		}	
	}
	
	return result;
}


static void StartTestMode(void)
{
	test_mode = 1;
	time_test = 400; //20мс*400=8сек
	LOW_FUEL = 1;
	BRAKE_ERROR = 1;
	OPEN_DOOR = 1;
	NumToTopStr(888888);
	NumToBottomStr(8888);
	SmcTestParams();
	SMCpos[SPEEDOMETR].smc_inp = 122880;
	SMCpos[FUEL_LEVEL].smc_inp = 46080;
}
static void StopTestMode(void)
{
	test_mode = 1;
	time_test = 0;
	LOW_FUEL = 0;
	BRAKE_ERROR = 0;
	OPEN_DOOR = 0;
	ClearTopLine();
	ClearBottomLine();
	Disable_simbols();
	SmcNormalParams();
}

static void Timer_1s(void)
{
	
}

static void Timer_60s(void)
{

}

void ButoonPress(unsigned char enState)
{
	if (enState == 2)     //long press
	{
		switch (m_enGuiMode)
		{
			case StandardView:
				km_sut = 0;  
				break;
								
			default:
				break;
		}			
	}
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

	km = ((unsigned long)data[0] << 24) | 
	 		((unsigned long)data[1] << 16) | 
	  		((unsigned long)data[2] << 8) | 
	   		((unsigned long)data[3]);
	if (km == 0xFFFFFFFFUL)
		km = (unsigned long)K * 99990;

	km_sut = ((unsigned long)data[4] << 24) | 
	 		((unsigned long)data[5] << 16) | 
	  		((unsigned long)data[6] << 8) | 
	   		((unsigned long)data[7]);
	if (km_sut == 0xFFFFFFFFUL)
		km_sut = (unsigned long)K * 90;

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

	//Сохранение пробега 4 байта + 1 CRC
	data[0] = ((unsigned long)km >> 24) & 0xFF; 	
	data[1] = ((unsigned long)km >> 16) & 0xFF; 
	data[2] = ((unsigned long)km >> 8) & 0xFF; 
	data[3] = ((unsigned long)km) & 0xFF; 
	
	// Сохранение пробега суточного
	data[4] = ((unsigned long)km_sut >> 24) & 0xFF; 	
	data[5] = ((unsigned long)km_sut >> 16) & 0xFF; 
	data[6] = ((unsigned long)km_sut >> 8) & 0xFF; 
	data[7] = ((unsigned long)km_sut) & 0xFF; 
	
	data[8] = 0;
	
	//CRC
	for (i = 0; i < (USAGE_MEMORY-1); i++)
		CRC = CRC + data[i];
		
	data[USAGE_MEMORY-1] = ~CRC;	
	
	Data_flash_SectorErase(SA2);	
	
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

	//Сохранение пробега 4 байта + 1 CRC
	data[0] = ((unsigned long)km >> 24) & 0xFF; 	
	data[1] = ((unsigned long)km >> 16) & 0xFF; 
	data[2] = ((unsigned long)km >> 8) & 0xFF; 
	data[3] = ((unsigned long)km) & 0xFF; 
	
	// Сохранение пробега суточного
	data[4] = ((unsigned long)km_sut >> 24) & 0xFF; 	
	data[5] = ((unsigned long)km_sut >> 16) & 0xFF; 
	data[6] = ((unsigned long)km_sut >> 8) & 0xFF; 
	data[7] = ((unsigned long)km_sut) & 0xFF; 
	
	data[8] = 0;
	
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
