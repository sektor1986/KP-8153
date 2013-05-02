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
#include "geometry.h"

#define I2C_ENABLE 1

#define VERS					0			// Версия программы 
#define USAGE_MEMORY			10			// Количество байт памяти используемых для настроек

// Выходы 
#define EDC						PDR01_P0
#define PARKING_BRAKE			PDR12_P6
#define ERROR_OIL_PRESSURE		PDR02_P2
#define PEREGREV				PDR06_P6
#define EFU						PDR04_P0
#define REMNI_BEZOP				PDR03_P3
#define KMPSUD					PDR04_P1
#define RAB_TORM_EBD			PDR01_P3
//Входы
#define INPUT_ERROR_OIL			PDR06_P7
#define INPUN_EBD				PDR03_P2
#define INPUN_REMNI_BEZOP		PDR03_P3
#define INPUT_PARKING_BRAKE		PDR11_P7
#define INPUT_PEREGREV			PDR17_P0

#define VOLTAGE                 0
#define SPEED                   1
#define STATE_DOOR              2
#define STATE_FUEL              3
#define RAB_TORMOZ              4
#define GABARITY                5

static en_gui_mode_t    m_enGuiMode = mode_Time;
static en_gui_mode_t    m_enGuiMode_before_empty = mode_Time;
static en_gui_mode_t    m_enGuiMode_before_error = mode_Time;

unsigned char test_mode = 0;
unsigned int time_test = 0;

unsigned char blink = 0;                        // Мерцание
unsigned int TimerBlink = 0;                    // Переменная таймера мерцания
unsigned char stateEngine = 0;                 // Состояние зажигания 
float Voltage = 0.0;                            // Значение показываемого напряжения 
float Pressure = 0.0;                           // Значение показываемого давления
float Pressure2 = 0.0;                           // Значение показываемого давления
int OutTemperature = 0;
unsigned int rpm = 0;                           // Обороты двигателя
static unsigned char pressure_error = 0;
static unsigned int timer_pressure_error = 250;
static unsigned char pressure2_error = 0;
static unsigned int timer_pressure2_error = 250;
static unsigned char voltage_error = 0;
static unsigned int timer_voltage_error = 250;
unsigned char sens_pressure_activ = 0;
unsigned char sens_pressure2_activ = 0;

unsigned int TimerFr = 0;                       // Переменная таймера обнуленя частоты
unsigned int TIME_OBNUL_FR = 0;                 // Расчетное время для обнуления

unsigned char	m_tr_data0[0x100];	/* master transmission data area */
unsigned char	m_re_data0[0x100];	/* master reception data area */
unsigned int	m_tr_num0;		/* master transmission data counter */
unsigned int	m_re_num0;		/* master reception data counter */
unsigned char	s_tr_data0[0x100];	/* slave transmission data area */
unsigned char	s_re_data0[0x100];	/* slave reception data area */
unsigned int	s_tr_num0;		/* slave transmission data counter */
unsigned int	s_re_num0;		/* slave reception data counter */

static void DriverUpdate(void);
static void Timer_1s(void);
static void Timer_250ms(void);
static void CalcRPM(void);
static void CalcPressure(void);
static void CalcVoltage(void);
static void UpdatePressure(void);
static void UpdatePressure2();
static void UpdateVoltage(void);
static unsigned int UpdateStateEngine(void);
static void UpdateTime(void);
static void InitMode(en_gui_mode_t mode);

static void StartTestMode(void);
static void StopTestMode(void);

// Функции сохранения параметров
void LoadAllParams(void);
unsigned int SaveAllParams(void);
unsigned int SaveAllParamsSecondary(void);

void DriverInit(void)
{	
	//Выходы 
	DDR01_D1 = 1;	// SENS_P
	DDR01_D0 = 1;	// EDC
	DDR12_D6 = 1;	// PARKING_BRAKE
	DDR02_D2 = 1;	// ERROR_OIL_PRESSURE
	DDR06_D6 = 1;	// PEREGREV
	DDR04_D0 = 1;	// EFU
	DDR03_D3 = 1;	// REMNI_BEZOP
	DDR04_D1 = 1;	// KMPSUD
	DDR01_D3 = 1;	// RAB_TORM_EBD
	
	//Входы
	DDR06_D7 = 0;	// INPUT_ERROR_OIL
	PIER06_IE7 = 1;
	DDR03_D3 = 0;	// INPUN_REMNI_BEZOP
	PIER03_IE3 = 1;
	DDR03_D2 = 0;	// INPUN_EBD
	PIER03_IE2 = 1;	
	DDR11_D7 = 0;	// INPUT_PARKING_BRAKE
	PIER11_IE7 = 1;
	DDR17_D0 = 0;	// INPUT_PEREGREV
	PIER17_IE0 = 1;	
	
	TIME_OBNUL_FR = 60;
	InitLCD();
	InitSoundGen();
	LoadAllParams();	// Чтение параметров из flash
	Timer_Start(TIMER_ID_MAIN, 20, TRUE, DriverUpdate);	
	Timer_Start(TIMER_1S, 1000, TRUE, Timer_1s);
	Timer_Start(TIMER_250ms, 250, TRUE, Timer_250ms);
	InitMode(mode_Time);
	
	//**************************
	Timer_Wait(TIMER_TIMEOUT, 200, TRUE);
	if (s_re_data0[0] > 80)
		StartTestMode();
}

