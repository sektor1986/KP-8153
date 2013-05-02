#ifndef __SMC_H
#define __SMC_H

#define SMC_RPM          1
#define SMC_COOLANT      0

#define SMC_COUNT    2

typedef struct stc_smc_pos
{
	long smc_inp;
	long smc_new;
} stc_smc_pos_t;


extern stc_smc_pos_t SMCpos[SMC_COUNT];


extern long smc_inp;
extern long smc_new;

void InitSMC(unsigned int x);
void ClearPosSMC(void);
void ZeroPosSMC(void);
void SmcParamsForReturn(void);
void SmcNormalParams(void);
void SmcTestParams(void);
void SMC_IRQ (void);
//__interrupt void SMC_IRQ (void);

#endif // __SMC_H
