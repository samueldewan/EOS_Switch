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

#ifndef LIBETHERNET_ETHERNET_H__
#define LIBETHERNET_ETHERNET_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#ifdef IMPLEMENT_UDP
#	include "udp.h"
#endif
#ifdef IMPLEMENT_TCP
#	include "tcp.h"
#endif

#ifdef USE_INTERRUPTS
#ifdef HANDLE_LINK_STATUS_CHANGES
/// The prototype of the callback that will be invoked when a link status change has been detected
typedef void (*ethernet_callback_link_status_change)(bool CurrentStatus);
#endif //HANDLE_LINK_STATUS_CHANGES
#endif //USE_INTERRUPTS

/// The hardware type identifier for ethernet used in multiple protocols such as ARP and DHCP
#define HARDWARE_TYPE_ETHERNET 0x0001

/// The length of a MAC address
#define MAC_ADDRESS_LENGTH 6

/**
 * Generates a DWORD containing the IP Address (so you can easily read the IPs like MAKE_IP(127,0,0,1))
 * @param a,b,c,d The four IP bytes
 * @return The IP DWORD
 */
#define MAKE_IP(a,b,c,d) (((unsigned long)(d)<<24)+((unsigned long)(c)<<16)+((unsigned long)(b)<<8)+a)

/**
 * Converts a two-byte integer from host endianess to network endianess
 * @param x The integer in host endianess
 * @return The integer in network endianess
 */
#define HTONS(x) ((uint16_t)((((uint16_t)(x)) << 8) | (((uint16_t)(x)) >> 8)))

/**
 * Converts a two-byte integer from network endianess to host endianess
 * @param x The integer in network endianess
 * @return The integer in host endianess
 */
#define NTOHS(x) ((uint16_t)((((uint16_t)(x)) << 8) | (((uint16_t)(x)) >> 8)))

/**
 * Converts a four-byte integer from host endianess to network endianess
 * @param x The integer in host endianess
 * @return The integer in network endianess
 */
#define HTONL(x) ((((x) & 0xFF000000)>>24)+(((x) & 0x00FF0000)>>8)+(((x) & 0x0000FF00)<<8)+(((x) & 0x000000FF)<<24))

/**
 * Converts a four-byte integer from network endianess to host endianess
 * @param x The integer in network endianess
 * @return The integer in host endianess
 */
#define NTOHL(x) ((((x) & 0xFF000000)>>24)+(((x) & 0x00FF0000)>>8)+(((x) & 0x0000FF00)<<8)+(((x) & 0x000000FF)<<24))

/**
 * Initialises the ethernet stack
 * @remark You must not call any other function before this one!
 * @param IPAddress The static IP address the module should use
 * @param NetMask The net mask for the network
 * @param RouterIP The network's router IP address. Required for outgoing ARP requests
 */
void ethernet_initialise(uint32_t IPAddress,uint32_t NetMask,uint32_t RouterIP);

#ifdef IMPLEMENT_DHCP
/**
 * Initialises the ethernet stack using DHCP
 * @remark You must not call any other function before this one!
 * @param Hostname Our hostname. Can be NULL for no hostname
 * @param Timeout The timeout (in milliseconds) until the request is aborted. Can be 0 so the request will never time out
 */
void ethernet_initialise_dhcp(const char* Hostname,uint16_t Timeout);
#endif //IMPLEMENT_DHCP

/**
 * Deinitialises the ethernet stack
 * @remark You must not call any other function (except initialise) after this one!
 */
void ethernet_deinitialise(void);

/**
 * Informs the ethernet stack that one second has passed.
 * @remark Call this one everytime a second has passed!
 */
void ethernet_second_tick(void);

/**
 * Updates the ethernet stack
 * @remark Call this one frequently in your main loop!
 */
void ethernet_update(void);

#ifdef USE_INTERRUPTS
/**
 * Informs the ethernet stack of a data interrupt
 * @remark Call this in your interrupt handler function!
 */
void ethernet_data_interrupt(void);
#endif //USE_INTERRUPTS

/**
 * Checks if the ethernet link is up
 * @return True if the link is established
 */
bool ethernet_get_link_status(void);

/**
 * Waits until link is up with a given timeout
 * @param Timeout The timeout, 0 for infinite
 * @return False on a timeout, True if the link is established
 */
bool ethernet_wait_for_link_status(uint16_t Timeout);

#ifdef HANDLE_LINK_STATUS_CHANGES
/**
 * Sets the function to handle link status change callbacks
 * @param NewCallback The callback handler, NULL for no callback
 */
void ethernet_set_link_status_change_callback(ethernet_callback_link_status_change NewCallback);
#endif //HANDLE_LINK_STATUS_CHANGES

/**
 * Sets our IP address, the net mask of our network and its router IP
 * @remark Use this with caution!!!
 * @param IP The new IP
 * @param NetMask The new net mask
 * @param RouterIP The new router IP
 */
void _ethernet_set_ip_netmask_router(uint32_t IP, uint32_t NetMask, uint32_t RouterIP);

