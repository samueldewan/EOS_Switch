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

#include <util/delay.h>

#include "global.h"
#include "../global.h"
#include "utils.h"
#include "enc28j60.h"
#include "arp_table.h"
#include "ethernet.h"
#include "ntp.h"
#include "dhcp.h"
#include "dns.h"

// -----------------------------------------------------------------------------------------------
// ------------------------------------- Constants / Structs -------------------------------------
// -----------------------------------------------------------------------------------------------
#define IP_ADDRESS_LENGTH 4

// Ethernet header
#define ETHERNET_HEADER_OFFSET 0
#define ETHERNET_HEADER_LENGTH 14

#define ETHERNET_PACKET_TYPE_IP 0x0800
#define ETHERNET_PACKET_TYPE_ARP 0x0806

typedef struct _EthernetHeader
{
	/// Destination MAC Address
	uint8_t Dest[MAC_ADDRESS_LENGTH];

	/// Source MAC Address
	uint8_t Src[MAC_ADDRESS_LENGTH];

	/// Packet type
	uint16_t PacketType;
} EthernetHeader;


// ARP header
#define ARP_HEADER_OFFSET (ETHERNET_HEADER_OFFSET + ETHERNET_HEADER_LENGTH)
#define ARP_HEADER_LENGTH 28

#define ARP_OPCODE_REQUEST 0x0001
#define ARP_OPCODE_REPLY 0x0002

typedef struct _ARPHeader
{
	/// Hardware type
	uint16_t HWType;

	/// Protocol type
	uint16_t PRType;

	/// Hardware address length (here: 6 bytes MAC)
	uint8_t HWLen;

	/// Protocol address length (here: 4 bytes IP)
	uint8_t PRLen;

	/// Operation code (Request or Answer)
	uint16_t Opcode;

	/// Source MAC address
	uint8_t SHAddr[6];

	/// Source IP address
	uint32_t SIPAddr;

	/// Target MAC address
	uint8_t THAddr[6];

	/// Target IP address
	uint32_t TIPAddr;
} ARPHeader;


// IP header
#define IP_HEADER_OFFSET (ETHERNET_HEADER_OFFSET + ETHERNET_HEADER_LENGTH)
#define IP_HEADER_LENGTH 20

#ifdef IMPLEMENT_ICMP
#	define IP_PROTOCOL_ICMP 0x01
#endif //IMPLEMENT_ICMP
#ifdef IMPLEMENT_TCP
#	define IP_PROTOCOL_TCP 0x06
#endif //IMPLEMENT_TCP
#ifdef IMPLEMENT_UDP
#	define IP_PROTOCOL_UDP 0x11
#endif //IMPLEMENT_UDP

typedef struct _IPHeader
{
	/// Version and header length
	uint8_t VersLen;

	/// Type of service
	uint8_t TOS;

	/// Length of the packet
	uint16_t PktLen;

	/// Packet ID (used for fragmentation/reassembling)
	uint16_t ID;

	/// Fragmentation offset
	uint16_t FragOffset;

	/// Time To Live
	uint8_t TTL;

	/// Protocol ID
	uint8_t Proto;

	/// Header Checksum
	uint16_t HdrCksum;

	/// Source IP address
	uint32_t SrcAddr;

	/// Destination IP address
	uint32_t DestAddr;
} IPHeader;


// ICMP header
#define ICMP_HEADER_OFFSET (IP_HEADER_OFFSET + IP_HEADER_LENGTH)
#define ICMP_HEADER_LENGTH 8
#define ICMP_DATA_OFFSET (ICMP_HEADER_OFFSET + ICMP_HEADER_LENGTH);

typedef struct _ICMPHeader
{
	/// Message type
	uint8_t Type;

	/// Further information on the message type
	uint8_t Code;

	/// Checksum
	uint16_t Cksum;

	/// ID number
	uint16_t ID;

	/// Sequence number
	uint16_t SeqNum;
} ICMPHeader;


// UDP header
#ifdef IMPLEMENT_UDP
#define UDP_HEADER_OFFSET (IP_HEADER_OFFSET + IP_HEADER_LENGTH)
#define UDP_HEADER_LENGTH 8
#define UDP_DATA_OFFSET (UDP_HEADER_OFFSET + UDP_HEADER_LENGTH)

typedef struct _UDPHeader
{
	/// Source port
	uint16_t SrcPort;

	/// Destination port
	uint16_t DestPort;

	/// Packet length (including UDP header)
	uint16_t Length;

	/// Checksum
	uint16_t Checksum;
} UDPHeader;
#endif //IMPLEMENT_UDP


// TCP Header
#ifdef IMPLEMENT_TCP
#define TCP_HEADER_OFFSET (IP_HEADER_OFFSET + IP_HEADER_LENGTH)
#define TCP_HEADER_LENGTH 20

#define MAX_TCP_WINDOW_SIZE (MTU_SIZE - TCP_HEADER_OFFSET - TCP_HEADER_LENGTH)

#define TCP_MAKE_HEADER_LENGTH(HeaderLengthInDWORDs) ((HeaderLengthInDWORDs) << 12)
#define TCP_GET_HEADER_LENGTH(Flags) (((Flags) >> 12) & 0x0F)
#define TCP_HEADER_FLAG_URG (1 << 5)
#define TCP_HEADER_FLAG_ACK (1 << 4)
#define TCP_HEADER_FLAG_PSH (1 << 3)
#define TCP_HEADER_FLAG_RST (1 << 2)
#define TCP_HEADER_FLAG_SYN (1 << 1)
#define TCP_HEADER_FLAG_FIN (1 << 0)

typedef struct _TCPHeader
{
	uint16_t SrcPort;
	uint16_t DestPort;
	uint32_t SequenceNumber;
	uint32_t AcknowledgementNumber;
	uint16_t Flags;
	uint16_t Window;
	uint16_t Checksum;
	uint16_t UrgentPtr;
} TCPHeader;

#endif //IMPLEMENT_TCP

// -----------------------------------------------------------------------------------------------
// -------------------------------------- Global Variables ---------------------------------------
// -----------------------------------------------------------------------------------------------
/// Our IP address
static uint32_t ethernet_IPAddress;
/// Our net mask
static uint32_t ethernet_NetMask;
/// Our network's router IP
static uint32_t ethernet_RouterIP;
/// The read-and-write packet buffer
static uint8_t ethernet_PacketBuffer[MTU_SIZE+1];
/// The IP packet counter
static uint16_t ethernet_IP_IDCounter;
/// Indicates that one second has elapsed
volatile bool ethernet_SecondElapsed;

#ifdef USE_INTERRUPTS
/// If set, indicates that an interrupt came from the controller
volatile bool ethernet_InterruptOccurred;

#	ifdef HANDLE_LINK_STATUS_CHANGES
static ethernet_callback_link_status_change ethernet_link_status_change;
#	endif //HANDLE_LINK_STATUS_CHANGES
#endif //USE_INTERRUPTS