static void InitMode(en_gui_mode_t mode)
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
			Voltage = (0.09083*s_re_data0[0] - 0.25631);
			break;
		
		case mode_Pressure:
			// Если датчик давления активен или если нет второго датчика
			if ((sens_pressure_activ) || (!sens_pressure2_activ))
			{
				segment1(11);		//P
				ENABLE_POINT = 1;
			}
			else
				InitMode(mode_Pressure2);
			break;
		
		case mode_Pressure2:
			if (sens_pressure2_activ)
			{
				segment1(11);		//P
				ENABLE_POINT = 1;
			}
			else
				InitMode(mode_Time);
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
			UpdateVoltage();
			break;
		
		case mode_Pressure:
			UpdatePressure();
			break;
		
		case mode_Pressure2:
			UpdatePressure2();
			break;
				
		case Empty:
			UpdateTime();
			break;	
								
		default: 
			break;
	}	
}

static void UpdateSMC(void)
{	
	float temp_float = 0.0;
	float duration = 0.0;
	static unsigned char timer = 0;
	
	//Тахометр
	CalcRPM();
	if (rpm > 6000)
		rpm = 6000;
	// 1 градус 512 шагов, 1prm = 240 / 6000  = 0.04 гр; 1rpm = 0.04*512 = 20.48 шага; max = (1+0.1)*61440 = 50688
	SMCpos[SMC_RPM].smc_inp = (long)(20.48 * rpm);
	//*************************************************
		
	//*****Температура охлаждающей жидкости****
	if (timer > 10)
	{
		if (flag_temp == 0)
		{
			valueTemper = 0;
		}
		flag_temp = 0;
		timer = 0;
	}
	else
	{
		timer++;
	}
	if (valueTemper != 0)
		duration = (float)valueTemper/ 16000;
	else
		duration = 0;
	
	if ((duration > 0) && (duration < 20))
		temp_float = -13.328930*duration + 151.255614;
	else
	{	
		/* Указатель уровня топлива (резистивный) */
		// Сопротивление	Показание прибора 	ADC 
		// 1092.5			40					733
		// 490				60					550
		// 240				80					372
		// 131				100					245
		// 114				105					220
		// 75				120					156

		if (adc_value[ADC_COOLANT_TEMP] > 733)
			temp_float = ValueLine(adc_value[ADC_COOLANT_TEMP], 550, 733, 60, 40);	
		else if (adc_value[ADC_COOLANT_TEMP] > 185)
			temp_float = ValueLine(adc_value[ADC_COOLANT_TEMP], 372, 550, 80, 60);
		else if (adc_value[ADC_COOLANT_TEMP] > 118)
			temp_float = ValueLine(adc_value[ADC_COOLANT_TEMP], 245, 372, 100, 80);
		else if (adc_value[ADC_COOLANT_TEMP] > 65)
			temp_float = 5+ValueLine(adc_value[ADC_COOLANT_TEMP], 220, 245, 105, 100);		
		else
			temp_float = 5+ValueLine(adc_value[ADC_COOLANT_TEMP], 156, 220, 120, 105);
	}	
	
	if (temp_float > 123)
		temp_float = 123;
	else if (temp_float < 35)
		temp_float = 35;
	
	// 1 градус 512 шагов, 1гр = 90 / 80  = 1.125 гр; 1гр = 1.125*512 = 576 шага; max = (1+0.1)*61440 = 50688
	//Начально положение -5гр. 5гр = 5*576шагов  = 2880
	SMCpos[SMC_COOLANT].smc_inp = (long)((temp_float-40) * 576 + 2880);
	/*Конец обрабoтки указателя уровня топлива */
}

