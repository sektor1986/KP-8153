
/**    THIS SAMPLE CODE IS PROVIDED AS IS. FUJITSU MICROELECTRONICS     **/
/** ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR OMMISSIONS **/
/*****************************************************************************
 *  Date   :        2011/02/07
 *  PROJECT:        I2C_slave
*****************************************************************************/

#include "mcu.h"
#include "extern_i2c.h"

unsigned char s_tr_data0[RX_BUF_I2C];	/* slave transmission data area */
unsigned char s_re_data0[TX_BUF_I2C];	/* slave reception data area */
unsigned int  s_tr_num0;		            /* slave transmission data counter */
unsigned int  s_re_num0;		            /* slave reception data counter */

void init_i2c_0_slave(void);

void receive_data_i2c(void)
{
	while(IIC0_ICCR0_EN == 1)
	{
		__wait_nop();
	}
	
	init_i2c_0_slave();
}

void init_i2c_0_slave(void)
{
	ICR = 0x6001;			/* i2c 0 (96) "level 1" */
	
	IIC0_ITBA0 = 0x0000;
	IIC0_ITMK0 = 0x3FFF;

	IIC0_ISBA0 = 0x60;				/* slave address set(1100000B) */
	IIC0_ISMK0_ENSB = 1;				/* seven slave address enable */
	IIC0_ICCR0_EN = 1;				/* I2C enable */
	IIC0_IBCR0 = 0x4a;				/* BEIEbit set, slave mode */
}

void i2c_0_slave(void)
{
	if(IIC0_IBSR0_TRX == 0)			/* slave receive mode ? */
	{
		if(IIC0_IBSR0_ADT == 1)		/* first Byte ? */
		{
			IIC0_IBCR0_INT = 0;		/* Bus free */
		}
		else
		{ 
			s_re_data0[s_re_num0] = IIC0_IDAR0;	/* reception data read */
			s_re_num0++;				/* memory access counter up */
			if(s_re_num0 == TX_BUF_I2C)		/* TX_BUF_I2C reception finish ? */
			{
				s_re_num0 = 0;		/* counter reset */
			}
			
			IIC0_IBCR0_INT = 0;		/* Bus free */
		}
	}
	else
	{
		IIC0_IBCR0_ACK = 0;		/* no ack */
		IIC0_IDAR0 = s_tr_data0[s_tr_num0];	/* transmission data set */
		s_tr_num0++;				/* memory access counter up */
		if(s_tr_num0 == RX_BUF_I2C)			/* RX_BUF_I2C transmission finish ? */
		{
			s_tr_num0 = 0;			/* counter reset */
		}
		IIC0_IBCR0_INT = 0;		/* transfer start */
	}
}


__interrupt void i2c_0_slave_int(void)
{
	if (IIC0_IBCR0_BER == 1)			/* Bus error ? */
	{
	
		IIC0_IBCR0_BER = 0;		/* BERbit clear */
	}
	else
	{
		if (IIC0_IBSR0_LRB == 1)		/* ACK=High ? at slave transfer mode */
		{
			IIC0_IBCR0_INT = 0;	/* Bus free */
		}
		else
		{
			i2c_0_slave();
		}
	}
}