#ifdef IMPLEMENT_UDP
/// The socket to which the current UDP packet will be sent
static UDPSocket ethernet_CurrentPacketUDPSocket;
#endif //IMPLEMENT_UDP

#ifdef IMPLEMENT_TCP
/// The socket to which the current TCP packet will be sent
static TCPSocket ethernet_CurrentPacketTCPSocket;
#endif //IMPLEMENT_TCP


// -----------------------------------------------------------------------------------------------
// ----------------------------- Internal Function Implementations -------------------------------
// -----------------------------------------------------------------------------------------------
/**
 * Calculates the checksum for a given chunk of data
 * @remark Only for internal use!
 * @param Buffer The Buffer holding the data
 * @param BufferSize Buffer's size
 * @param StartValue The initial checksum (of previous chunks)
 * @return The calculated checksum
 */
uint16_t _ethernet_calculate_checksum(const uint8_t* Buffer,uint16_t BufferSize,uint32_t StartValue)
{
	uint8_t DataH, DataL;

	while(BufferSize > 1){
		DataH = *Buffer++;
		DataL = *Buffer++;
		StartValue += (((uint16_t)DataH) << 8) | DataL;
		BufferSize -= 2;
	}

	if(BufferSize > 0){
		DataH = *Buffer;
		StartValue += ((uint16_t)DataH) << 8;
	}

	// build complement
	StartValue = (StartValue & 0x0000FFFF) + ((StartValue & 0xFFFF0000) >> 16);
	StartValue = (StartValue & 0x0000FFFF) + ((StartValue & 0xFFFF0000) >> 16);

	return ~(StartValue & 0x0000FFFF);
}

/**
 * Gets the IP to look up (taking care of routing) in the ARP table for any given IP address
 * @remark Only for internal use!
 * @param IP The IP address
 * @return The IP address to look up in the ARP table
 */
uint32_t _ethernet_get_arp_table_ip(uint32_t IP)
{
	// If the target IP is not within our subnet, we need to send the packet to our router
	if((IP & ethernet_NetMask) != (ethernet_IPAddress & ethernet_NetMask))
		return ethernet_RouterIP;
	return IP;
}

/**
 * Prepares the Ethernet Header of a packet to be sent
 * @remark Only for internal use!
 * @param DestIP The destination IP of the packet
 */
void _ethernet_prepare_ethernet_header(uint32_t DestIP)
{
	EthernetHeader* eth_hdr = (EthernetHeader*)(&ethernet_PacketBuffer[ETHERNET_HEADER_OFFSET]);

	// Source MAC address
	const uint8_t* mac_addr = enc28j60_get_mac_address();
	for(uint8_t i = 0; i < MAC_ADDRESS_LENGTH; ++i)
		eth_hdr->Src[i] = mac_addr[i];
	
	// Destination MAC address
	if(DestIP != MAKE_IP(255,255,255,255)){
		const ARPTableEntry* arp_entry = arp_table_get(_ethernet_get_arp_table_ip(DestIP));
		if(arp_entry){
			for(uint8_t i = 0; i < MAC_ADDRESS_LENGTH; ++i)
				eth_hdr->Dest[i] = arp_entry->MAC[i];
		}else{
			for(uint8_t i = 0; i < MAC_ADDRESS_LENGTH; ++i)
				eth_hdr->Dest[i] = 0xFF;
		}
	}else{
		for(uint8_t i = 0; i < MAC_ADDRESS_LENGTH; ++i)
			eth_hdr->Dest[i] = 0xFF;
	}
}

/**
 * Prepares the IP Header of a packet to be sent
 * @remark Only for internal use!
 * @param DestIP The destination IP of the packet
 */
void _ethernet_prepare_ip_header(uint32_t DestIP)
{
	EthernetHeader* eth_hdr = (EthernetHeader*)(&ethernet_PacketBuffer[ETHERNET_HEADER_OFFSET]);
	IPHeader* ip_hdr = (IPHeader*)(&ethernet_PacketBuffer[IP_HEADER_OFFSET]);

	// Prepare the Ethernet Header
	_ethernet_prepare_ethernet_header(DestIP);
	eth_hdr->PacketType = HTONS(ETHERNET_PACKET_TYPE_IP);

	// Prepare the IP Header
	ip_hdr->FragOffset = 0x0040; // no fragmentation
	ip_hdr->TTL = 128;
	ip_hdr->ID = HTONS(ethernet_IP_IDCounter);
	++ethernet_IP_IDCounter;
	ip_hdr->VersLen = 0x45;
	ip_hdr->TOS = 0;
	ip_hdr->DestAddr = DestIP;
	ip_hdr->SrcAddr = ethernet_IPAddress;
	ip_hdr->HdrCksum = 0;

	// Generate its checksum
	uint16_t Length = (ip_hdr->VersLen & 0x0F) << 2;
	ip_hdr->HdrCksum = HTONS(_ethernet_calculate_checksum((const uint8_t*)ip_hdr,Length,0));
}

#ifdef IMPLEMENT_ICMP
/**
 * Sends an ICMP packet
 * @remark Only for internal use!
 * @param DestIP The destination IP of the packet
 * @param ICMP_Type The ICMP packet type (see ICMPHeader for more information)
 * @param ICMP_Code The ICMP packet code (see ICMPHeader for more information)
 * @param ICMP_SN The ICMP sequence number (see ICMPHeader for more information)
 * @param ICMP_ID The ICMP ID number (see ICMPHeader for more information)
 * @param Length The packet length
 */
void _ethernet_send_icmp_packet(uint32_t DestIP,uint8_t ICMP_Type,uint8_t ICMP_Code,uint16_t ICMP_SN,uint16_t ICMP_ID,uint16_t Length)
{
	IPHeader* ip_hdr = (IPHeader*)(&ethernet_PacketBuffer[IP_HEADER_OFFSET]);
	ICMPHeader* icmp_hdr = (ICMPHeader*)(&ethernet_PacketBuffer[ICMP_HEADER_OFFSET]);

	size_t len = Length + IP_HEADER_LENGTH + ICMP_HEADER_LENGTH;

	icmp_hdr->Type = ICMP_Type;
	icmp_hdr->Code = ICMP_Code;
	icmp_hdr->ID = HTONS(ICMP_ID);
	icmp_hdr->SeqNum = HTONS(ICMP_SN);
	icmp_hdr->Cksum = 0;
	ip_hdr->PktLen = HTONS(len);
	ip_hdr->Proto = IP_PROTOCOL_ICMP;
	
	_ethernet_prepare_ip_header(DestIP);

	uint16_t Checksum = _ethernet_calculate_checksum((const uint8_t*)icmp_hdr,len - ((ip_hdr->VersLen & 0x0F) << 2),0);
	icmp_hdr->Cksum = HTONS(Checksum);

	enc28j60_send(ethernet_PacketBuffer,Length + ICMP_HEADER_LENGTH + IP_HEADER_LENGTH + ETHERNET_HEADER_LENGTH);
}

