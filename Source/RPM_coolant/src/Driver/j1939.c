#include "timer.h"
#include "J1939.h"

#define TIMER_INTERVAL_UPDATE   10

stc_can_cantrol_t J1939CtrBufer[CAN_COUNT_PARAM];

static J1939_INFO J1939_Params[CAN_COUNT_PARAM] =
{
//   ID          mask        time upd.
	{0x0CFE6C00, 0x00FFFF00, 100},
	{0x0CFE6C00, 0x00FFFF00, 100},
};

static void J1939_Update();

void J1939_init(void)
{
	int i;
	CAN_Init();
	
	for (i = 0; i < CAN_COUNT_PARAM; i++)
	{
		J1939CtrBufer[i].CanMask =      J1939_Params[i].mask;   //Маска
		J1939CtrBufer[i].CanId.ulID =   J1939_Params[i].ID;     //Идентификатор
	    J1939CtrBufer[i].CanId.bExtID = true;                   //Длина идентификатора 29 бит
	    J1939CtrBufer[i].Available=     false;                  //Доступность значения
	    J1939CtrBufer[i].TimeUpdate =   (J1939_Params[i].timeUpdate/TIMER_INTERVAL_UPDATE) * 4;   //Время обновления
	    CAN_SetupReceiver(i, &J1939CtrBufer[i].CanId, J1939CtrBufer[i].CanMask, &J1939CtrBufer[i].CanMessage);
	}
	Timer_Start(TIMER_ID_J1939, TIMER_INTERVAL_UPDATE, TRUE, J1939_Update);
}

//Функция обновления значения параметров
static void J1939_Update()
{
	int i;
	
	for (i = 0; i < CAN_COUNT_PARAM; i++)
	{
		if (J1939CtrBufer[i].CanMessage.bNewRxData == true)
		{
			J1939CtrBufer[i].CanMessage.bNewRxData = false;
			J1939CtrBufer[i].CounterPasses = 0;	
			J1939CtrBufer[i].Available = true;
					
		}
		else
		{
			J1939CtrBufer[i].CounterPasses++;
			if (J1939CtrBufer[i].CounterPasses > J1939CtrBufer[i].TimeUpdate)
			{
				J1939CtrBufer[i].Available = false;
			}		
		}			
	}
			
}
