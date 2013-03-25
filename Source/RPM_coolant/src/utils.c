#include "utils.h"

unsigned long ToBCD(unsigned long byte)
{
	unsigned long bcd = 0;
	unsigned char count = 0, count2,i;

	if (!byte) return 0;
	
	while (!(byte&0x80000000l))
	{
		byte<<=1;
		count++;
	}
	count  = 32 - count;
	count2 = count-1;

	for (i=0; i<count; i++)
	{
		bcd<<=1;
		if (byte& 0x80000000l)
			bcd|=1;
		byte<<=1;
		if (i==count2) break;
		    // 12345678
		bcd+=0x33333333l;
					// 12345678
		if (!((bcd)& 0x00000008l))
			bcd-=0x00000003l;
		if (!((bcd)& 0x00000080l))
			bcd-=0x00000030l;
		if (!((bcd)& 0x00000800l))
			bcd-=0x00000300l;
		if (!((bcd)& 0x00008000l))
			bcd-=0x00003000l;
		if (!((bcd)& 0x00080000l))
			bcd-=0x00030000l;
		if (!((bcd)& 0x00800000l))
			bcd-=0x00300000l;
		if (!((bcd)& 0x08000000l))
			bcd-=0x03000000l;
		if (!((bcd)& 0x80000000l))
			bcd-=0x30000000l;
		
	}
	return bcd;
}