/**
 * Handles a received ICMP packet
 * @remark Only for internal use!
 * @param PacketLength The length of the packet in bytes
 */
void _ethernet_handle_packet_icmp(size_t PacketLength)
{
	IPHeader* ip_hdr = (IPHeader*)(&ethernet_PacketBuffer[IP_HEADER_OFFSET]);
	ICMPHeader* icmp_hdr = (ICMPHeader*)(&ethernet_PacketBuffer[ICMP_HEADER_OFFSET]);

	switch(icmp_hdr->Type){
		// Ping reply (ignored as we don't send pings)
		case 0x00:
			break;
		// Ping request
		case 0x08:
			_ethernet_send_icmp_packet(ip_hdr->SrcAddr,0x00,0x00,NTOHS(icmp_hdr->SeqNum),NTOHS(icmp_hdr->ID),NTOHS(ip_hdr->PktLen) - IP_HEADER_LENGTH - ICMP_HEADER_LENGTH);
			break;
	}
}
#endif //IMPLEMENT_ICMP

#ifdef IMPLEMENT_UDP
/**
 * Prepares the UDP header of a packet
 * @remark Only for internal use!
 * @param Socket The socket of the UDP connection this packet will be sent to
 * @param Length The length of the packet data excluding any header
 */
bool _ethernet_prepare_udp_header(UDPSocket Socket, size_t Length)
{
	// Find the UDP table entry of this connection
	const UDPTableEntry* udp_entry = udp_table_get_by_socket(Socket);
	if(!udp_entry)
		return false;

	// Prepare the packet's IP header
	IPHeader* ip_hdr = (IPHeader*)(&ethernet_PacketBuffer[IP_HEADER_OFFSET]);
	ip_hdr->PktLen = HTONS(Length + IP_HEADER_LENGTH + UDP_HEADER_LENGTH);
	ip_hdr->Proto = IP_PROTOCOL_UDP;
	_ethernet_prepare_ip_header(udp_entry->RemoteIP);

	// Prepare the packet's UDP header
	UDPHeader* udp_hdr = (UDPHeader*)(&ethernet_PacketBuffer[UDP_HEADER_OFFSET]);
	udp_hdr->SrcPort = HTONS(udp_entry->LocalPort);
	udp_hdr->DestPort = HTONS(udp_entry->RemotePort);
	udp_hdr->Length = HTONS(Length + UDP_HEADER_LENGTH);
	udp_hdr->Checksum = 0; //TODO

	return true;
}

/**
 * Handles a received UDP packet
 * @remark Only for internal use!
 * @param PacketLength The length of the packet in bytes
 */
void _ethernet_handle_packet_udp(size_t PacketLength)
{
	IPHeader* ip_hdr = (IPHeader*)(&ethernet_PacketBuffer[IP_HEADER_OFFSET]);
	UDPHeader* udp_hdr = (UDPHeader*)(&ethernet_PacketBuffer[UDP_HEADER_OFFSET]);

	// Find the application listening on the local port
	const UDPApplication* udp_app = udp_get_port_application(NTOHS(udp_hdr->DestPort));
	if(!udp_app)
		return;

	// Find the UDP table entry of this connection
	const UDPTableEntry* udp_entry = udp_table_get(ip_hdr->SrcAddr,NTOHS(udp_hdr->DestPort),NTOHS(udp_hdr->SrcPort));

	// If there is no entry yet, this probably is a new connection
	if(!udp_entry){
		UDPSocket sock = _udp_table_add(ip_hdr->SrcAddr,NTOHS(udp_hdr->DestPort),NTOHS(udp_hdr->SrcPort),udp_app->TimeoutValue,false);

		// If the UDP table is full, just ignore the connection
		if(!udp_table_is_valid_socket(sock))
			return;

		udp_entry = udp_table_get_by_socket(sock);

		// Invoke the open connection callback of our application
		if(udp_app->OpenConnectionCallback)
			udp_app->OpenConnectionCallback(udp_entry->Socket,udp_entry->RemoteIP);

		// Check if the socket is still valid (if the user closed the connection in his open callback, it wouldn't be)
		if(!udp_table_is_valid_socket(sock))
			return;
	}

	// Invoke the packet handler callback of our application
	udp_app->HandlePacketCallback(udp_entry->Socket,&ethernet_PacketBuffer[UDP_DATA_OFFSET],NTOHS(udp_hdr->Length) - UDP_HEADER_LENGTH);
}
#endif //IMPLEMENT_UDP

#ifdef IMPLEMENT_TCP
/**
 * Prepares the TCP header of a packet and sends it
 * @remark Only for internal use!
 * @param Socket The socket of the TCP connection this packet will be sent to
 * @param Length The length of the packet data excluding any header
 * @param Flags The TCP header flags to send
 */
bool _ethernet_prepare_and_send_tcp_packet(size_t Length, uint16_t Flags, uint8_t AdditionalHeaderDWORDs)
{
	// Find the TCP table entry of this connection
	TCPTableEntry* tcp_entry = tcp_table_get_by_socket(ethernet_CurrentPacketTCPSocket);
	if(!tcp_entry)
		return false;

	// Prepare the packet's IP header
	IPHeader* ip_hdr = (IPHeader*)(&ethernet_PacketBuffer[IP_HEADER_OFFSET]);
	ip_hdr->PktLen = HTONS(Length + AdditionalHeaderDWORDs * sizeof(uint32_t) + IP_HEADER_LENGTH + TCP_HEADER_LENGTH);
	ip_hdr->Proto = IP_PROTOCOL_TCP;
	_ethernet_prepare_ip_header(tcp_entry->RemoteIP);

	// Prepare the packet's TCP header
	if(!tcp_entry->HasAcknowledgedLastPacket){
		tcp_entry->HasAcknowledgedLastPacket = true;
		Flags |= TCP_HEADER_FLAG_ACK;
	}

	if(Flags & TCP_HEADER_FLAG_PSH){
		if(tcp_entry->NeedsPushOnNextPacket)
			tcp_entry->NeedsPushOnNextPacket = false;
		else	
			Flags &= ~TCP_HEADER_FLAG_PSH;
	}


	TCPHeader* tcp_hdr = (TCPHeader*)(&ethernet_PacketBuffer[TCP_HEADER_OFFSET]);
	tcp_hdr->SrcPort = HTONS(tcp_entry->LocalPort);
	tcp_hdr->DestPort = HTONS(tcp_entry->RemotePort);
	tcp_hdr->Flags = TCP_MAKE_HEADER_LENGTH((TCP_HEADER_LENGTH / sizeof(uint32_t)) + AdditionalHeaderDWORDs) | Flags;
	tcp_hdr->Flags = HTONS(tcp_hdr->Flags);
	tcp_hdr->SequenceNumber = HTONL(tcp_entry->LastSequenceNumber);
	tcp_entry->LastSequenceNumber += Length;
	if(Flags & TCP_HEADER_FLAG_ACK)
		tcp_hdr->AcknowledgementNumber = HTONL(tcp_entry->LastAcknowledgementNumber);
	else
		tcp_hdr->AcknowledgementNumber = 0;
	tcp_hdr->UrgentPtr = 0;
	tcp_hdr->Window = HTONS(MAX_TCP_WINDOW_SIZE);
	
	uint16_t len = NTOHS(ip_hdr->PktLen) + 8 - ((ip_hdr->VersLen & 0x0F) << 2);
	tcp_hdr->Checksum = 0;
	tcp_hdr->Checksum = _ethernet_calculate_checksum((const uint8_t*)(&ip_hdr->SrcAddr),len,len-2);
	tcp_hdr->Checksum = HTONS(tcp_hdr->Checksum);

	enc28j60_send(ethernet_PacketBuffer,Length + AdditionalHeaderDWORDs * sizeof(uint32_t) + TCP_HEADER_OFFSET + TCP_HEADER_LENGTH);
	return true;
}

