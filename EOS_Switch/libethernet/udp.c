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
#include "udp.h"

#ifdef IMPLEMENT_UDP

// -----------------------------------------------------------------------------------------------
// -------------------------------------- Global Variables ---------------------------------------
// -----------------------------------------------------------------------------------------------
static UDPTableEntry udp_table[UDP_TABLE_SIZE];
static UDPApplication udp_applications[UDP_APPLICATION_TABLE_SIZE];

// -----------------------------------------------------------------------------------------------
// ----------------------------- External Function Implementations -------------------------------
// -----------------------------------------------------------------------------------------------
void _udp_initialise(void)
{
	for(size_t i = 0; i < UDP_TABLE_SIZE; ++i)
		udp_table[i].Socket = INVALID_UDP_SOCKET;
}

UDPSocket _udp_table_add(uint32_t RemoteIP, uint16_t LocalPort, uint16_t RemotePort, uint16_t TimeoutValue, bool ClosePortOnTermination)
{
	for(size_t i = 0; i < UDP_TABLE_SIZE; ++i){
		if(udp_table[i].Socket == INVALID_UDP_SOCKET){
			udp_table[i].Socket = i;
			udp_table[i].RemoteIP = RemoteIP;
			udp_table[i].LocalPort = LocalPort;
			udp_table[i].RemotePort = RemotePort;
			udp_table[i].TimeoutValue = TimeoutValue;
			udp_table[i].ClosePortOnTermination = ClosePortOnTermination;

			return i;
		}
	}
	return INVALID_UDP_SOCKET;
}

void _udp_table_remove(UDPSocket Socket)
{
	if(udp_table_is_valid_socket(Socket) && udp_table[Socket].Socket == Socket)
		udp_table[Socket].Socket = INVALID_UDP_SOCKET;
}

const UDPTableEntry* udp_table_get_by_socket(UDPSocket Socket)
{
	if(!udp_table_is_valid_socket(Socket))
		return NULL;
	if(udp_table[Socket].Socket != Socket)
		return NULL;
	return &udp_table[Socket];
}

const UDPTableEntry* udp_table_get(uint32_t RemoteIP, uint16_t LocalPort, uint16_t RemotePort)
{
	for(size_t i = 0; i < UDP_TABLE_SIZE; ++i){
		if(udp_table[i].RemoteIP == RemoteIP && udp_table[i].LocalPort == LocalPort && udp_table[i].RemotePort == RemotePort)
			return &udp_table[i];
	}
	return NULL;
}

bool udp_table_is_valid_socket(UDPSocket Socket)
{
	return (Socket < UDP_TABLE_SIZE);
}

bool udp_open_port(uint16_t Port, uint16_t TimeoutValue, UDPCallbackOpenConnection OpenCallback, UDPCallbackCloseConnection CloseCallback, UDPCallbackHandlePacket HandlePacketCallback)
{
	// Check if any other application is already listening on the port
	if(udp_get_port_application(Port))
		return false;

	// Try to find an empty application slot
	for(size_t i = 0; i < UDP_APPLICATION_TABLE_SIZE; ++i){
		if(udp_applications[i].Port == 0){
			udp_applications[i].Port = Port;
			udp_applications[i].TimeoutValue = TimeoutValue;
			udp_applications[i].OpenConnectionCallback = OpenCallback;
			udp_applications[i].CloseConnectionCallback = CloseCallback;
			udp_applications[i].HandlePacketCallback = HandlePacketCallback;
			return true;
		}
	}
	return false;
}

void udp_close_port(uint16_t Port)
{
	for(size_t i = 0; i < UDP_APPLICATION_TABLE_SIZE; ++i){
		if(udp_applications[i].Port == Port){
			udp_applications[i].Port = 0;
			break;
		}
	}
}

const UDPApplication* udp_get_port_application(uint16_t Port)
{
	for(size_t i = 0; i < UDP_APPLICATION_TABLE_SIZE; ++i){
		if(udp_applications[i].Port == Port)
			return &udp_applications[i];
	}
	return NULL;
}

#endif //IMPLEMENT_UDP