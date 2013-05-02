
/**    THIS SAMPLE CODE IS PROVIDED AS IS. FUJITSU MICROELECTRONICS     **/
/** ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR OMMISSIONS **/
/*****************************************************************************
 *  Date   :        2011/02/07
 *  PROJECT:        I2C_slave
*****************************************************************************/

#include "_ffmc16.h"
#include "extern_i2c.h"


void init_i2c_0_slave(void)
{
//	IO_ICR.word = 0x6001;			/* i2c 0 (96) "level 1" */
	
	IO_IIC0.ITBA0.word = 0x0000;
	IO_IIC0.ITMK0.word = 0x3FFF;

	IO_IIC0.ISBA0.byte = 0x60;				/* slave address set(1100000B) */
	IO_IIC0.ISMK0.bit.ENSB = 1;				/* seven slave address enable */
	IO_IIC0.ICCR0.bit.EN = 1;				/* I2C enable */
	IO_IIC0.IBCR0.byte = 0x4a;				/* BEIEbit set, slave mode */
}

void i2c_0_slave(void)
{
	if(IO_IIC0.IBSR0.bit.TRX == 0)			/* slave receive mode ? */
	{
		if(IO_IIC0.IBSR0.bit.ADT == 1)		/* first Byte ? */
		{
			IO_IIC0.IBCR0.bit.INT = 0;		/* Bus free */
		}
		else
		{ 
			s_re_data0[s_re_num0] = IO_IIC0.IDAR0;	/* reception data read */
			s_re_num0++;				/* memory access counter up */
			if(s_re_num0 == 0x100)		/* 100hByte reception finish ? */
			{
				s_re_num0 = 0;		/* counter reset */
			}
			
			IO_IIC0.IBCR0.bit.INT = 0;		/* Bus free */
		}
	}
	else
	{
		IO_IIC0.IBCR0.bit.ACK = 0;		/* no ack */
		IO_IIC0.IDAR0 = s_tr_data0[s_tr_num0];	/* transmission data set */
		s_tr_num0++;				/* memory access counter up */
		if(s_tr_num0 == 0x100)			/* 100hByte transmission finish ? */
		{
			s_tr_num0 = 0;			/* counter reset */
		}
		IO_IIC0.IBCR0.bit.INT = 0;		/* transfer start */
	}
}


__interrupt void	i2c_0_slave_int(void)
{
	if(IO_IIC0.IBCR0.bit.BER == 1)			/* Bus error ? */
	{
	
		IO_IIC0.IBCR0.bit.BER = 0;		/* BERbit clear */
	}
	else
	{
		if(IO_IIC0.IBSR0.bit.LRB == 1)		/* ACK=High ? at slave transfer mode */
		{
			IO_IIC0.IBCR0.bit.INT = 0;	/* Bus free */
		}
		else
		{
			i2c_0_slave();
		}
	}
}



