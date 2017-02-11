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

#ifndef LIBETHERNET_UDP_H__
#define LIBETHERNET_UDP_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#ifdef IMPLEMENT_UDP


/// Identifier for an UDP connection
typedef uint8_t UDPSocket;

/// Indicates that a socket is invalid
#define INVALID_UDP_SOCKET (UDP_TABLE_SIZE)

typedef void (*UDPCallbackOpenConnection)(UDPSocket Socket,uint32_t IP);
typedef void (*UDPCallbackCloseConnection)(UDPSocket Socket);
typedef void (*UDPCallbackHandlePacket)(UDPSocket Socket,const uint8_t* Buffer,size_t Length);

typedef struct _UDPTableEntry
{
	/// Indicates if we have to close the port on termination
	bool ClosePortOnTermination;

	/// The socket
	UDPSocket Socket;

	/// The remote IP
	uint32_t RemoteIP;

	/// The port number on our side
	uint16_t LocalPort;

	/// The port number on the remote side
	uint16_t RemotePort;

	/// The timeout value used for all operations regarding this connection that can time out (in milliseconds)
	uint16_t TimeoutValue;
} UDPTableEntry;

typedef struct _UDPApplication
{
	/// The local port the application is listening on
	uint16_t Port;

	/// The timeout value used for all inbound connections for this application (in milliseconds)
	uint16_t TimeoutValue;

	/// Will be invoked once a new connection is incoming
	UDPCallbackOpenConnection OpenConnectionCallback;

	/// Will be invoked once a connection is terminated
	UDPCallbackCloseConnection CloseConnectionCallback;

	/// Will be invoked whenever a packet is received on the respective port
	UDPCallbackHandlePacket HandlePacketCallback;	
} UDPApplication;


/**
 * Initialises all UDP data to its default state
 * @remark Only for internal use!
 */
void _udp_initialise(void);

/**
 * Adds a new connection to the UDP table
 * @remark Only for internal use!
 * @param RemoteIP The remote IP address
 * @param LocalPort The local port used for the connection
 * @param RemotePort The remote port used for the connection
 * @param TimeoutValue The timeout value used for all operations regarding this connection (in milliseconds)
 * @param ClosePortOnTermination Indicates if the local port should be closed upon termination
 * @return A valid UDPSocket if everything went fine, INVALID_UDPSOCKET otherwise
 */
UDPSocket _udp_table_add(uint32_t RemoteIP, uint16_t LocalPort, uint16_t RemotePort, uint16_t TimeoutValue, bool ClosePortOnTermination);

/**
 * Removes a connection from the UDP table
 * @remark Only for internal use!
 * @param Socket The socket assigned to the connection to be removed
 */
void _udp_table_remove(UDPSocket Socket);

/**
 * Finds a connection by its socket
 * @param Socket The socket assigned to the connection
 * @return NULL if the connection was not found, a valid pointer otherwise
 */
const UDPTableEntry* udp_table_get_by_socket(UDPSocket Socket);

/**
 * Finds a connection by its connection information
 * @param RemoteIP The remote IP of the connection
 * @param LocalPort The local connection port
 * @param RemotePort The remote connection port
 * @return NULL if the connection was not found, a valid pointer otherwise
 */
const UDPTableEntry* udp_table_get(uint32_t RemoteIP, uint16_t LocalPort, uint16_t RemotePort);

/**
 * Checks if the given socket is valid
 * @param Socket The socket to check
 * @return True if it's valid, false otherwise
 */
bool udp_table_is_valid_socket(UDPSocket Socket);

/**
 * Starts listening on an UDP port using the specified callback handlers
 * @param Port The port to listen on
 * @param TimeoutValue The timeout value of the application (in milliseconds)
 * @param OpenCallback Pointer to the callback that will be invoked whenever a new connection on the port is established. Can be NULL.
 * @param CloseCallback Pointer to the callback that will be invoked whenever an existing connection on the port is terminated. Can be NULL.
 * @param HandlePacketCallback Pointer to the callback that will be invoked whenever a packet is sent to us on the port.
 * @return True if everything went fine, false if any error occurred.
 */
bool udp_open_port(uint16_t Port, uint16_t TimeoutValue, UDPCallbackOpenConnection OpenCallback, UDPCallbackCloseConnection CloseCallback, UDPCallbackHandlePacket HandlePacketCallback);

/**
 * Stops listening on an UDP port
 * @param Port The port to stop listening on
 */
void udp_close_port(uint16_t Port);

/**
 * Finds an application by the port its listening on
 * @param Port The port so search for
 * @return NULL if no application was found, a valid pointer otherwise
 */
const UDPApplication* udp_get_port_application(uint16_t Port);


#endif //IMPLEMENT_UDP

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //LIBETHERNET_UDP_H__