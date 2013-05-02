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

#define VERS         0                // Версия программы 
#define USAGE_MEMORY        10        // Количество байт памяти используемых для настроек

#define VOLTAGE                 0
#define SPEED                   1
#define STATE_DOOR              2
#define STATE_FUEL              3
#define RAB_TORMOZ              4
#define GABARITY                5

#define LOW_FUEL                PDR17_P0 
#define BRAKE_ERROR             PDR03_P4
#define OPEN_DOOR               PDR03_P6

#define INPUT_RAB_TORMOZ        PDR04_P0
#define INPUT_OPEN_DOOR         PDR04_P1

#define MAX_SPEED 200
#define SPEED_NO_CALC (MAX_SPEED + 50)

static en_gui_mode_t    m_enGuiMode = StandardView;

unsigned int TimerFr = 0;                       // Переменная таймера обнуленя частоты
unsigned int TIME_OBNUL_FR = 0;                 // Расчетное время для обнуления

float Speed = 0.0;

unsigned char test_mode = 0;
unsigned int time_test = 0;

unsigned char flag_smcUpdate = 1;
unsigned int timer_mode_empty = 0;
unsigned char save_params_flag = 0;
static unsigned char timeSetFulelLevel = 5;
static unsigned char stateEngine = 0;

unsigned char	*m_tr_data0;	/* master transmission data area */
unsigned char	*m_re_data0;	/* master reception data area */
unsigned int	m_tr_num0;		/* master transmission data counter */
unsigned int	m_re_num0;		/* master reception data counter */
unsigned char	*s_tr_data0;	/* slave transmission data area */
unsigned char	*s_re_data0;	/* slave reception data area */
unsigned int	s_tr_num0;		/* slave transmission data counter */
unsigned int	s_re_num0;		/* slave reception data counter */

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
	
	DDR04_D0 = 0;	// INPUT_RAB_TORMOZ
	PIER04_IE0 = 1;
	DDR04_D1 = 0;	// INPUT_OPEN_DOOR
	PIER04_IE1 = 1;
	
	TIME_OBNUL_FR = (36600/K)*10;  // Per = 3660*1000/K/10 
	MAX_FREQUENSY = ((float)SPEED_NO_CALC/ 3660.0 * (float)K);
	max_value = 2*(16000000.0 / MAX_FREQUENSY);
	timeSetFulelLevel = 5;
	InitLCD();
	Timer_Start(TIMER_ID_MAIN, 20, TRUE, DriverUpdate);
	Timer_Start(TIMER_1S, 1000, TRUE, Timer_1s);
	Timer_Start(TIMER_60S, 60000, TRUE, Timer_60s);

	#if (I2C_ENABLE == 1)
		PIER04_IE5 = 1;
		PIER04_IE4 = 1;	/* i2c 0 */
		initial_set();
		init_i2c_0_master();
	#endif
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
			ENABLE_POINT = 1;
			ENABLE_KM = 1;
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
	if (value != 0)
	{
		frequency = 16000000.0/((float)(value/2));
		// Расчет скорости с заданным коэфф.
		Speed = frequency*3660.0 / (float)K;
	}
	else
		Speed = 0;
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
	else if (Speed <= 20)
		Speed += 3.0;
	else if (Speed <= 180)
		Speed += 1.8;
	else
		Speed += 3.0;
		
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
	
	// 1 градус 512 шагов, 1гр = 90 / 1  = 90 гр; 1гр = 90*512 = 46080 шага; max = (1+0.1)*61440 = 50688
	//SMCpos[FUEL_LEVEL].smc_inp = (long)(temp_float * 46080 + 5*512);
	SMCpos[FUEL_LEVEL].smc_inp = (long)(temp_float * 48000 + 5*512);
	
	// Зажигание сигнализатора "низкий уровень топлва"
	if (SMCpos[FUEL_LEVEL].smc_new < 8560)
		LOW_FUEL = 1;
	else
		LOW_FUEL = 0;
	/*Конец обрабoтки указателя уровня топлива */
}