/**
 * Removes a TCP connection
 * @remark Only for internal use!
 * @param Socket The socket of the connection to remove
 */
void _ethernet_remove_tcp_connection(TCPSocket Socket)
{
	const TCPTableEntry* tcp_entry = tcp_table_get_by_socket(Socket);
	if(tcp_entry){
		// Stop listening on our local reception port if required
		if(tcp_entry->ClosePortOnTermination && tcp_entry->LocalPort != 0)
			tcp_close_port(tcp_entry->LocalPort);

		// Remove the connection
		_tcp_table_remove(Socket);
	}
}

/**
 * Handles a received TCP packet
 * @remark Only for internal use!
 * @param PacketLength The length of the packet in bytes
 */
void _ethernet_handle_packet_tcp(size_t PacketLength)
{
	IPHeader* ip_hdr = (IPHeader*)(&ethernet_PacketBuffer[IP_HEADER_OFFSET]);
	TCPHeader* tcp_hdr = (TCPHeader*)(&ethernet_PacketBuffer[TCP_HEADER_OFFSET]);
	tcp_hdr->Flags = NTOHS(tcp_hdr->Flags);

	const TCPApplication* tcp_app = tcp_get_port_application(NTOHS(tcp_hdr->DestPort));
	if(!tcp_app)
		return;

	// Find the TCP table entry of this connection
	TCPTableEntry* tcp_entry = tcp_table_get(ip_hdr->SrcAddr,NTOHS(tcp_hdr->DestPort),NTOHS(tcp_hdr->SrcPort));

	if(!tcp_entry){
		// A new incoming connection. Must be the start of the handshake!
		if(!(tcp_hdr->Flags & TCP_HEADER_FLAG_SYN))
			return;

		TCPSocket sock = _tcp_table_add(ip_hdr->SrcAddr,NTOHS(tcp_hdr->DestPort),NTOHS(tcp_hdr->SrcPort),tcp_app->TimeoutValue,false);

		// If the TCP table is full, just ignore the connection
		if(!tcp_table_is_valid_socket(sock))
			return;

		// Now find our entry
		tcp_entry = tcp_table_get_by_socket(sock);

		// Initialise it to handshake-state
		tcp_entry->ConnectionState = TCP_CONNECTION_STATE_HANDSHAKE_INCOMING;
		tcp_entry->LastAcknowledgementNumber = NTOHL(tcp_hdr->SequenceNumber) + 1;
		tcp_entry->LastSequenceNumber = rand32();

		// Reply with SYN|ACK (three-way-handshake) including the Maximum Segment Size Option (RFC 879)
		uint8_t* Buffer;
		size_t BufferSize;
		tcp_start_packet(tcp_entry->Socket,&Buffer,&BufferSize);
		Buffer[0] = 0x02;
		Buffer[1] = 0x04;
		*((uint16_t*)&Buffer[2]) = HTONS(MAX_TCP_WINDOW_SIZE);
		_ethernet_prepare_and_send_tcp_packet(0,TCP_HEADER_FLAG_SYN|TCP_HEADER_FLAG_ACK,1);

		++tcp_entry->LastSequenceNumber;
	}else{
		if(tcp_hdr->Flags & TCP_HEADER_FLAG_RST){
			// Request to reset the connection (we'll simply remove it)
			_ethernet_remove_tcp_connection(tcp_entry->Socket);
		}else{
			switch(tcp_entry->ConnectionState){
				case TCP_CONNECTION_STATE_HANDSHAKE_INCOMING:
				{
					// On an incoming handshake, the only thing we want to receive is an ACK telling us that the connection was established successfully!
					if(!(tcp_hdr->Flags & TCP_HEADER_FLAG_ACK))
						return;
					uint32_t SeqNum = NTOHL(tcp_hdr->SequenceNumber);
					uint32_t AckNum = NTOHL(tcp_hdr->AcknowledgementNumber);

					// Check if the ACK was meant for our SYN|ACK packet
					if(SeqNum == tcp_entry->LastAcknowledgementNumber && AckNum == tcp_entry->LastSequenceNumber){
						// If so, we've successfully established the connection!
						tcp_entry->ConnectionState = TCP_CONNECTION_STATE_CONNECTED;

						// Invoke the open connection callback of our application
						if(tcp_app->OpenConnectionCallback)
							tcp_app->OpenConnectionCallback(tcp_entry->Socket,tcp_entry->RemoteIP);
					}
					break;
				}
				case TCP_CONNECTION_STATE_HANDSHAKE_OUTGOING:
				{
					// On an outgoing handshake, we want to receive a SYN|ACK and reply with an ACK
					if(!(tcp_hdr->Flags & TCP_HEADER_FLAG_SYN))
						return;
					uint32_t AckNum = NTOHL(tcp_hdr->AcknowledgementNumber);

					// Check if this was a reply to our SYN packet
					if(!(tcp_hdr->Flags & TCP_HEADER_FLAG_ACK) || AckNum == tcp_entry->LastSequenceNumber){
						// On our side, the connection is already established!
						tcp_entry->LastAcknowledgementNumber = NTOHL(tcp_hdr->SequenceNumber) + 1;
						tcp_entry->ConnectionState = TCP_CONNECTION_STATE_CONNECTED;

						// However, we need to send back an ACK to tell our partner
						uint8_t* Buffer;
						size_t BufferSize;
						tcp_start_packet(tcp_entry->Socket,&Buffer,&BufferSize);
						_ethernet_prepare_and_send_tcp_packet(0,TCP_HEADER_FLAG_ACK,0);

						// Invoke the open connection callback of our application
						if(tcp_app->OpenConnectionCallback)
							tcp_app->OpenConnectionCallback(tcp_entry->Socket,tcp_entry->RemoteIP);
					}
					break;
				}
				case TCP_CONNECTION_STATE_TERMINATION_INCOMING:
				{
					// On an incoming teardown, the only thing we want to receive is an ACK telling us that the connection was terminated successfully!
					if(!(tcp_hdr->Flags & TCP_HEADER_FLAG_ACK))
						return;
					uint32_t SeqNum = NTOHL(tcp_hdr->SequenceNumber);
					uint32_t AckNum = NTOHL(tcp_hdr->AcknowledgementNumber);

					// Check if the ACK was meant for our SYN|ACK packet
					if(SeqNum == tcp_entry->LastAcknowledgementNumber && AckNum == tcp_entry->LastSequenceNumber){
						// If so, we've successfully terminated the connection!
						_ethernet_remove_tcp_connection(tcp_entry->Socket);
					}
					break;
				}
				case TCP_CONNECTION_STATE_TERMINATION_OUTGOING:
				{
					// On an outgoing teardown, we want to receive a FIN|ACK and reply with an ACK
					if(!(tcp_hdr->Flags & TCP_HEADER_FLAG_FIN))
						return;
					uint32_t AckNum = NTOHL(tcp_hdr->AcknowledgementNumber);

					// Check if this was a reply to our FIN packet
					if(AckNum == tcp_entry->LastSequenceNumber){
						// On our side, the connection is already terminated now! However, we need to send back an ACK to tell our partner
						tcp_entry->LastAcknowledgementNumber = NTOHL(tcp_hdr->SequenceNumber) + 1;

						uint8_t* Buffer;
						size_t BufferSize;
						tcp_start_packet(tcp_entry->Socket,&Buffer,&BufferSize);
						_ethernet_prepare_and_send_tcp_packet(0,TCP_HEADER_FLAG_ACK,0);

						// Remove the connection
						_ethernet_remove_tcp_connection(tcp_entry->Socket);
					}
					break;
				}
				case TCP_CONNECTION_STATE_CONNECTED:
				{
					if(tcp_hdr->Flags & TCP_HEADER_FLAG_ACK)
						tcp_entry->NeedsPushOnNextPacket = true;

					if(tcp_hdr->Flags & TCP_HEADER_FLAG_FIN){
						// Request from our partner to terminate the connection
						tcp_entry->ConnectionState = TCP_CONNECTION_STATE_TERMINATION_INCOMING;
						tcp_entry->LastAcknowledgementNumber = NTOHL(tcp_hdr->SequenceNumber) + 1;

						// Invoke the close connection callback of our application
						if(tcp_app->CloseConnectionCallback)
							tcp_app->CloseConnectionCallback(tcp_entry->Socket);

						// Reply with FIN|ACK (three-way-teardown)
						uint8_t* Buffer;
						size_t BufferSize;
						tcp_start_packet(tcp_entry->Socket,&Buffer,&BufferSize);
						_ethernet_prepare_and_send_tcp_packet(0,TCP_HEADER_FLAG_FIN|TCP_HEADER_FLAG_ACK,0);

						++tcp_entry->LastSequenceNumber;
					}else if(tcp_hdr->Flags & TCP_HEADER_FLAG_PSH){
						// The PSH flag is set, so there's data for our application
						size_t header_len = TCP_GET_HEADER_LENGTH(tcp_hdr->Flags) * sizeof(uint32_t);
						size_t data_len = NTOHS(ip_hdr->PktLen) - IP_HEADER_LENGTH - header_len;
						tcp_entry->LastAcknowledgementNumber = NTOHL(tcp_hdr->SequenceNumber) + data_len;
						tcp_entry->HasAcknowledgedLastPacket = false;

						// Execute the callback
						if(tcp_app->HandlePacketCallback)
							tcp_app->HandlePacketCallback(tcp_entry->Socket,&ethernet_PacketBuffer[TCP_HEADER_OFFSET + header_len],data_len);
			
						// If the user has sent at least one packet in response, an ACK has been transferred with it automatically. Otherwise, we need to send one manually
						if(tcp_entry->ConnectionState == TCP_CONNECTION_STATE_CONNECTED && !tcp_entry->HasAcknowledgedLastPacket){
							uint8_t* Buffer;
							size_t BufferSize;
							tcp_start_packet(tcp_entry->Socket,&Buffer,&BufferSize);
							_ethernet_prepare_and_send_tcp_packet(0,TCP_HEADER_FLAG_ACK,0);
						}
					}
					break;
				}
				default:
					break;
			}
		}
	}
}
#endif //IMPLEMENT_TCP

