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

#ifndef LIBETHERNET_NTP_H__
#define LIBETHERNET_NTP_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#ifdef IMPLEMENT_NTP

typedef struct
{
	unsigned int year;
	unsigned int month;
	unsigned int day;
	unsigned int hour;
	unsigned int minute;
	unsigned int second;
} ntp_time_struct;

/**
 * Initialises the NTP module
 * @remark You must call this prior to calling any other NTP-related function
 * @param TimeserverIP The IP address of the timeserver that will tell us the time
 * @param TimezoneOffset The offset to GMT of your timezone (in hours)
 */
void ntp_initialise(uint32_t TimeserverIP,int32_t TimezoneOffset);

/**
 * Tells the NTP module that one second has passed
 * @remark Do not call this function, ethernet_second_tick does it for you
 */
void ntp_second_tick(void);

/**
 * Requests the current time from the timeserver
 */
void ntp_refresh(void);

/**
 * Returns the current timestamp parsed to date and time
 * @return A ntp_time_struct containing the parsed current timestamp
 */
ntp_time_struct ntp_get_time(void);

/**
 * Checks if the NTP module is currently querying the server for the time
 * @return True if it is currently querying, false otherwise
 */
bool ntp_is_querying(void);

/**
 * Sets the current timezone offset
 * @param TimezoneOffset The offset to GMT of your timezone (in hours)
 */
void ntp_set_timezone_offset(int32_t TimezoneOffset);

#endif //IMPLEMENT_NTP

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //LIBETHERNET_NTP_H__

