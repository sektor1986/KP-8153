
#include "_ffmc16.h"
#include "extern_i2c.h"

void initial_set(void)
{
	m_tr_data0 =(unsigned char *)0x7300;	/* Adress 7300HÅ`73FFH */
	m_re_data0 =(unsigned char *)0x7500;	/* Adress 7500HÅ`75FFH */
	s_tr_data0 =(unsigned char *)0x7700;	/* Adress 7700HÅ`77FFH */
	s_re_data0 =(unsigned char *)0x7900;	/* Adress 7900HÅ`79FFH */

	IO_PIER01.byte = 0x01;
	IO_PDR01.byte = 0x00;				/* PDR1 data reset. */
	IO_DDR01.byte = 0x00;				/* DDR1 is set INPUT function. */

	for(m_tr_num0=0; m_tr_num0<=0x100; m_tr_num0++)	/* 100hByte data set */
	{
		m_tr_data0[m_tr_num0] = 0;		/* 00hÅ`FFh data set */
	}
	for(s_tr_num0=0; s_tr_num0<=0x100; s_tr_num0++)	/* 100hByte data set */
	{
		s_tr_data0[s_tr_num0] = 0;		/* 00hÅ`FFh data set */
	}
	m_tr_num0=0;
	s_tr_num0=0;
	m_re_num0=0;
	s_re_num0=0;
	
}

void SendDataI2C(void)
{		
	while(IO_IIC0.ICCR0.bit.EN == 1)
	{
		__wait_nop();
	}

//	init_i2c_0_slave();			/* i2c_slave  0ch set */
	init_i2c_0_master();			/* i2c_master 0ch set */
}