/**
 * Analyses a received IP packet and dispatches it to the correct handlers
 * @remark Only for internal use!
 * @param PacketLength The length of the packet in bytes
 */
void _ethernet_handle_packet_ip(size_t PacketLength)
{
	EthernetHeader* eth_hdr = (EthernetHeader*)(&ethernet_PacketBuffer[ETHERNET_HEADER_OFFSET]);
	IPHeader* ip_hdr = (IPHeader*)(&ethernet_PacketBuffer[IP_HEADER_OFFSET]);

	// Check if the packet is meant for us
	bool isForUs = (ip_hdr->DestAddr == ethernet_IPAddress);

	// If we're	not directly addressed, it might still be a broadcasted packet
	if(!isForUs){
		// Check if the packet is a broadcasted packet
		bool isBroadcast = true;
		for(uint8_t i = 0; i < MAC_ADDRESS_LENGTH; ++i)
			isBroadcast &= (eth_hdr->Dest[i] == 0xFF);

		// Check if the protocol allows broadcasting
		switch(ip_hdr->Proto){
#ifdef IMPLEMENT_UDP
			case IP_PROTOCOL_UDP:
				isForUs = true;
				break;
#endif //IMPLEMENT_UDP
		}
	}

	// If it's for us, handle it
	if(isForUs){
		arp_table_add(eth_hdr->Src,_ethernet_get_arp_table_ip(ip_hdr->SrcAddr));

		switch(ip_hdr->Proto){
#ifdef IMPLEMENT_ICMP
			case IP_PROTOCOL_ICMP:
				_ethernet_handle_packet_icmp(PacketLength);
				break;
#endif //IMPLEMENT_ICMP

#ifdef IMPLEMENT_TCP
			case IP_PROTOCOL_TCP:
				_ethernet_handle_packet_tcp(PacketLength);
				break;
#endif //IMPLEMENT_TCP

#ifdef IMPLEMENT_UDP
			case IP_PROTOCOL_UDP:
				_ethernet_handle_packet_udp(PacketLength);
				break;
#endif //IMPLEMENT_UDP
		}
	}
}

/**
 * Sends an ARP request packet for a given IP address
 * @remark Only for internal use!
 * @param IP The IP address to request
 */
