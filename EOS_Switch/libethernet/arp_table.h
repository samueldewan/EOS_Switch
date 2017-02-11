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

#ifndef LIBETHERNET_ARP_TABLE_H__
#define LIBETHERNET_ARP_TABLE_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus


/**
 * Contains information about one entry in the ARP table
 */
typedef struct _ARPTableEntry
{
	uint8_t MAC[6];
	uint32_t IP;
	uint16_t TimeLeft;
} ARPTableEntry;


/**
 * Informs the ARP table that one second has passed.
 * @remark Do not call this function, ethernet_second_tick does it for you
 */
void arp_table_second_tick(void);

/**
 * Initialises the ARP table to its default state
 */
void arp_table_initialise(void);

/**
 * Adds an entry to the ARP table
 * @remark If the IP is already in the table, the entry is refreshed
 * @param MAC Pointer to a six-byte-array containing the MAC Address
 * @param IP The IP Address
 */
void arp_table_add(const uint8_t* MAC,uint32_t IP);

/**
 * Receives the data stored for a specific IP
 * @param IP The IP the data is assigned to
 * @return If any data about the IP was found, a valid pointer to an ARPTableEntry struct containing the information. Otherwise NULL.
 */
const ARPTableEntry* arp_table_get(uint32_t IP);

/**
 * Checks if the ARP table is full
 * @return True if the table if full, false otherwise
 */
bool arp_table_is_full(void);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //LIBETHERNET_ARP_TABLE_H__