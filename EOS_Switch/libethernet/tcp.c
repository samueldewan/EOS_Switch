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
#include "tcp.h"

#ifdef IMPLEMENT_TCP

// -----------------------------------------------------------------------------------------------
// -------------------------------------- Global Variables ---------------------------------------
// -----------------------------------------------------------------------------------------------
static TCPTableEntry tcp_table[TCP_TABLE_SIZE];
static TCPApplication tcp_applications[TCP_APPLICATION_TABLE_SIZE];

// -----------------------------------------------------------------------------------------------
// ----------------------------- External Function Implementations -------------------------------
// -----------------------------------------------------------------------------------------------
void _tcp_initialise(void)
{
	for(size_t i = 0; i < TCP_TABLE_SIZE; ++i)
		tcp_table[i].Socket = INVALID_TCP_SOCKET;
}

TCPSocket _tcp_table_add(uint32_t RemoteIP, uint16_t LocalPort, uint16_t RemotePort, uint16_t TimeoutValue, bool ClosePortOnTermination)
{
	for(size_t i = 0; i < TCP_TABLE_SIZE; ++i){
		if(tcp_table[i].Socket == INVALID_TCP_SOCKET){
			tcp_table[i].Socket = i;
			tcp_table[i].RemoteIP = RemoteIP;
			tcp_table[i].LocalPort = LocalPort;
			tcp_table[i].RemotePort = RemotePort;
			tcp_table[i].TimeoutValue = TimeoutValue;
			tcp_table[i].LastAcknowledgementNumber = 0;
			tcp_table[i].LastSequenceNumber = 0;
			tcp_table[i].ConnectionState = TCP_CONNECTION_STATE_INVALID;
			tcp_table[i].HasAcknowledgedLastPacket = false;
			tcp_table[i].NeedsPushOnNextPacket = true;
			tcp_table[i].ClosePortOnTermination = ClosePortOnTermination;

			return i;
		}
	}
	return INVALID_TCP_SOCKET;
}

void _tcp_table_remove(TCPSocket Socket)
{
	if(tcp_table_is_valid_socket(Socket) && tcp_table[Socket].Socket == Socket)
		tcp_table[Socket].Socket = INVALID_TCP_SOCKET;
}

TCPTableEntry* tcp_table_get_by_socket(TCPSocket Socket)
{
	if(!tcp_table_is_valid_socket(Socket))
		return NULL;
	if(tcp_table[Socket].Socket != Socket)
		return NULL;
	return &tcp_table[Socket];
}

TCPTableEntry* tcp_table_get(uint32_t RemoteIP, uint16_t LocalPort, uint16_t RemotePort)
{
	for(size_t i = 0; i < TCP_TABLE_SIZE; ++i){
		if(tcp_table[i].RemoteIP == RemoteIP && tcp_table[i].LocalPort == LocalPort && tcp_table[i].RemotePort == RemotePort)
			return &tcp_table[i];
	}
	return NULL;
}

bool tcp_table_is_valid_socket(TCPSocket Socket)
{
	return (Socket < TCP_TABLE_SIZE);
}

bool tcp_open_port(uint16_t Port, uint16_t TimeoutValue, TCPCallbackOpenConnection OpenCallback, TCPCallbackCloseConnection CloseCallback, TCPCallbackHandlePacket HandlePacketCallback)
{
	// Check if any other application is already listening on the port
	if(tcp_get_port_application(Port))
		return false;

	// Try to find an empty application slot
	for(size_t i = 0; i < TCP_APPLICATION_TABLE_SIZE; ++i){
		if(tcp_applications[i].Port == 0){
			tcp_applications[i].Port = Port;
			tcp_applications[i].TimeoutValue = TimeoutValue;
			tcp_applications[i].OpenConnectionCallback = OpenCallback;
			tcp_applications[i].CloseConnectionCallback = CloseCallback;
			tcp_applications[i].HandlePacketCallback = HandlePacketCallback;
			return true;
		}
	}
	return false;
}

void tcp_close_port(uint16_t Port)
{
	for(size_t i = 0; i < TCP_APPLICATION_TABLE_SIZE; ++i){
		if(tcp_applications[i].Port == Port){
			tcp_applications[i].Port = 0;
			break;
		}
	}
}

const TCPApplication* tcp_get_port_application(uint16_t Port)
{
	for(size_t i = 0; i < TCP_APPLICATION_TABLE_SIZE; ++i){
		if(tcp_applications[i].Port == Port)
			return &tcp_applications[i];
	}
	return NULL;
}

#endif //IMPLEMENT_TCP