void _ethernet_send_arp_request(uint32_t IP)
{
	EthernetHeader* eth_hdr = (EthernetHeader*)(&ethernet_PacketBuffer[ETHERNET_HEADER_OFFSET]);
	ARPHeader* arp_hdr = (ARPHeader*)(&ethernet_PacketBuffer[ARP_HEADER_OFFSET]);

	_ethernet_prepare_ethernet_header(IP);
	eth_hdr->PacketType = HTONS(ETHERNET_PACKET_TYPE_ARP);

	const uint8_t* mac_addr = enc28j60_get_mac_address();
	for(uint8_t i = 0; i < MAC_ADDRESS_LENGTH; ++i){
		arp_hdr->THAddr[i] = 0x00;
		arp_hdr->SHAddr[i] = mac_addr[i];
	}
	arp_hdr->HWType = HTONS(HARDWARE_TYPE_ETHERNET);
	arp_hdr->PRType = HTONS(ETHERNET_PACKET_TYPE_IP);
	arp_hdr->HWLen = MAC_ADDRESS_LENGTH;
	arp_hdr->PRLen = IP_ADDRESS_LENGTH;
	arp_hdr->Opcode = HTONS(ARP_OPCODE_REQUEST);
	arp_hdr->TIPAddr = IP;
	arp_hdr->SIPAddr = ethernet_IPAddress;

	enc28j60_send(ethernet_PacketBuffer,ARP_HEADER_OFFSET + ARP_HEADER_LENGTH);
}

/**
 * Ensures a given IP address is stored in the ARP table
 * @remark Only for internal use!
 * @param IP The IP address
 * @param Timeout The timeout (in milliseconds) until the operation will be aborted)
 * @return True if everything went fine and the IP address is now stored in the ARP table, false otherwise
 */
bool _ethernet_ensure_arp_entry_exists(uint32_t IP, uint16_t Timeout)
{
	IP = _ethernet_get_arp_table_ip(IP);

	// Check if there is any slot left in the ARP table
	if(arp_table_is_full() && !arp_table_get(IP))
		return false;

	uint16_t Timer = 0;
	while(Timer <= Timeout){
		// Check if the entry is there
		if(arp_table_get(IP))
			return true;

		// Send a new ARP request every 500ms (in case the previous one got lost)
		if((Timer % 500) == 0)
			_ethernet_send_arp_request(IP);

		// Update
		ethernet_update();

		// Wait 1ms and continue
		_delay_ms(1);
		++Timer;
	}

	return false;
}

/**
 * Handles a received ARP packet
 * @remark Only for internal use!
 * @param PacketLength The length of the packet in bytes
 */
void _ethernet_handle_packet_arp(size_t PacketLength)
{
	EthernetHeader* eth_hdr = (EthernetHeader*)(&ethernet_PacketBuffer[ETHERNET_HEADER_OFFSET]);
	ARPHeader* arp_hdr = (ARPHeader*)(&ethernet_PacketBuffer[ARP_HEADER_OFFSET]);

	// Check if we're the target of the packet
	if(arp_hdr->TIPAddr != ethernet_IPAddress)
		return;

	// Check if the ARP packet's
	// -> Hardware Type is Ethernet
	// -> Protocol Type is IP
	// -> Hardware-Address length is MAC_ADDRESS_LENGTH
	// -> Protocol-Address length is IP_ADDRESS_LENGTH
	if(arp_hdr->HWType == HTONS(HARDWARE_TYPE_ETHERNET) && arp_hdr->PRType == HTONS(ETHERNET_PACKET_TYPE_IP) && arp_hdr->HWLen == MAC_ADDRESS_LENGTH && arp_hdr->PRLen == IP_ADDRESS_LENGTH){
		arp_table_add(arp_hdr->SHAddr,arp_hdr->SIPAddr);
		switch(arp_hdr->Opcode){
			// ARP request
			case HTONS(ARP_OPCODE_REQUEST):
			{
				_ethernet_prepare_ethernet_header(arp_hdr->SIPAddr);
				eth_hdr->PacketType = HTONS(ETHERNET_PACKET_TYPE_ARP);

				const ARPTableEntry* arp_entry = arp_table_get(arp_hdr->SIPAddr);
				if(arp_entry){
					const uint8_t* mac_addr = enc28j60_get_mac_address();
					for(uint8_t i = 0; i < MAC_ADDRESS_LENGTH; ++i){
						arp_hdr->THAddr[i] = arp_entry->MAC[i];
						arp_hdr->SHAddr[i] = mac_addr[i];
					}
				}

				arp_hdr->Opcode = HTONS(ARP_OPCODE_REPLY);
				arp_hdr->TIPAddr = arp_hdr->SIPAddr;
				arp_hdr->SIPAddr = ethernet_IPAddress;

				enc28j60_send(ethernet_PacketBuffer,ARP_HEADER_OFFSET + ARP_HEADER_LENGTH);
				break;
			}
			// ARP reply
			case HTONS(ARP_OPCODE_REPLY):
			{
				break;
			}
		}
	}
}

/**
 * Analyses a received ethernet packet and dispatches it to the correct handlers
 * @remark Only for internal use!
 * @param PacketLength The length of the packet in bytes
 */
void _ethernet_handle_packet(size_t PacketLength)
{
	EthernetHeader* eth_hdr = (EthernetHeader*)(&ethernet_PacketBuffer[ETHERNET_HEADER_OFFSET]);

	switch(eth_hdr->PacketType)
	{
		// IP
		case HTONS(ETHERNET_PACKET_TYPE_IP):
		{
			_ethernet_handle_packet_ip(PacketLength);
			break;
		}
		// ARP
		case HTONS(ETHERNET_PACKET_TYPE_ARP):
		{
			_ethernet_handle_packet_arp(PacketLength);
			break;
		}
	}
}

/**
 * Internal base initialisation routine
 * @remark Only for internal use!
 */
void _ethernet_initialise_impl(void)
{
	ethernet_IP_IDCounter = 0;
	ethernet_IPAddress = 0;
	ethernet_NetMask = 0;
	ethernet_RouterIP = 0;
	ethernet_SecondElapsed = false;

#ifdef USE_INTERRUPTS
	ethernet_InterruptOccurred = false;
#endif //USE_INTERRUPTS

	// Initialise ARP
	arp_table_initialise();

#ifdef IMPLEMENT_UDP
	// Initialise UDP
	_udp_initialise();
	ethernet_CurrentPacketUDPSocket = INVALID_UDP_SOCKET;
#endif //IMPLEMENT_UDP

#ifdef IMPLEMENT_TCP
	// Initialise TCP
	_tcp_initialise();
	ethernet_CurrentPacketTCPSocket = INVALID_TCP_SOCKET;
#endif //IMPLEMENT_TCP
}

// -----------------------------------------------------------------------------------------------
// ----------------------------- External Function Implementations -------------------------------
// -----------------------------------------------------------------------------------------------
void ethernet_initialise(uint32_t IPAddress,uint32_t NetMask,uint32_t RouterIP)
{
	_ethernet_initialise_impl();

	ethernet_IPAddress = IPAddress;
	ethernet_NetMask = NetMask;
	ethernet_RouterIP = RouterIP;
}

#ifdef IMPLEMENT_DHCP
void ethernet_initialise_dhcp(const char* Hostname,uint16_t Timeout)
{
	_ethernet_initialise_impl();
	_ethernet_configure_via_dhcp(Hostname,Timeout);
}
#endif //IMPLEMENT_DHCP