static void DriverUpdate(void)
{
	if (test_mode)
	{
		time_test--;
		if (time_test)
		{	
			CalcRPM();
			//Проверяем наличие включенного зажигания и отсутствие скорости
//			if ((rpm < 50.0) && (s_re_data0[0] > 80) && (s_re_data0[1] < 3))
			if ((s_re_data0[0] > 80) && (s_re_data0[1] < 3))
			{
				//Если прошла половина времени теста возвращаем стрелки назад
				if (time_test == 220)
				{
					SMCpos[SMC_COOLANT].smc_inp = 0;
					SMCpos[SMC_RPM].smc_inp = 0;
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
	
	TimerBlink++;
	if (TimerBlink > 25)
	{
		blink = ~blink;
		TimerBlink = 0;
	}
	
	
		//Обработка сигнализатора стояночного тормоза
	if (INPUT_PARKING_BRAKE == 0)
	{
		if (blink)
			PARKING_BRAKE = 1;
		else
			PARKING_BRAKE = 0;
	}
	else
	{
		PARKING_BRAKE = 0;
	}
	
	// Обработка включения сигнализатора работчего тормоза при включениии EBD 
	if (INPUN_EBD = 1)
		RAB_TORM_EBD = 1;
	else
		RAB_TORM_EBD = 0;
	// Обновление показаний дисплея	
	UdateLCD();
	
	//Проверка состояния зажигания
	if 	(UpdateStateEngine())
	{
		stateEngine = 1;
		//Расчет давления масла
		CalcPressure();
		CalcVoltage();
		
		//Проверка на аварийное давление 1
		if (((Pressure < 0.4) || (Pressure > 5.0)) && (sens_pressure_activ))
		{
			//Проверяем сколько времени уже ошибочное давление
			if (timer_pressure_error)
				timer_pressure_error--;
			else
			{
				if (!pressure_error)
				{
					m_enGuiMode_before_error = m_enGuiMode;
					InitMode(mode_Pressure);
					pressure_error = 1;
				}
			}
		}
		else
		{	
			if (pressure_error)
			{
				InitMode(m_enGuiMode_before_error);
				pressure_error = 0;
				timer_pressure_error = 250;
			}
		}
	
		//Проверка на аварийное давление 2
		if (((Pressure2 < 0.4) || (Pressure2 > 8.0)) && sens_pressure2_activ)
		{
			//Проверяем сколько времени уже ошибочное давление
			if (timer_pressure2_error)
				timer_pressure2_error--;
			else
			{
				if (!pressure2_error)
				{
					m_enGuiMode_before_error = m_enGuiMode;
					InitMode(mode_Pressure2);
					pressure2_error = 1;
				}
			}
		}
		else
		{	
			if (pressure2_error)
			{
				InitMode(m_enGuiMode_before_error);
				pressure2_error = 0;
				timer_pressure2_error = 250;
			}
		}
		
		if ((Voltage < 10.8) || (Voltage > 15.0))
		{
			//Проверяем сколько времени уже ошибочное давление
			if (timer_voltage_error)
				timer_voltage_error--;
			else
			{
				if (!voltage_error)
				{
					m_enGuiMode_before_error = m_enGuiMode;
					InitMode(mode_Voltage);
					voltage_error = 1;
				}
			}
		}
		else
		{	
			if (voltage_error)
			{
				InitMode(m_enGuiMode_before_error);
				voltage_error = 0;
				timer_voltage_error = 250;
			}
		}
				
		// Обновление покзаний стрелочных приборов
		UpdateSMC();
	}
	else
		stateEngine = 0;
	
}

// Функция провеки состояния напряжения зажигания
static unsigned int UpdateStateEngine(void)
{
	unsigned int result = 0;
	static unsigned char timer_mode_empty = 0;
	
	if ((s_re_data0[0] > 80)  && (s_re_data0[0] < 205))
	{	
		if ((m_enGuiMode == Empty) && (s_re_data0[0] > 85))
		{
			SmcNormalParams();
			m_enGuiMode = m_enGuiMode_before_empty;
			InitMode(m_enGuiMode_before_empty);
			// Включение сигнализаторов на 3 сек после включения зажигания
			EDC = 1;
			PARKING_BRAKE = 1;
			ERROR_OIL_PRESSURE = 1;
			PEREGREV = 1;
			EFU = 1;
			KMPSUD = 1;
			Timer_Wait(TIMER_TIMEOUT, 2000, TRUE);
			EDC = 0;
			PARKING_BRAKE = 0;
			ERROR_OIL_PRESSURE = 0;
			PEREGREV = 0;
			EFU = 0;
			KMPSUD = 0;
			timer_mode_empty = 0;
		}
		result = 1;
	}
	else
	{	
		if (timer_mode_empty < 10)
			timer_mode_empty++;
		else
		{
			DeactivateAllSound();
			SmcParamsForReturn();
			if (m_enGuiMode != Empty)
			{
				m_enGuiMode_before_empty = m_enGuiMode;
				InitMode(Empty);		
			}
			result = 0;
		}	
	}
	
	return result;
}

static void Timer_1s(void)		
{
	static unsigned char time_pressure_error = 0;
	
	if (test_mode)
		return;
	
	if (stateEngine == 0)
	{
		//Включены габариты, открыты двери
		if ((s_re_data0[GABARITY] > 125) && (s_re_data0[STATE_DOOR] == 1))
			ActivateSound(sound_gabarity);
		return;
	}
	DeactivateSound(sound_gabarity);
	
	//Обработка звукового сигнала аварийного давления масла	
	if ((INPUT_ERROR_OIL == 0) && (rpm >= 500))
	{
		if (time_pressure_error < 10)
			time_pressure_error++;
		else
			ActivateSound(sound_oil_pressure);
	}
	else
	{
		DeactivateSound(sound_oil_pressure);
		time_pressure_error = 0;
	}
	//******************************************************
	
	//Обработка звука перегрева двигателя
	 if (INPUT_PEREGREV == 0)
	 	ActivateSound(sound_peregrev);
	 else
	 	DeactivateSound(sound_peregrev);
	//******************************************************
	
	//Обработка звука стояночного тормоза и включение сигнализатора
	 if (INPUT_PARKING_BRAKE == 0)
	 {
	 	if (s_re_data0[SPEED] > 3.0) 
	 		ActivateSound(sound_parking_brake);
	 }
	 else
	 {
	 	DeactivateSound(sound_parking_brake);
	 }
	//******************************************************
	
	//обработка звукового сигнала ремни безопасности
	if ((INPUN_REMNI_BEZOP == 0) && (s_re_data0[SPEED] > 3.0))
	 	ActivateSound(sound_remni);
	 else
	 	DeactivateSound(sound_remni);
	//******************************************************
	
	//обработка сигнала низкого уровня топлива
	if (s_re_data0[STATE_FUEL] == 1)
	 	ActivateSound(sound_low_fuel);
	 else
	 	DeactivateSound(sound_low_fuel);	
	//******************************************************
	
	//обработка сигнала открытой двери
	if ((s_re_data0[STATE_DOOR] == 1)  && (s_re_data0[SPEED] > 3.0))
	 	ActivateSound(sound_open_door);
	 else
	 	DeactivateSound(sound_open_door);
	//******************************************************
	
	//обработка сигнала неисправности рабочего тормоза
	if ((s_re_data0[RAB_TORMOZ] == 1) || (INPUN_EBD == 1))
	 	ActivateSound(sound_brake_EBD);
	 else
	 	DeactivateSound(sound_brake_EBD);
	//******************************************************
}

static void Timer_250ms(void)
{
	static debouns_counter = 0;
	
	if (test_mode)
	{
		return;
	}
	
	if (stateEngine == 0) 
	{
		//Если включены габариты и открыта дверь
		if (!(((s_re_data0[GABARITY] > 125)) && (s_re_data0[STATE_DOOR] == 1)))
		{
			debouns_counter = 0;
		}
		else
		{
			if (debouns_counter >= 16)
				UpdateSound();
			else
				debouns_counter++;
		}
	}
	else
	{
		if (debouns_counter >= 16)
			UpdateSound();
		else
			debouns_counter++;
	}
}

void ButtonCallback(uint16_t u16ButtonId, en_button_state_t enState)
{
	char result = 0;
	unsigned char temp_min = 0;
	unsigned char temp_sec = 0;
	
	if (test_mode)
		return;
		
	if (enState == StateLow)
	{
		return;
	}
    
	if (enState == StateHigh)
	{
		switch (u16ButtonId)
		{
			case BUTTON_ID_B1:
				if (m_enGuiMode == Empty)
					return;
				if (m_enGuiMode < mode_Pressure2)
					m_enGuiMode++;
				else
					m_enGuiMode = mode_Time;
				InitMode(m_enGuiMode);
				if (pressure_error)
					pressure_error = 0;
				if (pressure2_error)
					pressure2_error = 0;
				if (voltage_error)
					voltage_error = 0;
				timer_voltage_error = 250;
				timer_pressure_error = 250;
				timer_pressure2_error = 250;
				break;
				
			case BUTTON_ID_B2:
				switch (m_enGuiMode)
				{
					case mode_Time:
					case Empty:
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
			
			case BUTTON_ID_B3:
				switch (m_enGuiMode)
				{
					case mode_Time:
					case Empty:
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
					case Empty:
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
				
			case BUTTON_ID_B3:
				switch (m_enGuiMode)
				{
					case mode_Time:
					case Empty:
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
        
			default:
				break;
		}
    }	
}

static void UpdateTime(void)
{
	unsigned char hour = 0;
	unsigned char min  = 0;
	
	SIMBOL_TIME = blink;
	
	segment1(WTHR / 10);
	segment2(WTHR % 10);
	segment3(WTMR / 10);
	segment4(WTMR % 10);	
}

static void CalcVoltage(void)
{
	static unsigned time_update = 25;
	static float tempVoltage = 0.0;
	
	if (time_update)
	{
		time_update--;
		tempVoltage += 0.09083*s_re_data0[0] - 0.25631;
	}
	else
	{
		if (tempVoltage < 0.0)
			tempVoltage = 0;
		Voltage = tempVoltage / 25; 
		tempVoltage = 0;
		time_update = 25;
	}	
}

static void CalcRPM(void)
{
	float frequency = 0.0;
	
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
	if (value != 0)
	{
		frequency = 16000000.0/(float)(value/2);
		rpm = frequency * 30; 
	}
	else
		rpm = 0;
}

static void CalcPressure(void)
{
	static unsigned time_update = 25;
	static float tempPressure = 0.0;
	static float tempPressure2 = 0.0;
	
	if (time_update)
	{
		time_update--;
		tempPressure +=  -0.016820*adc_value[ADC_OIL_PRESS_6] + 7.067905;
		tempPressure2 += -0.028945*adc_value[ADC_OIL_PRESS_10] + 12.043255;
	}
	else
	{
		if (tempPressure > -170.0)	   // значение при сопротивлении датчика больше 2 кОм
			sens_pressure_activ = 1;
		else
			sens_pressure_activ = 0;
			
		if (tempPressure2 > -290.0)	
			sens_pressure2_activ = 1;
		else
			sens_pressure2_activ = 0;	
		
		if (tempPressure < 0.0)
			tempPressure = 0;
		if (tempPressure2 < 0.0)
			tempPressure2 = 0;	
			
		Pressure = tempPressure / 25; 
		Pressure2 = tempPressure2 / 25; 		
		tempPressure = 0;
		tempPressure2 = 0;
		time_update = 25;
	}
}

static void UpdatePressure(void)
{
	if ((!pressure_error) || blink)
	{
		segment1(11);		//P
		ENABLE_POINT = 1;
		NumToTopStr2((unsigned long)(Pressure*10.0));
	}
	else
	{
		ENABLE_POINT = 0;
		ClearTopLine();
	}
}


static void UpdatePressure2(void)
{
	if ((!pressure2_error) || blink)
	{
		segment1(11);		//P
		ENABLE_POINT = 1;
		NumToTopStr2((unsigned long)(Pressure2*10.0));
		//NumToTopStr((unsigned long)adc_value[ADC_OIL_PRESS_10]);
	}
	else
	{
		ENABLE_POINT = 0;
		ClearTopLine();
	}
}

static void UpdateVoltage(void)
{
	if ((!voltage_error) || blink)
	{
		SIMBOL_V = 1;
		ENABLE_POINT = 1;
		NumToTopStr3((unsigned long)((Voltage + 0.1)*10.0));
	}
	else
	{
		SIMBOL_V = 0;
		ENABLE_POINT = 0;
		ClearTopLine();
	}
}

static void StartTestMode(void)
{
	test_mode = 1;
	time_test = 440; //20мс*440=8.8сек
	EDC = 1;
	PARKING_BRAKE = 1;
	ERROR_OIL_PRESSURE = 1;
	PEREGREV = 1;
	EFU = 1;
	KMPSUD = 1;
	NumToTopStr(8888);
	Enable_simbols();
	SmcTestParams();
	SMCpos[SMC_RPM].smc_inp = 124000;
//	SMCpos[SMC_COOLANT].smc_inp = 48960;
	SMCpos[SMC_COOLANT].smc_inp = 52000;
}
static void StopTestMode(void)
{
	test_mode = 0;
	time_test = 0;
	EDC = 0;
	PARKING_BRAKE = 0;
	ERROR_OIL_PRESSURE = 0;
	PEREGREV = 0;
	EFU = 0;
	KMPSUD = 0;
	ClearTopLine();
	Disable_simbols();
	SMCpos[SMC_COOLANT].smc_inp = 0;
	SMCpos[SMC_RPM].smc_inp = 0;
	Timer_Wait(TIMER_TIMEOUT, 500, TRUE);
	SmcNormalParams();
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
