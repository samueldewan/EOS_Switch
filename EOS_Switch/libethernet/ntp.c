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
#include "ethernet.h"
#include "ntp.h"

#ifdef IMPLEMENT_NTP

// -----------------------------------------------------------------------------------------------
// ------------------------------------- Constants / Structs -------------------------------------
// -----------------------------------------------------------------------------------------------
#define NTP_PORT 123
#define NTP_HEADER_OFFSET 0
#define NTP_HEADER_LENGTH 48

typedef struct _NTPHeader
{
	uint8_t Flags;
	uint8_t PeerStratum;
	uint8_t PeerPollingInterval;
	uint8_t PeerClockPrecision;
	uint32_t RootDelay;
	uint32_t RootDispersion;
	uint32_t ReferenceID;
	uint64_t ReferenceTimestamp;
	uint64_t OriginTimestamp;
	uint64_t ReceiveTimestamp;
	uint64_t TransmitTimestamp;
} NTPHeader;

// -----------------------------------------------------------------------------------------------
// -------------------------------------- Global Variables ---------------------------------------
// -----------------------------------------------------------------------------------------------
static uint32_t ntp_TimeserverIP = 0;
static int32_t ntp_TimezoneOffset = 0;
static uint32_t ntp_CurrentSeconds = 0;
static bool ntp_IsQuerying = false;

// -----------------------------------------------------------------------------------------------
// ----------------------------- Internal Function Implementations -------------------------------
// -----------------------------------------------------------------------------------------------
/**
 * Checks if the given year is a leap year
 * @remark Only for internal use!
 * @param year The year to check
 * @return True if [year] was a leap year
 */
bool _ntp_is_leap_year(unsigned int year)
{
	return (year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0));
}

/**
 * Calculates how many days the given year had
 * @remark Only for internal use!
 * @param year The year
 * @return The number of days [year] had
 */
unsigned int _ntp_get_days_of_year(unsigned int year)
{
	if(_ntp_is_leap_year(year))
		return 366;
	return 365;
}

/**
 * Calculates how many days the given month of the given year had
 * @remark Only for internal use!
 * @param year The year
 * @param month The month
 * @return The number of days [month] had in [year]
 */
unsigned int _ntp_get_days_of_month(unsigned int year,unsigned int month)
{
	if(month <= 6){
		if(month == 1)
			return _ntp_is_leap_year(year) ? 29 : 28;
		return (month % 2 == 0) ? 31 : 30;
	}else{
		return (month % 2 == 1) ? 31 : 30;
	}
}

/**
 * Converts a given NTP timestamp into a date
 * @remark Only for internal use!
 * @param Seconds The seconds since beginning of the current epoch
 * @return The date contained in a ntp_time_struct
 */
ntp_time_struct _ntp_convert_time(uint32_t Seconds)
{
	// Convert the time
	ntp_time_struct res;
	res.second = Seconds % 60;
	res.minute = (Seconds % 3600) / 60;
	res.hour = (Seconds % 86400) / 3600;

	// Calculate the current year
	unsigned int cur_year = 1900;
	unsigned int days = Seconds / 86400;
	unsigned int days_of_cur_year = _ntp_get_days_of_year(cur_year);
	while(days >= days_of_cur_year){
		days -= days_of_cur_year;
		++cur_year;
		days_of_cur_year = _ntp_get_days_of_year(cur_year);
	}
	res.year = cur_year;

	// Calculate the current month
	unsigned int cur_month = 0;
	unsigned int days_of_cur_month = _ntp_get_days_of_month(cur_year,cur_month);
	while(days >= days_of_cur_month){
		days -= days_of_cur_month;
		++cur_month;
		days_of_cur_month = _ntp_get_days_of_month(cur_year,cur_month);
	}
	res.month = cur_month + 1;
	res.day = days + 1;
	return res;
}

/**
 * The NTP packet handler callback
 * @remark Only for internal use! Confer UDPCallbackHandlePacket for further information.
 */
void _ntp_handle_udp_packet(UDPSocket Socket, const uint8_t* Buffer, size_t Length)
{
	if(Length != NTP_HEADER_LENGTH)
		return;

	// Read the timestamp from the packet
	const NTPHeader* ntp_hdr = (const NTPHeader*)&Buffer[NTP_HEADER_OFFSET];
	ntp_CurrentSeconds = (uint32_t)(ntp_hdr->TransmitTimestamp & 0xFFFFFFFF);
	ntp_CurrentSeconds = NTOHL(ntp_CurrentSeconds) + ntp_TimezoneOffset;

	// We can now disconnect from the time server
	udp_disconnect(Socket);

	ntp_IsQuerying = false;
}

/**
 * Sends a NTP request packet
 * @param Socket The UDP socket this packet will be sent to
 */
void _ntp_send_request(UDPSocket Socket)
{
	uint8_t* Buffer;
	size_t BufferSize;
	
	udp_start_packet(Socket,&Buffer,&BufferSize);
	NTPHeader* ntp_hdr = (NTPHeader*)&Buffer[NTP_HEADER_OFFSET];
	ntp_hdr->Flags = 0xE3; //Leap indicator unknown, NTP version 4, Client mode
	ntp_hdr->PeerStratum = 0;
	ntp_hdr->PeerPollingInterval = 4;
	ntp_hdr->PeerClockPrecision = 0xFA;
	ntp_hdr->RootDelay = HTONL(0x00010000); // 1s
	ntp_hdr->RootDispersion = HTONL(0x00010000); // 1s
	ntp_hdr->ReferenceID = 0;
	ntp_hdr->ReferenceTimestamp = 0;
	ntp_hdr->OriginTimestamp = 0;
	ntp_hdr->ReceiveTimestamp = 0;
	ntp_hdr->TransmitTimestamp = 0;
	udp_send(NTP_HEADER_LENGTH);
}

// -----------------------------------------------------------------------------------------------
// ----------------------------- External Function Implementations -------------------------------
// -----------------------------------------------------------------------------------------------
void ntp_initialise(uint32_t TimeserverIP,int32_t TimezoneOffset)
{
	ntp_TimeserverIP = TimeserverIP;
	ntp_TimezoneOffset = TimezoneOffset * 3600;
	ntp_CurrentSeconds = 0;
	ntp_IsQuerying = false;
}

void ntp_second_tick(void)
{
	++ntp_CurrentSeconds;
}

void ntp_refresh(void)
{
	if(ntp_IsQuerying)
		return;

	ntp_IsQuerying = true;
	UDPSocket sck = udp_connect(ntp_TimeserverIP,NTP_PORT,5000,&_ntp_handle_udp_packet);
	if(sck == INVALID_UDP_SOCKET)
		ntp_IsQuerying = false;
	else
		_ntp_send_request(sck);
}

ntp_time_struct ntp_get_time(void)
{
	return _ntp_convert_time(ntp_CurrentSeconds);
}

bool ntp_is_querying(void)
{
	return ntp_IsQuerying;
}

void ntp_set_timezone_offset(int32_t TimezoneOffset)
{
	ntp_TimezoneOffset = TimezoneOffset * 3600;
}

#endif //IMPLEMENT_NTP