#ifdef IMPLEMENT_DHCP
/**
 * Tries to retrieve a valid DHCP configuration
 * @remark Only for internal use!
 * @param Hostname Our Hostname. Can be NULL for no hostname
 * @param Timeout The timeout (in milliseconds) until the request is aborted. Can be 0 so the request will never time out
 * @return True if a valid configuration has been received, False otherwise
 */
bool _ethernet_configure_via_dhcp(const char* Hostname, uint16_t Timeout);
#endif // IMPLEMENT_DHCP	
 
/**
 * Gets the current IP address of the module
 * @return The IP address
 */
uint32_t ethernet_get_ip(void);

/**
 * Gets the current net mask of the module
 * @return The net mask
 */
uint32_t ethernet_get_netmask(void);

/**
 * Gets the current router IP address of the module
 * @return The router IP address
 */
uint32_t ethernet_get_router_ip(void);


#ifdef IMPLEMENT_UDP
/**
 * Establishes an UDP connection to the given IP at the given Port
 * @param IP The IP address
 * @param Port The port
 * @param Timeout The maximum time any operation regarding the connection may take (in milliseconds). If this elapses during an operation, the connection is terminated
 * @param HandlePacketCallback The callback that will be invoked when answers are received. Can be NULL (you cannot receive any answer then!)
 * @return The connection's socket. INVALID_UDP_SOCKET if the connection failed
 */
UDPSocket udp_connect(uint32_t IP, uint16_t Port, uint16_t Timeout, UDPCallbackHandlePacket HandlePacketCallback);

/**
 * Establishes an UDP connection to the given IP at the given Port using a given local Port
 * @param IP The IP address
 * @param Port The port
 * @param Timeout The maximum time any operation regarding the connection may take (in milliseconds). If this elapses during an operation, the connection is terminated
 * @param HandlePacketCallback The callback that will be invoked when answers are received. Can be NULL (you cannot receive any answer then!)
 * @param LocalPort The local port
 * @return The connection's socket. INVALID_UDP_SOCKET if the connection failed or LocalPort is already in use
 */
UDPSocket udp_connect_ex(uint32_t IP, uint16_t Port, uint16_t Timeout, UDPCallbackHandlePacket HandlePacketCallback, uint16_t LocalPort);

/**
 * Terminates an UDP connection
 * @remark Socket will no longer be valid after the connection was terminated!
 * @param Socket The connection's socket
 */
void udp_disconnect(UDPSocket Socket);

/**
 * Tries to start a new UDP packet to be sent to a given socket in the global packet buffer
 * @remark WARNING: There is only one global packet buffer into which packets are received and from which packets are sent.
 * @param Socket The socket this packet will be sent to when calling udp_send()
 * @param BufferPtr Will store a pointer to the first byte you may write to
 * @param BufferSize Will store the maximum size (in bytes) that you may write
 * @return True if everything succeeded. If False is returned, the connection has timed out and Socket IS NO LONGER VALID!
 */
bool udp_start_packet(UDPSocket Socket, uint8_t** BufferPtr, size_t* BufferSize);

/**
 * Sends a packet via UDP
 * @remark Sends the last packet started via udp_start_packet
 * @param Socket The socket of the connection
 * @param Length The number (in bytes) of data to send
 */
void udp_send(size_t Length);
#endif //IMPLEMENT_UDP

#ifdef IMPLEMENT_TCP
/**
 * Establishes a TCP connection to the given IP at the given Port
 * @param IP The IP address
 * @param Port The port
 * @param Timeout The maximum time the TCP handshake may take (in milliseconds). If this elapses, the connection process will be aborted.
 * @param HandlePacketCallback The callback that will be invoked when answers are received. Can be NULL (you cannot receive any answer then!).
 * @return The connection's socket. INVALID_TCP_SOCKET if the connection failed
 */
TCPSocket tcp_connect(uint32_t IP, uint16_t Port, uint16_t Timeout, TCPCallbackHandlePacket HandlePacketCallback);

/**
 * Terminates a TCP connection
 * @remark Socket will no longer be valid after the connection was terminated!
 * @param Socket The connection's socket
 */
void tcp_disconnect(TCPSocket Socket);

/**
 * Starts a new TCP packet to be sent to a given socket in the global packet buffer
 * @remark WARNING: There is only one global packet buffer into which packets are received and from which packets are sent.
 * @param Socket The socket this packet will be sent to when calling tcp_send()
 * @param BufferPtr Will store a pointer to the first byte you may write to
 * @param BufferSize Will store the maximum size (in bytes) that you may write
 * @return True if everything succeeded. If False is returned, the connection has timed out and Socket IS NO LONGER VALID!
 */
bool tcp_start_packet(TCPSocket Socket, uint8_t** BufferPtr, size_t* BufferSize);

/**
 * Sends a packet via TCP
 * @remark Sends the last packet started via tcp_start_packet
 * @param Socket The socket of the connection
 * @param Length The number (in bytes) of data to send
 */
void tcp_send(size_t Length);
#endif //IMPLEMENT_TCP

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //LIBETHERNET_ETHERNET_H__