static void DriverUpdate(void)
{	
	m_tr_data0[VOLTAGE] = adc_value[ADC_IGNITION] >> 2;
	m_tr_data0[SPEED] = (uint8_t)Speed;
	
	if (INPUT_RAB_TORMOZ == 0)
		m_tr_data0[RAB_TORMOZ] = 1;
	else
		m_tr_data0[RAB_TORMOZ] = 0;
		
	if (INPUT_OPEN_DOOR == 0)
		m_tr_data0[STATE_DOOR] = 1;
	else
		m_tr_data0[STATE_DOOR] = 0;
	m_tr_data0[GABARITY] = adc_value[ADC_BACKLIGHT] >> 2;
			
	m_tr_data0[STATE_FUEL] = LOW_FUEL;
	// Отправка данных на второй контроллер
	#if (I2C_ENABLE == 1)
		SendDataI2C();
	#endif
	
	if (test_mode)
	{
		time_test--;
		if (time_test)
		{	
			CalcSpeed();
			//Проверяем наличие включенного зажигания и отсутствие скорости
			if ((adc_value[ADC_IGNITION] > 320) && (Speed < 3.0))
			{
				//Если прошла половина времени теста возвращаем стрелки назад
				if (time_test == 220)
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

	//Проверка состояния зажигания
	if 	(UpdateStateEngine())
	{
		// Обновление покзаний стрелочных приборов
		UpdateSMC();
		// Обновление показаний дисплея	
		UdateLCD();
		stateEngine = 1;
	}
	else
	{	
		stateEngine = 0;
	}
	
}

// Функция провеки состояния напряжения зажигания
static unsigned int UpdateStateEngine(void)
{
	unsigned int result = 0;
	if ((adc_value[ADC_IGNITION] > 320)  && (adc_value[ADC_IGNITION] < 820))
	{	
		if ((m_enGuiMode == Empty) && (adc_value[ADC_IGNITION] > 340))
		{
			m_enGuiMode = StandardView;
			SmcNormalParams();
			// Включение сигнализаторов на 3 сек после включения зажигания
			LOW_FUEL = 1;
			BRAKE_ERROR = 1;
			OPEN_DOOR = 1;
			TimerFr  = TIME_OBNUL_FR + 1; ///!!!!!!!!!
			Timer_Wait(TIMER_TIMEOUT, 2000, TRUE);
			timeSetFulelLevel = 5;
			LOW_FUEL = 0;
			BRAKE_ERROR = 0;
			OPEN_DOOR = 0;
			//************************************************************
			ENABLE_POINT = 1;
			ENABLE_KM = 1;
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
		if (timer_mode_empty < 5)
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
	time_test = 440; //20мс*400=8.8сек
	LOW_FUEL = 1;
	BRAKE_ERROR = 1;
	OPEN_DOOR = 1;
	Speed = 0;
	NumToTopStr(888888);
	NumToBottomStr(8888);
	ENABLE_POINT = 1;
	ENABLE_KM = 1;
	SmcTestParams();
	SMCpos[SPEEDOMETR].smc_inp = 124000;
	SMCpos[FUEL_LEVEL].smc_inp = 52000;
}
static void StopTestMode(void)
{
	test_mode = 0;
	time_test = 0;
	LOW_FUEL = 0;
	BRAKE_ERROR = 0;
	OPEN_DOOR = 0;
	ClearTopLine();
	ClearBottomLine();
	Disable_simbols();
	SMCpos[SPEEDOMETR].smc_inp = 0;
	SMCpos[FUEL_LEVEL].smc_inp = 0;
	Timer_Wait(TIMER_TIMEOUT, 500, TRUE);
	SmcNormalParams();
}

static void Timer_1s(void)
{
	if (!test_mode && stateEngine)
	{
		// время на установку значения уровня топливав
		// при включениизажигания
		if (timeSetFulelLevel == 0)
		{
			if (Speed > 3.0)
				SmcFuelDemp();
			else
				SmcNormalParams();
		}
		else
			timeSetFulelLevel--;
	}
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
