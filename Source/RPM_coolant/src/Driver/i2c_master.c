
/**    THIS SAMPLE CODE IS PROVIDED AS IS. FUJITSU MICROELECTRONICS     **/
/** ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR OMMISSIONS **/
/*****************************************************************************
 *  Date   :        2011/02/07
 *  PROJECT:        I2C_master
*****************************************************************************/

#include "mcu.h"
#include "extern_i2c.h"



unsigned char	m_tr_data0[TX_BUF_I2C];	/* master transmission data area */
unsigned char	m_re_data0[RX_BUF_I2C];	/* master reception data area */
unsigned int	m_tr_num0;		/* master transmission data counter */
unsigned int	m_re_num0;		/* master reception data counter */

void init_i2c_0_master(void);

void init_setting_i2c(void)
{
	for(m_tr_num0 = 0; m_tr_num0 < RX_BUF_I2C; m_tr_num0++)	/* 100hByte data set */
	{
		m_tr_data0[m_tr_num0] = 0;		/* 00h`FFh data set */
	}
	for(s_tr_num0 = 0; s_tr_num0 < RX_BUF_I2C; s_tr_num0++)	/* 100hByte data set */
	{
		s_tr_data0[s_tr_num0] = 0;		/* 00h`FFh data set */
	}
	m_tr_num0=0;
	s_tr_num0=0;
	m_re_num0=0;
	s_re_num0=0;
	
}

void send_data_i2c(void)
{
	while(IIC0_ICCR0_EN == 1)
	{
		__wait_nop();
	}
	
	init_i2c_0_master();
}

void init_i2c_0_master(void)
{
	ICR = 0x6001;			/* i2c 0 (96) "level 1" */

	IIC0_ITBA0 = 0x0000;
	IIC0_ITMK0 = 0x3FFF;
	IIC0_ISBA0 = 0x60;				/* slave address set(1100000B) */
	IIC0_ISMK0 = 0x00;				/* slave address mask off */
	IIC0_IBCR0_BEIE = 1;				/* BEIEbit set */
	IIC0_ICCR0 = 0x0C;				/* 16MHz/(12*12+16)=100kbps */
	IIC0_ICCR0_EN = 1;				/* I2C enable */

//	if(PDR01_P0 == 0)				/* P01_0=Low master-transmitter */
	{
		IIC0_IDAR0 = 0xc0;				/* transfer slave address set */
		if (IIC0_IBSR0_BB == 1)			/* Bus busy ? */
		{
			__wait_nop();				/* wait */
		}
		else
		{
			IIC0_IBCR0 = 0x12;		/* start condition transfer */
		}
		if (IIC0_IBSR0_BB == 0 && IIC0_IBSR0_AL == 1)/* BBbit=0 and ALbit=1 ? */
		{
			IIC0_ICCR0_EN = 0;		/* I2C disable */
			init_i2c_0_master();			/* retry */
		}
	}

//	else							/* P01_0=High master-receiver */
//	{
//		IIC0_IDAR0 = 0xc1;				/* transfer slave address set */
//		if(IIC0_IBSR0_BB == 1)			/* Bus busy ? */
//		{
//			__wait_nop();				/* wait */
//		}
//		else
//		{
//		IIC0_IBCR0 = 0x12;			/* start condition transfer */
//		}
//		if (IIC0_IBSR0_BB == 0 && IIC0_IBSR0_AL == 1)/* BBbit=0 and ALbit=1 ? */
//		{
//			__wait_nop();
//			__wait_nop();
//			IIC0_ICCR0_EN = 0;		/* I2C disable */
//			init_i2c_0_master();			/* retry */
//		}
//	}

}

__interrupt void i2c_0_master_int(void)
{
	if (IIC0_IBCR0_BER == 1)				/* Bus error ? */
	{
		IIC0_IBCR0_BER = 0;			/* BERbit clear */
	}
	else if (IIC0_IBSR0_AL == 1)			/* AL ? */
	{
		IIC0_IBCR0_ACK = 1;			/* ACK enable */
		if(IIC0_IBSR0_AAS == 1)			/* addressing ? */
		{
			i2c_0_slave();
		}
		else
		{
			IIC0_IBCR0 = 0x00;		/* stop condition transfer */
		}
	}
	else if (IIC0_IBSR0_LRB == 1)			/* ack = High H */
	{
		IIC0_IBCR0 = 0x00;			/* stop condition transfer */
	}
	else if (IIC0_IBCR0_MSS == 0)			/* slave mode */
	{
		i2c_0_slave();
	}
	else	
	{
		if (IIC0_IBSR0_TRX == 1)			/* master-transmitter ? */
		{
			if(m_tr_num0 == TX_BUF_I2C)
			{
				IIC0_IBCR0 = 0x00;	/* stop condition transfer */
			}
			IIC0_IDAR0 = m_tr_data0[m_tr_num0];	/* transmission data set */
			if(m_tr_num0 == TX_BUF_I2C)
				m_tr_num0;
			else
				m_tr_num0++;				/* memory access counter up */
			IIC0_IBCR0_INT = 0;		/* transfer start */
			__wait_nop();
		}
		else						/* master-receiver */
		{
			IIC0_IBCR0_ACK = 1;		/* ACK enable */
			if (IIC0_IBSR0_ADT == 1)		/* first byte transfer ? */
			{
				IIC0_IBCR0_INT = 0;	/* Bus clear */
			}
			else
			{
				m_re_data0[m_re_num0] = IIC0_IDAR0;	/* reception data read */
				m_re_num0++;		/* memory access counter up */
				if (m_re_num0 == RX_BUF_I2C)
				{
					IIC0_IBCR0_ACK = 0;	/* ACK disable */
					IIC0_IBCR0_INT = 0;	/* Bus free */
					m_re_num0 = 0;
				}
				else
				{
					IIC0_IBCR0_INT = 0;	/* Bus free */
				}
			}
		}
	}
}


