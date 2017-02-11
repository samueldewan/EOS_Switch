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

#ifndef LIBETHERNET_TCP_H__
#define LIBETHERNET_TCP_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#ifdef IMPLEMENT_TCP


/// Identifier for a TCP connection
typedef uint8_t TCPSocket;

/// Indicates that a socket is invalid
#define INVALID_TCP_SOCKET (TCP_TABLE_SIZE)

typedef void (*TCPCallbackOpenConnection)(TCPSocket Socket,uint32_t IP);
typedef void (*TCPCallbackCloseConnection)(TCPSocket Socket);
typedef void (*TCPCallbackHandlePacket)(TCPSocket Socket,const uint8_t* Buffer,size_t Length);

/// Indicates the current state of a TCP connection
typedef enum
{
	TCP_CONNECTION_STATE_INVALID,
	TCP_CONNECTION_STATE_HANDSHAKE_OUTGOING,
	TCP_CONNECTION_STATE_HANDSHAKE_INCOMING,
	TCP_CONNECTION_STATE_CONNECTED,
	TCP_CONNECTION_STATE_TERMINATION_OUTGOING,
	TCP_CONNECTION_STATE_TERMINATION_INCOMING,
} TCPConnectionState;

typedef struct _TCPTableEntry
{
	/// Indicates if we have to close the port on termination
	bool ClosePortOnTermination;

	/// The socket
	TCPSocket Socket;

	/// The remote IP
	uint32_t RemoteIP;

	/// The port number on our side
	uint16_t LocalPort;

	/// The port number on the remote side
	uint16_t RemotePort;

	/// The last acknowledged packet number
	uint32_t LastAcknowledgementNumber;

	/// The last sequence number
	uint32_t LastSequenceNumber;

	/// The connection's state
	TCPConnectionState ConnectionState;

	/// Indicates if an ACK should be sent with the next packet or not
	bool HasAcknowledgedLastPacket;

	/// Indicates if a PSH needs to be sent with the next packet or not
	bool NeedsPushOnNextPacket;

	/// The timeout value used for all operations regarding this connection that can time out (in milliseconds)
	uint16_t TimeoutValue;
} TCPTableEntry;

typedef struct _TCPApplication
{
	/// The local port the application is listening on
	uint16_t Port;

	/// The timeout value used for all inbound connections for this application (in milliseconds)
	uint16_t TimeoutValue;

	/// Will be invoked once a new connection is incoming
	TCPCallbackOpenConnection OpenConnectionCallback;

	/// Will be invoked once a connection is terminated
	TCPCallbackCloseConnection CloseConnectionCallback;

	/// Will be invoked whenever a packet is received on the respective port
	TCPCallbackHandlePacket HandlePacketCallback;	
} TCPApplication;


/**
 * Initialises all TCP data to its default state
 * @remark Only for internal use!
 */
void _tcp_initialise(void);

/**
 * Adds a new connection to the TCP table
 * @remark Only for internal use!
 * @param RemoteIP The remote IP address
 * @param LocalPort The local port used for the connection
 * @param RemotePort The remote port used for the connection
 * @param TimeoutValue The timeout value used for all operations regarding this connection (in milliseconds)
 * @return A valid TCPSocket if everything went fine, INVALID_TCPSOCKET otherwise
 */
TCPSocket _tcp_table_add(uint32_t RemoteIP, uint16_t LocalPort, uint16_t RemotePort, uint16_t TimeoutValue, bool ClosePortOnTermination);

/**
 * Removes a connection from the TCP table
 * @remark Only for internal use!
 * @param Socket The socket assigned to the connection to be removed
 */
void _tcp_table_remove(TCPSocket Socket);

/**
 * Finds a connection by its socket
 * @param Socket The socket assigned to the connection
 * @return NULL if the connection was not found, a valid pointer otherwise
 */
TCPTableEntry* tcp_table_get_by_socket(TCPSocket Socket);

/**
 * Finds a connection by its connection information
 * @param RemoteIP The remote IP of the connection
 * @param LocalPort The local connection port
 * @param RemotePort The remote connection port
 * @return NULL if the connection was not found, a valid pointer otherwise
 */
TCPTableEntry* tcp_table_get(uint32_t RemoteIP, uint16_t LocalPort, uint16_t RemotePort);

/**
 * Checks if the given socket is valid
 * @param Socket The socket to check
 * @return True if it's valid, false otherwise
 */
bool tcp_table_is_valid_socket(TCPSocket Socket);

/**
 * Starts listening on an TCP port using the specified callback handlers
 * @param Port The port to listen on
 * @param TimeoutValue The timeout value of the application (in milliseconds)
 * @param OpenCallback Pointer to the callback that will be invoked whenever a new connection on the port is established. Can be NULL.
 * @param CloseCallback Pointer to the callback that will be invoked whenever an existing connection on the port is terminated. Can be NULL.
 * @param HandlePacketCallback Pointer to the callback that will be invoked whenever a packet is sent to us on the port.
 * @return True if everything went fine, false if any error occurred.
 */
bool tcp_open_port(uint16_t Port, uint16_t TimeoutValue, TCPCallbackOpenConnection OpenCallback, TCPCallbackCloseConnection CloseCallback, TCPCallbackHandlePacket HandlePacketCallback);

/**
 * Stops listening on an TCP port
 * @param Port The port to stop listening on
 */
void tcp_close_port(uint16_t Port);

/**
 * Finds an application by the port its listening on
 * @param Port The port so search for
 * @return NULL if no application was found, a valid pointer otherwise
 */
const TCPApplication* tcp_get_port_application(uint16_t Port);

#endif //IMPLEMENT_TCP

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //LIBETHERNET_TCP_H__