/* THIS SAMPLE CODE IS PROVIDED AS IS AND IS SUBJECT TO ALTERATIONS. FUJITSU */
/* MICROELECTRONICS ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR */
/* ELIGIBILITY FOR ANY PURPOSES.                                             */
/*                 (C) Fujitsu Microelectronics Europe GmbH                  */

/******************************* Defines *************************************/
#define seq_AAAA ((__far volatile unsigned int*)0xDF0AAA)  // sequence address 1
#define seq_5554 ((__far volatile unsigned int*)0xDF0554)  // sequence address 2	

// sector start address
#define SA2 ((__far unsigned int*)0xDF4000)  // Main Flash SA2
#define SA3 ((__far unsigned int*)0xDF6000)  // Main Flash SA3

#define DQ7 0x0080          // data polling flag
#define DQ5 0x0020          // time limit exceeding flag
#define DQ3 0x0008          // sector erase timer flag

/******************************* Prototyps ***********************************/
unsigned int Data_Flash_write(volatile __far unsigned int*, unsigned int);
unsigned int Data_flash_SectorErase(volatile __far unsigned int * pu16SecAdr);
