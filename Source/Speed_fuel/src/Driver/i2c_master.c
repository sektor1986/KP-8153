
/**    THIS SAMPLE CODE IS PROVIDED AS IS. FUJITSU MICROELECTRONICS     **/
/** ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR OMMISSIONS **/
/*****************************************************************************
 *  Date   :        2011/02/07
 *  PROJECT:        I2C_master
*****************************************************************************/

#include "_ffmc16.h"
#include "extern_i2c.h"

void init_i2c_0_master(void)
{
//	IO_ICR.word = 0x6001;			/* i2c 0 (96) "level 1" */

	IO_IIC0.ITBA0.word = 0x0000;
	IO_IIC0.ITMK0.word = 0x3FFF;
	IO_IIC0.ISBA0.byte = 0x60;				/* slave address set(1100000B) */
	IO_IIC0.ISMK0.byte = 0x00;				/* slave address mask off */
	IO_IIC0.IBCR0.bit.BEIE = 1;				/* BEIEbit set */
	IO_IIC0.ICCR0.byte = 0x0C;				/* 16MHz/(12*12+16)=100kbps */
	IO_IIC0.ICCR0.bit.EN = 1;				/* I2C enable */

	//master-transmitter
	IO_IIC0.IDAR0 = 0xc0;				/* transfer slave address set */
//	IO_IIC0.IDAR0 = 0x60;
	if (IO_IIC0.IBSR0.bit.BB == 1)			/* Bus busy ? */
		__wait_nop();				/* wait */
	else
		IO_IIC0.IBCR0.byte = 0x12;		/* start condition transfer */
//	if (IO_IIC0.IBSR0.bit.BB == 0 && IO_IIC0.IBSR0.bit.AL == 1)/* BBbit=0 and ALbit=1 ? */
//	{
//		IO_IIC0.ICCR0.bit.EN = 0;		/* I2C disable */
//		init_i2c_0_master();			/* retry */
//	}
	
	// master-receiver
//	IO_IIC0.IDAR0 = 0xc1;				/* transfer slave address set */
//	if(IO_IIC0.IBSR0.bit.BB == 1)			/* Bus busy ? */
//		__wait_nop();				/* wait */
//	else
//		IO_IIC0.IBCR0.byte = 0x12;			/* start condition transfer */
//	if (IO_IIC0.IBSR0.bit.BB == 0 && IO_IIC0.IBSR0.bit.AL == 1)/* BBbit=0 and ALbit=1 ? */
//	{
//		__wait_nop();
//		__wait_nop();
//		IO_IIC0.ICCR0.bit.EN = 0;		/* I2C disable */
//		init_i2c_0_master();			/* retry */
//	}
}

__interrupt void i2c_0_master_int(void)
	{
	if (IO_IIC0.IBCR0.bit.BER == 1)				/* Bus error ? */
		IO_IIC0.IBCR0.bit.BER = 0;			/* BERbit clear */
	else if (IO_IIC0.IBSR0.bit.AL == 1)			/* AL ? */
	{	
		IO_IIC0.IBCR0.bit.ACK = 1;			/* ACK enable */
		if (IO_IIC0.IBSR0.bit.AAS == 1)			/* addressing ? */
			i2c_0_slave();
		else
			IO_IIC0.IBCR0.byte=0x00;		/* stop condition transfer */
	}
	else if (IO_IIC0.IBSR0.bit.LRB == 1)			/* ack = High ÅH */
		IO_IIC0.IBCR0.byte=0x00;			/* stop condition transfer */
	else if (IO_IIC0.IBCR0.bit.MSS == 0)			/* slave mode */
		i2c_0_slave();
	else	
	{
		if(IO_IIC0.IBSR0.bit.TRX == 1)			/* master-transmitter ? */
		{
			if(m_tr_num0 == 0x100)
			{
				m_tr_num0 = 0;
				IO_IIC0.IBCR0.byte=0x00;	/* stop condition transfer */
				IO_IIC0.ICCR0.bit.EN = 0;
				return;
			}
			IO_IIC0.IDAR0 = m_tr_data0[m_tr_num0];	/* transmission data set */
			m_tr_num0++;				/* memory access counter up */
			IO_IIC0.IBCR0.bit.INT = 0;		/* transfer start */
			__wait_nop();
		}
		else						/* master-receiver */
		{
			IO_IIC0.IBCR0.bit.ACK = 1;		/* ACK enable */
			if (IO_IIC0.IBSR0.bit.ADT == 1)		/* first byte transfer ? */
				IO_IIC0.IBCR0.bit.INT = 0;	/* Bus clear */
			else
			{
				m_re_data0[m_re_num0] = IO_IIC0.IDAR0;	/* reception data read */
				if (m_re_num0 == 0xff)
				{
					IO_IIC0.IBCR0.bit.ACK = 0;	/* ACK disable */
					m_re_num0++;		/* memory access counter up */
					IO_IIC0.IBCR0.bit.INT = 0;	/* Bus free */
				}
				else
				{
					m_re_num0++;		/* memory access counter up */
					IO_IIC0.IBCR0.bit.INT = 0;	/* Bus free */
				}
			}
		}
	}
}
	
	

