
extern unsigned char	*m_tr_data0;	/* master transmission data area */
extern unsigned char	*m_tr_data1;	/* master transmission data area */
extern unsigned char	*m_re_data0;	/* master reception data area */
extern unsigned char	*m_re_data1;	/* master reception data area */
extern unsigned int	m_tr_num0;		/* master transmission data counter */
extern unsigned int	m_tr_num1;		/* master transmission data counter */
extern unsigned int	m_re_num0;		/* master reception data counter */
extern unsigned int	m_re_num1;		/* master reception data counter */
extern unsigned char	*s_tr_data0;	/* slave transmission data area */
extern unsigned char	*s_tr_data1;	/* slave transmission data area */
extern unsigned char	*s_re_data0;	/* slave reception data area */
extern unsigned char	*s_re_data1;	/* slave reception data area */
extern unsigned int	s_tr_num0;		/* slave transmission data counter */
extern unsigned int	s_tr_num1;		/* slave transmission data counter */
extern unsigned int	s_re_num0;		/* slave reception data counter */
extern unsigned int	s_re_num1;		/* slave reception data counter */
extern	__interrupt void i2c_0_slave_int(void);
extern	__interrupt void i2c_0_master_int(void);
