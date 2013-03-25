#ifndef __RTC_H
#define __RTC_H

#define WTMR        WT_WTMR
#define WTHR        WT_WTHR
#define WTCR_UPDT   WT_WTCR_UPDT 
#define WTBR1       WT_WTBR1
#define WTBR0       WT_WTBR_ML
#define WTCR_INTE0  WT_WTCR_INTE0
#define WTCR_INTE1  WT_WTCR_INTE1
#define WTCR_INTE2  WT_WTCR_INTE2
#define WTCR_INTE3  WT_WTCR_INTE3
#define WTCER_INTE4 WT_WTCER_INTE4
#define WTCR_OE     WT_WTCR_OE
#define WTCKSR      WT_WTCKSR
#define WTSR        WT_WTSR
#define WTCR_ST     WT_WTCR_ST

void InitRTC (void);

#endif // __RTC_H
