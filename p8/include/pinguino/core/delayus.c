/*	----------------------------------------------------------------------------
	FILE:			delayus.c
	PROJECT:		pinguino
	PURPOSE:		pinguino delays functions
	PROGRAMER:		jean-pierre mandon
	FIRST RELEASE:	2008
	LAST RELEASE:	2013-01-17
	----------------------------------------------------------------------------
	CHANGELOG:
    * 2015-01-17    rblanchot - delays are now based on System_getPeripheralFrequency
    * 2016-05-02    rblanchot - delays use now _cpu_clock_
	----------------------------------------------------------------------------
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
	--------------------------------------------------------------------------*/

#ifndef __DELAYUS_C__
#define __DELAYUS_C__

#include <compiler.h>
#include <typedef.h>
#include <macro.h>
#include <mathlib.c>

extern u32 _cpu_clock_;

/*
    NB:Cycles per second = FOSC/4
    31KHz < FOSC                   < 64MHz
     7750 < Cycles per second      < 16.000.000
        8 < Cycles per millisecond < 16.000
        0 < Cycles per microsecond < 16
*/

void Delayus(u16 us)
{
    u8 i;
    u8 cyus = udiv32(_cpu_clock_, 1000000UL);
    
    while (us--)
    {
        i = cyus;
        while (i--);
    }
}

#endif // __DELAYUS_C__ 
