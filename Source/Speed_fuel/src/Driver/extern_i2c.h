#ifndef __I2C_H
#define __I2C_H

#define TX_BUF_I2C      20
#define RX_BUF_I2C      20

extern unsigned char m_tr_data0[TX_BUF_I2C];	/* master transmission data area */
extern unsigned char m_re_data0[RX_BUF_I2C];	/* master reception data area */
extern unsigned int  m_tr_num0;		            /* master transmission data counter */
extern unsigned int  m_re_num0;		            /* master reception data counter */
extern unsigned char s_tr_data0[RX_BUF_I2C];	/* slave transmission data area */
extern unsigned char s_re_data0[TX_BUF_I2C];	/* slave reception data area */
extern unsigned int  s_tr_num0;		            /* slave transmission data counter */
extern unsigned int  s_re_num0;		            /* slave reception data counter */

extern void init_setting_i2c(void);
extern void send_data_i2c(void);
extern void receive_data_i2c(void);
extern __interrupt void i2c_0_master_int(void);
extern __interrupt void i2c_0_slave_int(void);

#endif // __I2C_H
