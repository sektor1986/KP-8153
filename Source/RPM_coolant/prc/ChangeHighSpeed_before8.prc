#====================================================================
#  F2MC-16FX Family Template Project V01L01
#  ALL RIGHTS RESERVED, COPYRIGHT (C) FUJITSU SEMICONDUCTOR LIMITED 2012
#  LICENSED MATERIAL - PROGRAM PROPERTY OF FUJITSU SEMICONDUCTOR LIMITED
#====================================================================
set var FREQ_4MHZ=D'4000000L # set      FREQ_4MHZ       D'4000000L
set var FREQ_8MHZ=D'8000000L # set      FREQ_8MHZ       D'8000000L
set var CRYSTAL=%FREQ_8MHZ   # set      CRYSTAL         FREQ_8MHZ  ; <<< select external crystal frequency

set var CPU_RC_CLKP1_RC_CLKP2_RC=0x0                # set CPU_RC_CLKP1_RC_CLKP2_RC                   0x0  ; will not modify initial clock settings
set var CPU_MAINOSC_CLKP1_MAINOSC_CLKP2_MAINOSC=0x1 # set CPU_MAINOSC_CLKP1_MAINOSC_CLKP2_MAINOSC    0x1
set var CPU_16MHZ_CLKP1_16MHZ_CLKP2_16MHZ=0x2       # set CPU_16MHZ_CLKP1_16MHZ_CLKP2_16MHZ          0x2
set var CPU_32MHZ_CLKP1_16MHZ_CLKP2_16MHZ=0x3       # set CPU_32MHZ_CLKP1_16MHZ_CLKP2_16MHZ          0x3
set var CPU_32MHZ_CLKP1_32MHZ_CLKP2_16MHZ=0x4       # set CPU_32MHZ_CLKP1_32MHZ_CLKP2_16MHZ          0x4
set var CLOCK_SPEED=%CPU_32MHZ_CLKP1_16MHZ_CLKP2_16MHZ  #; <<< set clock speeds #set      CLOCK_SPEED     CPU_32MHZ_CLKP1_16MHZ_CLKP2_16MHZ  ; <<< set clock speeds

set var CKSR=0x0401
set var CKSSR=0x0402
set var CKFCR=0x0404
set var PLLCR=0x0406
set var MC_STAB_TIME=0x4

CANCEL FREQUENCY /MAX			# disable high speed mode
reset

#;====================================================================
#; 6.4   Set clock ratio (ignore subclock)
#;====================================================================
#;#0: CPU_RC_CLKP1_RC_CLKP2_RC
#;#1: CPU_MAINOSC_CLKP1_MAINOSC_CLKP2_MAINOSC
#;#2: CPU_16MHZ_CLKP1_16MHZ_CLKP2_16MHZ
#;#3: CPU_32MHZ_CLKP1_16MHZ_CLKP2_16MHZ
#;#4: CPU_32MHZ_CLKP1_32MHZ_CLKP2_16MHZ
#;
#;+Main-+     +-----+          [PLLCR]        #1        #2        #3        #4
#;| OSC +--+--+ PLL +---CLKPLL   4M                 16M(div4) 32M(div8) 32M(div8) VCO=64M
#;+-----+  |  +-----+     |      8M                 16M(div2) 32M(div4) 32M(div4) VCO=64M
#;         |              |
#;         |              |     [CKFCR]       #1        #2        #3        #4
#;         +----CLKMC-----x---+--CLKB   4M|8M(div1) 16M(div1) 32M(div1) 32M(div1)
#;                            +--CLKP1  4M|8M(div1) 16M(div1) 16M(div2) 32M(div1)
#;                            +--CLKP2  4M|8M(div1) 16M(div1) 16M(div2) 16M(div2)
#;
#;       #1       [CKSR] CLKMC(=0xB5) ;SCE="1",PCE="0",MCE="1",RCE="1",SC2S="01", SC1S="01"
#;       #2/#3/#4 [CKSR] CLKPLL(=0xFA);SCE="1",PCE="1",MCE="1",RCE="1",SC2S="10", SC1S="10"

  SET MEM/BYTE %CKSSR=H'38|%MC_STAB_TIME

    SET MEM/WORD %PLLCR=H'0003
    SET MEM/WORD %CKFCR=H'1101

  SET MEM/BYTE %CKSR=H'FA

SET FREQUENCY /MAX 64		# enable high speed mode
SHOW SYSTEM