void ethernet_deinitialise(void)
{
#ifdef IMPLEMENT_DHCP
	dhcp_release();
#endif //IMPLEMENT_DHCP
}

void ethernet_second_tick(void)
{
	ethernet_SecondElapsed = true;
}

void ethernet_update(void)
{
	if(ethernet_SecondElapsed){
		ethernet_SecondElapsed = false;

		arp_table_second_tick();
#ifdef IMPLEMENT_DHCP
		dhcp_second_tick();
#endif // IMPLEMENT_DHCP
#ifdef IMPLEMENT_NTP
		ntp_second_tick();
#endif // IMPLEMENT_NTP
	}

#ifdef USE_INTERRUPTS
	if(ethernet_InterruptOccurred){
		// Disable interrupts so we don't miss any that could occur while handling them
		enc28j60_disable_interrupts();

#	ifdef HANDLE_LINK_STATUS_CHANGES
		if(enc28j60_has_link_status_interrupt()){
			// Invoke the callback handler if there is any
			if(ethernet_link_status_change)
				ethernet_link_status_change(ethernet_get_link_status());
		}
#	endif //HANDLE_LINK_STATUS_CHANGES
#endif //USE_INTERRUPTS

		// Receive all queued packets
		while(enc28j60_has_packet_interrupt()){
			uint16_t PacketLength = enc28j60_receive(ethernet_PacketBuffer,MTU_SIZE);
			if(PacketLength == 0)
				break;
			ethernet_PacketBuffer[PacketLength] = 0x00;
			_ethernet_handle_packet(PacketLength);
		}

#ifdef USE_INTERRUPTS
		ethernet_InterruptOccurred = false;
		enc28j60_enable_interrupts();
	}
#endif //USE_INTERRUPTS
}

#ifdef USE_INTERRUPTS
void ethernet_data_interrupt(void)
{
	ethernet_InterruptOccurred = true;
}
#endif //USE_INTERRUPTS

bool ethernet_get_link_status(void)
{
	return enc28j60_get_link_status();
}

bool ethernet_wait_for_link_status(uint16_t Timeout)
{
	uint16_t Timer = 0;
	while(Timeout == 0 || Timer <= Timeout){
		if(ethernet_get_link_status())
			return true;

		_delay_ms(1);
		++Timer;
	}
	return false;
}

#ifdef HANDLE_LINK_STATUS_CHANGES
void ethernet_set_link_status_change_callback(ethernet_callback_link_status_change NewCallback)
{
	ethernet_link_status_change = NewCallback;
}
#endif //HANDLE_LINK_STATUS_CHANGES

void _ethernet_set_ip_netmask_router(uint32_t IP, uint32_t NetMask, uint32_t RouterIP)
{
	ethernet_IPAddress = IP;
	ethernet_NetMask = NetMask;
	ethernet_RouterIP = RouterIP;
}

#ifdef IMPLEMENT_DHCP
bool _ethernet_configure_via_dhcp(const char* Hostname, uint16_t Timeout)
{
    uint32_t start_time = millis;
    
	// Wait until we're connected
	ethernet_wait_for_link_status(Timeout);

	// Wait until we've got a valid DHCP configuration
	bool HasNewConfig = false;
	uint32_t IP, NetMask, RouterIP;
	while(!HasNewConfig && ((millis - start_time) < Timeout))
		HasNewConfig = dhcp_request(Hostname,Timeout,&IP,&NetMask,&RouterIP,NULL,NULL);

	// Update our data
	if(HasNewConfig){
		ethernet_IPAddress = IP;
		ethernet_NetMask = NetMask;
		ethernet_RouterIP = RouterIP;
	}else{
		ethernet_IPAddress = 0;
		ethernet_NetMask = 0;
		ethernet_RouterIP = 0;
	}

	return HasNewConfig;
}
#endif //IMPLEMENT_DHCP

uint32_t ethernet_get_ip(void)
{
	return ethernet_IPAddress;
}

uint32_t ethernet_get_netmask(void)
{
	return ethernet_NetMask;
}

uint32_t ethernet_get_router_ip(void)
{
	return ethernet_RouterIP;
}

#ifdef IMPLEMENT_UDP
UDPSocket udp_connect(uint32_t IP, uint16_t Port, uint16_t Timeout, UDPCallbackHandlePacket HandlePacketCallback)
{
	return udp_connect_ex(IP,Port,Timeout,HandlePacketCallback,0);
}

UDPSocket udp_connect_ex(uint32_t IP, uint16_t Port, uint16_t Timeout, UDPCallbackHandlePacket HandlePacketCallback, uint16_t LocalPort)
{
	// If we want to receive packets, we need a local port on which we'll listen for answers
	if(HandlePacketCallback){
		// Check if a port is given and free
		if(LocalPort != 0 && udp_get_port_application(LocalPort))
			return INVALID_UDP_SOCKET;

		// If no local port was specified, we need a choose a random one
		while(LocalPort == 0 || udp_get_port_application(LocalPort))
			LocalPort = rand();

		// Start listening on that port
		if(!udp_open_port(LocalPort,Timeout,NULL,NULL,HandlePacketCallback))
			return INVALID_UDP_SOCKET;
	}else{
		LocalPort = 0;
	}

	// Add the connection
	UDPSocket sock = _udp_table_add(IP,LocalPort,Port,Timeout,true);

	// If the connection isn't valid, stop listening instantly
	if(HandlePacketCallback && sock == INVALID_UDP_SOCKET)
		udp_close_port(LocalPort);

	// Ensure we have the remote IP in our ARP table
	if(sock != INVALID_UDP_SOCKET){
		if(IP != MAKE_IP(255,255,255,255)){
			if(!_ethernet_ensure_arp_entry_exists(IP,Timeout)){
				udp_disconnect(sock);
				return INVALID_UDP_SOCKET;
			}
		}
	}

	return sock;
}

void udp_disconnect(UDPSocket Socket)
{
	const UDPTableEntry* udp_entry = udp_table_get_by_socket(Socket);
	if(udp_entry){
		// Invoke the close connection callback
		const UDPApplication* udp_app = udp_get_port_application(udp_entry->LocalPort);
		if(udp_app){
			if(udp_app->CloseConnectionCallback)
				udp_app->CloseConnectionCallback(Socket);
		}

		// Stop listening on our local reception port if required
		if(udp_entry->ClosePortOnTermination && udp_entry->LocalPort != 0)
			udp_close_port(udp_entry->LocalPort);

		// Remove the connection
		_udp_table_remove(Socket);
	}
}

