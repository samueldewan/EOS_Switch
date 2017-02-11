/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * This file is part of avr-libethernet. For latest information and software updates,    *
 * see http://www.sourceforge.net/p/avrlibethernet. If you have wishes, improvements,    *
 * changes, bugfixes or suggestions regarding this file or any other of avr-libethernet, *
 * the copyright holders (see below) would be most welcome if you share them on the      *
 * project's website for further improvement of this project.                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2012 by Niklas Fritz, Alexander Gladis                                  *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "global.h"
#include "arp_table.h"

// -----------------------------------------------------------------------------------------------
// -------------------------------------- Global Variables ---------------------------------------
// -----------------------------------------------------------------------------------------------
static ARPTableEntry arp_table[ARP_TABLE_SIZE];


// -----------------------------------------------------------------------------------------------
// ----------------------------- External Function Implementations -------------------------------
// -----------------------------------------------------------------------------------------------
void arp_table_second_tick(void)
{
	for(size_t i = 0; i < ARP_TABLE_SIZE; ++i){
		if(arp_table[i].TimeLeft == 0){
			// The entry has expired, remove it
			arp_table[i].IP = 0;
			for(uint8_t idx = 0; idx < sizeof(arp_table[i].MAC); ++idx)
				arp_table[i].MAC[idx] = 0;
		}else{
			--arp_table[i].TimeLeft;
		}
	}
}

void arp_table_initialise(void)
{
	for(size_t i = 0; i < ARP_TABLE_SIZE; ++i)
		arp_table[i].IP = 0;
}

void arp_table_add(const uint8_t* MAC,uint32_t IP)
{
	const ARPTableEntry* entry = arp_table_get(IP);
	
	if(entry){
		// If we already have data about that IP, simply refresh it
		((ARPTableEntry*)entry)->TimeLeft = ARP_TABLE_TIMEOUT;
	}else{
		// Insert a new entry
		for(size_t i = 0; i < ARP_TABLE_SIZE; ++i){
			if(arp_table[i].IP == 0){
				for(uint8_t idx = 0; idx < sizeof(arp_table[i].MAC); ++idx)
					arp_table[i].MAC[idx] = MAC[idx];
				arp_table[i].IP = IP;
				arp_table[i].TimeLeft = ARP_TABLE_TIMEOUT;
				break;
			}
		}
	}
}

const ARPTableEntry* arp_table_get(uint32_t IP)
{
	for(size_t i = 0; i < ARP_TABLE_SIZE; ++i){
		if(arp_table[i].IP == IP)
			return &arp_table[i];
	}

	return NULL;
}

bool arp_table_is_full(void)
{
	for(size_t i = 0; i < ARP_TABLE_SIZE; ++i){
		// If this is an empty slot, the table isn't full. We can abort here.
		if(arp_table[i].IP == 0)
			return false;
	}
	return true;
}