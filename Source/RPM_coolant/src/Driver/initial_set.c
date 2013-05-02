
#include "_ffmc16.h"
#include "extern_i2c.h"

void initial_set(void)
{
//	m_tr_data0 =(unsigned char *)0x7300;	/* Adress 7300H`73FFH */
//	m_re_data0 =(unsigned char *)0x7500;	/* Adress 7500H`75FFH */
//	s_tr_data0 =(unsigned char *)0x7700;	/* Adress 7700H`77FFH */
//	s_re_data0 =(unsigned char *)0x7900;	/* Adress 7900H`79FFH */

	for(m_tr_num0=0; m_tr_num0<=0x100; m_tr_num0++)	/* 100hByte data set */
	{
		m_tr_data0[m_tr_num0] = 0;		/* 00h`FFh data set */
	}
	for(s_tr_num0=0; s_tr_num0<=0x100; s_tr_num0++)	/* 100hByte data set */
	{
		s_tr_data0[s_tr_num0] = 0;		/* 00h`FFh data set */
	}
	m_tr_num0=0;
	s_tr_num0=0;
	m_re_num0=0;
	s_re_num0=0;
}

void en_check0(void)
{
	while(IO_IIC0.ICCR0.bit.EN == 1)
	{
		__wait_nop();
	}
	init_i2c_0_slave();			/* i2c_slave  0ch set */
//	init_i2c_0_master();			/* i2c_master 0ch set */
}