bool udp_start_packet(UDPSocket Socket, uint8_t** BufferPtr, size_t* BufferSize)
{
	ethernet_CurrentPacketUDPSocket = Socket;

	// Find the connection in our UDP table
	const UDPTableEntry* udp_entry = udp_table_get_by_socket(ethernet_CurrentPacketUDPSocket);
	if(!udp_entry)
		return false;

	// Refresh its ARP entry
	if(udp_entry->RemoteIP != MAKE_IP(255,255,255,255)){
		if(!_ethernet_ensure_arp_entry_exists(udp_entry->RemoteIP,udp_entry->TimeoutValue)){
			// If the refresh failed, we assume the connection to have timed out and the remote host to be no longer available
			udp_disconnect(ethernet_CurrentPacketUDPSocket);
			ethernet_CurrentPacketUDPSocket = INVALID_UDP_SOCKET;
			return false;
		}
	}

	*BufferPtr = &(ethernet_PacketBuffer[UDP_DATA_OFFSET]);
	*BufferSize = MTU_SIZE - UDP_DATA_OFFSET;

	return true;
}

void udp_send(size_t Length)
{
	if(!udp_table_is_valid_socket(ethernet_CurrentPacketUDPSocket))
		return;

	// Prepare the header
	if(!_ethernet_prepare_udp_header(ethernet_CurrentPacketUDPSocket,Length))
		return;

	// Send the packet
	enc28j60_send(ethernet_PacketBuffer,Length + UDP_HEADER_LENGTH + IP_HEADER_LENGTH + ETHERNET_HEADER_LENGTH);
}
#endif //IMPLEMENT_UDP

#ifdef IMPLEMENT_TCP
TCPSocket tcp_connect(uint32_t IP, uint16_t Port, uint16_t Timeout, TCPCallbackHandlePacket HandlePacketCallback)
{
	// Find an unused local port for packet reception
	uint16_t LocalPort = 0;

	// If we want to receive packets, choose a random local port on which we'll listen for answers
	while(LocalPort == 0 || tcp_get_port_application(LocalPort))
		LocalPort = rand();

	// Start listening on that port
	if(!tcp_open_port(LocalPort,Timeout,NULL,NULL,HandlePacketCallback))
		return INVALID_TCP_SOCKET;

	// Add the connection
	TCPSocket sock = _tcp_table_add(IP,LocalPort,Port,Timeout,true);

	// If the socket isn't valid, stop listening instantly
	if(LocalPort != 0 && sock == INVALID_TCP_SOCKET){
		tcp_close_port(LocalPort);
		return INVALID_TCP_SOCKET;
	}

	// Ensure we have the remote IP in our ARP table
	if(!_ethernet_ensure_arp_entry_exists(IP,Timeout)){
		_ethernet_remove_tcp_connection(sock);
		return INVALID_TCP_SOCKET;
	}

	// Start the handshake including the MSS option
	TCPTableEntry* tcp_entry = tcp_table_get_by_socket(sock);
	tcp_entry->ConnectionState = TCP_CONNECTION_STATE_HANDSHAKE_OUTGOING;
	tcp_entry->HasAcknowledgedLastPacket = true;
	tcp_entry->LastSequenceNumber = rand32();

	uint8_t* Buffer;
	size_t BufferSize;
	tcp_start_packet(tcp_entry->Socket,&Buffer,&BufferSize);
	Buffer[0] = 0x02;
	Buffer[1] = 0x04;
	*((uint16_t*)&Buffer[2]) = HTONS(MAX_TCP_WINDOW_SIZE);
	_ethernet_prepare_and_send_tcp_packet(0,TCP_HEADER_FLAG_SYN,1);

	++tcp_entry->LastSequenceNumber;

	// Wait for the handshake to finish
	uint16_t Timer = 0;
	while(Timer <= Timeout){
		// Update
		ethernet_update();

		// Check if we have received an answer
		if(tcp_entry->ConnectionState != TCP_CONNECTION_STATE_HANDSHAKE_OUTGOING)
			break;

		// Wait 1ms and continue
		_delay_ms(1);
		++Timer;
	}

	// If the connection failed, remove it
	if(tcp_entry->ConnectionState != TCP_CONNECTION_STATE_CONNECTED){
		_ethernet_remove_tcp_connection(sock);
		return INVALID_TCP_SOCKET;
	}

	return sock;
}

void tcp_disconnect(TCPSocket Socket)
{
	TCPTableEntry* tcp_entry = tcp_table_get_by_socket(Socket);
	if(tcp_entry){
		// Inform the TCP application of the connection termination if required
		const TCPApplication* tcp_app = tcp_get_port_application(tcp_entry->LocalPort);
		if(tcp_app){
			if(tcp_app->CloseConnectionCallback)
				tcp_app->CloseConnectionCallback(tcp_entry->Socket);
		}

		// If there is an ACK pending, send it
		if(!tcp_entry->HasAcknowledgedLastPacket){
			uint8_t* Buffer;
			size_t BufferSize;
			tcp_start_packet(tcp_entry->Socket,&Buffer,&BufferSize);
			_ethernet_prepare_and_send_tcp_packet(0,TCP_HEADER_FLAG_ACK,0);
		}

		// Send the FIN request to our partner
		tcp_entry->ConnectionState = TCP_CONNECTION_STATE_TERMINATION_OUTGOING;
		uint8_t* Buffer;
		size_t BufferSize;
		tcp_start_packet(tcp_entry->Socket,&Buffer,&BufferSize);
		_ethernet_prepare_and_send_tcp_packet(0,TCP_HEADER_FLAG_FIN,0);
	}
}

bool tcp_start_packet(TCPSocket Socket, uint8_t** BufferPtr, size_t* BufferSize)
{
	ethernet_CurrentPacketTCPSocket = Socket;

	// Find the connection in our TCP table
	const TCPTableEntry* tcp_entry = tcp_table_get_by_socket(ethernet_CurrentPacketTCPSocket);
	if(!tcp_entry)
		return false;

	// Refresh its ARP entry
	if(!_ethernet_ensure_arp_entry_exists(tcp_entry->RemoteIP,tcp_entry->TimeoutValue)){
		// If the refresh failed, we assume the connection to have timed out and the remote host to be no longer available
		// Inform the TCP application of the connection termination if required
		const TCPApplication* tcp_app = tcp_get_port_application(tcp_entry->LocalPort);
		if(tcp_app){
			if(tcp_app->CloseConnectionCallback)
				tcp_app->CloseConnectionCallback(Socket);
		}

		// Forcefully remove the connection (we cannot even send the teardown anymore...)
		_ethernet_remove_tcp_connection(ethernet_CurrentPacketTCPSocket);
		ethernet_CurrentPacketTCPSocket = INVALID_TCP_SOCKET;
		return false;
	}

	*BufferPtr = &(ethernet_PacketBuffer[TCP_HEADER_OFFSET + TCP_HEADER_LENGTH]);
	*BufferSize = MTU_SIZE - TCP_HEADER_OFFSET - TCP_HEADER_LENGTH;

	return true;
}

void tcp_send(size_t Length)
{
	_ethernet_prepare_and_send_tcp_packet(Length,TCP_HEADER_FLAG_PSH|TCP_HEADER_FLAG_ACK,0);
}
#endif //IMPLEMENT_TCP
