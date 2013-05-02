#include "mcu.h"
#include "options.h"
#include "smc.h"
#include "fstdint.h"

#define PWC(o)		(((volatile unsigned char*)0x05E0)[(o) * 10])
#define PWEC(o)		(((volatile unsigned char*)0x05E1)[(o) * 10])
#define PWCA(o)		(((volatile unsigned short*)0x05E2)[(o) * 5])
#define PWCB(o)		(((volatile unsigned short*)0x05E4)[(o) * 5])
#define PWSA(o)		(((volatile unsigned char*)0x05E6)[(o) * 10])
#define PWSB(o)		(((volatile unsigned char*)0x05E7)[(o) * 10])

unsigned int time_update_smc = 0;
unsigned int time_update_speed_out = 0;

unsigned char current_smc = 0;

/*----------- Global Variables --------------------------------------*/  
stc_smc_pos_t SMCpos[SMC_COUNT];
/*-------------------------------------------------------------------*/
 
typedef struct stc_smc_control
{
    long             smc_pt1;
    long             smc_clc1;
    long             smc_old;
    long             smc_velo;
    long             smc_velo_old;
    int              smc_dn;
    int              smc_vmax;
    int              smc_amax;
} stc_smc_control_t;

static stc_smc_control_t SMCcontrol[SMC_COUNT];




/*---- sin\cos Lookup table for microstepping------------------------*/

unsigned char const SMC_TAB_CS[128]={0,  3,  6,  9, 13, 16, 19, 22,     
                                     25, 28, 31, 34, 37, 41, 44, 47,
                                     50, 53, 56, 59, 62, 65, 68, 71,
                                     74, 77, 80, 83, 86, 89, 92, 95,
                                     98,100,103,106,109,112,115,117,
                                     120,123,126,128,131,134,136,139,
                                     142,144,147,149,152,154,157,159,
                                     162,164,167,169,171,174,176,178,
                                     180,183,185,187,189,191,193,195,
                                     197,199,201,203,205,207,208,210,
                                     212,214,215,217,219,220,222,223,
                                     225,226,228,229,231,232,233,234,
                                     236,237,238,239,240,241,242,243,
                                     244,245,246,247,247,248,249,249,
                                     250,251,251,252,252,253,253,253,
                                     254,254,254,255,255,255,255,255 };

/*-------------------------------------------------------------------*/

/* Lookup tables for quadrant management*/

unsigned char const smc_quad_a[4]={0x02, 0x10, 0x10, 0x02};
unsigned char const smc_quad_b[4]={0x50, 0x50, 0x42, 0x42};
unsigned char const smc_quad_b1[4]={0x42, 0x42, 0x50, 0x50};


void InitSMC(unsigned int x) /* set up all neccesary CPU resources  */ 
{
	unsigned char i = 0;
  /* initialise cpu output port for the motor */ 
	DDR08  = 0xFF;                   /* assign pins as output      */ 
	PHDR08 = 0xFF; 
	
	for (i = 0; i < 2; i++)
	{ 
		PWEC(i) = 0x03; 
    	/* initialise low pass filters     */ 
		SMCpos[i].smc_inp=0;                    /* clear target position    */ 
		SMCcontrol[i].smc_pt1=0;                    /* clear actual position         */ 
		SMCpos[i].smc_new=0; 
		SMCcontrol[i].smc_old=0;         /* clear actual outputs        */ 
		/* initialise variables for physical limits */ 
		SMCcontrol[i].smc_dn =   6;                 /* set up damping grade       */ 
		SMCcontrol[i].smc_vmax = 100;               /* set up velocity limit          */ 
		SMCcontrol[i].smc_amax =  1;                /* set up acceleration limit      */ 
	}
    /* initialise reload timer 2   
    time_update_smc            */ 
	TMRLR2  = x;                /* set reload value in [us] 2*x    */ 
	TMCSR2  = 0x81B;            /* prescaler 2us at 32MHz         */ 
}

void ZeroPosSMC(void)
{
  	SMCpos[0].smc_inp = -125000;	
  	SMCpos[1].smc_inp = -125000;
}

void ClearPosSMC(void)
{
	unsigned char i = 0;
	for (i = 0; i < 2; i++)
	{
		SMCpos[i].smc_inp=0;                    /* clear target position    */ 
		SMCcontrol[i].smc_pt1=0;                    /* clear actual position         */ 
		SMCpos[i].smc_new=0; 
		SMCcontrol[i].smc_old=0;         /* clear actual outputs        */ 
		SMCcontrol[i].smc_dn =   8;                 /* set up damping grade       */ 
		SMCcontrol[i].smc_amax =  1;                /* set up acceleration limit      */ 
		SMCcontrol[i].smc_vmax = 100;               /* set up velocity limit          */ 
	}
}

void SmcParamsForReturn(void)
{
//	SMCcontrol[0].smc_dn =   6;                 /* set up damping grade       */ 
//	SMCcontrol[0].smc_vmax = 200;               /* set up velocity limit          */ 
//	SMCcontrol[0].smc_amax =  4;                /* set up acceleration limit      */ 
	SMCpos[0].smc_inp = SMCpos[0].smc_inp >> 1;	
//	SMCcontrol[1].smc_dn =   6;                 /* set up damping grade       */ 
//	SMCcontrol[1].smc_vmax = 200;               /* set up velocity limit          */ 
//	SMCcontrol[1].smc_amax =  4;                /* set up acceleration limit      */ 	
	SMCpos[1].smc_inp = SMCpos[1].smc_inp >> 1;	
}

