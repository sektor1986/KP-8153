/* THIS SAMPLE CODE IS PROVIDED AS IS AND IS SUBJECT TO ALTERATIONS. FUJITSU */
/* MICROELECTRONICS ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR */
/* ELIGIBILITY FOR ANY PURPOSES.                                             */
/*                 (C) Fujitsu Microelectronics Europe GmbH                  */
/* Handles all the sequences to work with the Flash memory in the CPU mode   */

#include "mcu.h"
#include "flash.h"

//#pragma section FAR_CODE=RAMCODE	// located the program into RAM
//#pragma section CODE=RAMCODE

//------------------------------------------------------------------------------

unsigned int Data_Flash_write(volatile __far unsigned int *wr_adr, volatile unsigned int wdata)
{
	unsigned int flag = 0; 
 
	// Enable Command Sequencer 
	DFCA = 0x03; 
 
	// clear status flags 
	DFSA = 0x00; 
	
	*wr_adr  = wdata;    // send data to the pointed address 
 
	// Wait until write has finished, i.e. Command Sequencer is in idle state 
	while (DFSA_ST != 0)
		WDTCP_ = 0x00; 
 
	// obtain result 
	flag = (DFSA & 0xF0) >> 4; 
 
	// disable Command Sequencer 
	DFCA = 0x00; 
 
	return(flag); 
}

unsigned int Data_flash_SectorErase(volatile __far unsigned int * pu16SecAdr)
{
	unsigned int u16Flag = 0;
	
	DFCA_WE = 1;
	// start with flash sector erase sequence
	*seq_AAAA = 0x00AA;     // sends erase command to the pointed address
	*seq_5554 = 0x0055;
	*seq_AAAA = 0x0080;
	*seq_AAAA = 0x00AA;
	*seq_5554 = 0x0055;
	*pu16SecAdr = 0x0030;

	while ((*pu16SecAdr & DQ3) != DQ3);    // sector erase timer ready?
	while(0 == u16Flag)
	{
		if(DQ7 == (*pu16SecAdr & DQ7))     // data polling
		{
			u16Flag = 1;                    // successful erased!
		}
    
		if(DQ5 == (*pu16SecAdr & DQ5))     // time out?
		{
			if(DQ7 == (*pu16SecAdr & DQ7)) // successful anyway?
			{
				u16Flag = 1;                // successful erased!
			}
			else
			{
				u16Flag = 2;                // time out error!
			}
		}
	}
    
	DFCA_WE = 0;
	return(u16Flag - 1);
}

//--------------------------------------------------------------------------------