void SmcNormalParams(void)
{
	SMCcontrol[0].smc_dn =   8;                 /* set up damping grade       */ 
	SMCcontrol[0].smc_amax =  1;                /* set up acceleration limit      */ 
	SMCcontrol[0].smc_vmax = 100;               /* set up velocity limit          */ 	
	SMCcontrol[1].smc_dn =   8;                 /* set up damping grade       */ 
	SMCcontrol[1].smc_amax =  1;                /* set up acceleration limit      */ 
	SMCcontrol[1].smc_vmax = 100;               /* set up velocity limit          */ 	
}

void SmcTestParams(void)
{
	SMCcontrol[SMC_COOLANT].smc_dn =   8;                 /* set up damping grade       */ 
	SMCcontrol[SMC_COOLANT].smc_amax =  1;                /* set up acceleration limit      */ 
	SMCcontrol[SMC_COOLANT].smc_vmax = 12;               /* set up velocity limit          */ 	
	SMCcontrol[SMC_RPM].smc_dn =   8;                 /* set up damping grade       */ 
	SMCcontrol[SMC_RPM].smc_amax =  1;                /* set up acceleration limit      */ 
	SMCcontrol[SMC_RPM].smc_vmax = 30;               /* set up velocity limit          */ 		
}

void smc_out(long ustp, unsigned char n) 
{ 
	int q,d,smc_a,smc_b;    /* some squeeze intermediate memories  */ 
	
	q=((ustp>>8) & 3);      /* normalise the over all granulation  to 1024 microsteps per polpair change  */ 
	d=((ustp>>1) & 127);    /* normalise the inner granulation to 512 microsteps per polpair change so that the Bit0 of ustp is don't care! */ 
	
	smc_a=SMC_TAB_CS[d];    /* preload of sin component      */ 
	smc_b=SMC_TAB_CS[127-d];/* preload of cos component note the trick with the enlarged table, which can be used in reverse order  */ 
	if ((q & 1)==1) 
	{                       /* decide where to go whatever    */ 
		PWCA(n)=smc_a;        /* set up the sin value for coil A    */ 
		PWCB(n)=smc_b;        /* set up the cos value for coil B    */ 
	}
	else 
	{                       /* otherwise change the signs    */ 
		PWCA(n)=smc_b;        /* set up the cos value for coil A    */ 
		PWCB(n)=smc_a;        /* set up the sin value for coil B    */ 
	} 
	//PWC=0xE8;              /* startover with the resource operation  */ 
	PWC(n)=0xB8;
	PWSA(n)=smc_quad_a[q];    /* arming the signal for coil A    */ 
	#if (SMC_TYPE == SMC_TYPE_VID23)
		PWSB(n)=smc_quad_b[q];    /* arming the signal for coil B    */
	#else
		PWSB(n)=smc_quad_b1[q];    /* arming the signal for coil B   */
	#endif
}

void smc__lpf(unsigned char n) 
{ /* this tiny calculation should be done in a less part of a millisecond  */  
	
	SMCcontrol[n].smc_old = SMCpos[n].smc_new;     /* yesterdays future is passed today */ 
    SMCcontrol[n].smc_velo_old = SMCcontrol[n].smc_velo;

	SMCcontrol[n].smc_clc1 = (SMCcontrol[n].smc_pt1 << SMCcontrol[n].smc_dn) - SMCcontrol[n].smc_pt1;
	SMCcontrol[n].smc_pt1 = (SMCcontrol[n].smc_clc1 + SMCpos[n].smc_inp) >> SMCcontrol[n].smc_dn;
	
	SMCcontrol[n].smc_clc1 = (SMCpos[n].smc_new << SMCcontrol[n].smc_dn) - SMCpos[n].smc_new;
	SMCpos[n].smc_new = (SMCcontrol[n].smc_clc1 + SMCcontrol[n].smc_pt1) >> SMCcontrol[n].smc_dn;

		
	SMCcontrol[n].smc_velo = SMCpos[n].smc_new - SMCcontrol[n].smc_old;
	SMCcontrol[n].smc_clc1 = SMCcontrol[n].smc_velo - SMCcontrol[n].smc_velo_old;
	
	if (SMCcontrol[n].smc_clc1 >= 0)
	{
		if (SMCcontrol[n].smc_clc1 > SMCcontrol[n].smc_amax)
		{
			SMCcontrol[n].smc_velo = SMCcontrol[n].smc_velo_old + SMCcontrol[n].smc_amax;
		}
	}
	else
	{
		if (SMCcontrol[n].smc_clc1 < (-SMCcontrol[n].smc_amax))
		{
			SMCcontrol[n].smc_velo = SMCcontrol[n].smc_velo_old - SMCcontrol[n].smc_amax;
		}		
	}
	
	if (SMCcontrol[n].smc_velo >= 0)
	{
		if (SMCcontrol[n].smc_velo > SMCcontrol[n].smc_vmax)
		{
			SMCcontrol[n].smc_velo = SMCcontrol[n].smc_vmax;
		}
	}
	else
	{
		if (SMCcontrol[n].smc_velo < (-SMCcontrol[n].smc_vmax))
		{
			SMCcontrol[n].smc_velo = -SMCcontrol[n].smc_vmax;
		}	
	}
	
	SMCpos[n].smc_new = SMCcontrol[n].smc_old + SMCcontrol[n].smc_velo;
	
}

//__interrupt void SMC_IRQ (void)
void SMC_IRQ (void)
{   /* background task for motor controlling */ 
	
	smc_out(SMCpos[current_smc].smc_new, current_smc);  /*  force the output first, for less delay glitch  */ 
	smc__lpf(current_smc);       /* calculate next cycle output value     */  
	time_update_smc = 0;
	current_smc++;
	if (current_smc > 1)
		current_smc = 0;

	TMCSR2_UF = 0;    /* reset underflow interrupt request flag    */ 
}
