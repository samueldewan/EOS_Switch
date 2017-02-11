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
#include "utils.h"
#include "enc28j60.h"
#include "ethernet.h"
#include "dhcp.h"

#include <util/delay.h>
#include <string.h>

#ifdef IMPLEMENT_DHCP

// -----------------------------------------------------------------------------------------------
// ------------------------------------- Constants / Structs -------------------------------------
// -----------------------------------------------------------------------------------------------
#define DHCP_LOCAL_PORT 68
#define DHCP_REMOTE_PORT 67

#define DHCP_OPCODE_REQUEST 0x01
#define DHCP_OPCODE_REPLY 0x02

#define DHCP_FLAG_BROADCAST 0x8000

#define DHCP_MAGIC_COOKIE 0x63825363
 
#define DHCP_MESSAGE_TYPE_DISCOVER 0x01
#define DHCP_MESSAGE_TYPE_OFFER 0x02
#define DHCP_MESSAGE_TYPE_REQUEST 0x03
#define DHCP_MESSAGE_TYPE_DECLINE 0x04
#define DHCP_MESSAGE_TYPE_ACK 0x05
#define DHCP_MESSAGE_TYPE_NAK 0x06
#define DHCP_MESSAGE_TYPE_RELEASE 0x07
#define DHCP_MESSAGE_TYPE_INFORM 0x08

#define DHCP_INVALID_TIMER 0xFFFFFFFF

typedef struct _DHCPHeader
{
	uint8_t Opcode;
	uint8_t HardwareType;
	uint8_t HardwareLength;
	uint8_t Hops;
	uint32_t TransactionID;
	uint16_t SecondsSinceStartup;
	uint16_t Flags;
	uint32_t ClientIP;
	uint32_t YourIP;
	uint32_t ServerIP;
	uint32_t GatewayIP;
	uint8_t ClientHardwareAddress[16];
	char ServerName[64];
	char File[128];
} DHCPHeader;

#define DHCP_HEADER_OFFSET 0
#define DHCP_HEADER_LENGTH sizeof(DHCPHeader)
#define DHCP_DATA_OFFSET (DHCP_HEADER_OFFSET + DHCP_HEADER_LENGTH)
#define DHCP_OPTIONS_OFFSET (DHCP_DATA_OFFSET + sizeof(uint32_t)) // Options start after the magic cookie


// -----------------------------------------------------------------------------------------------
// -------------------------------------- Global Variables ---------------------------------------
// -----------------------------------------------------------------------------------------------
static uint32_t dhcp_CurrentTransactionID;
static UDPSocket dhcp_CurrentSocket = INVALID_UDP_SOCKET;
static bool dhcp_DataValid = false;

static const char* dhcp_CurrentHostname;
static uint32_t dhcp_ServerIP;
static uint32_t dhcp_CurrentIP;
static uint32_t dhcp_LeaseTime;
static uint32_t dhcp_RenewalTime;
static uint32_t dhcp_RebindingTime;
static uint32_t dhcp_SubnetMask;
static uint32_t dhcp_RouterIP;
static uint32_t dhcp_DNSServerIP;
static uint32_t dhcp_NTPServerIP;

// -----------------------------------------------------------------------------------------------
// ----------------------------- Internal Function Implementations -------------------------------
// -----------------------------------------------------------------------------------------------
/**
 * Generates a new DHCP transaction ID
 * @remark Only for internal use!
 * @return A transaction ID
 */
uint32_t _dhcp_generate_transaction_id(void)
{
	return rand32();
}

/**
 * Invalidates our current configuration
 * @remark Only for internal use!
 */
void _dhcp_invalidate(void)
{
	dhcp_ServerIP = 0;
	dhcp_CurrentIP = 0;
	dhcp_LeaseTime = DHCP_INVALID_TIMER;
	dhcp_RenewalTime = DHCP_INVALID_TIMER;
	dhcp_RebindingTime = DHCP_INVALID_TIMER;
	dhcp_SubnetMask = 0;
	dhcp_RouterIP = 0;
	dhcp_DNSServerIP = 0;
	dhcp_NTPServerIP = 0;
	dhcp_DataValid = false;
}

/**
 * Prepares a BOOTP / DHCP packet header
 * @remark Only for internal use!
 * @param Buffer Pointer to the first byte of the packet
 * @param Opcode The DHCP operation code
 * @param TransactionID The ID of the current transaction
 * @param Flags The flags
 * @param ClientIP Our IP address
 * @param YourIP 
 * @param ServerIP The IP address of the DHCP server
 * @param GatewayIP The IP address of the DHCP gateway
 * @return The number of bytes written to Buffer
 */
size_t _dhcp_prepare_packet_header(uint8_t* Buffer, uint8_t Opcode, uint32_t TransactionID, uint16_t Flags, uint32_t ClientIP, uint32_t YourIP, uint32_t ServerIP, uint32_t GatewayIP)
{
	// Prepare the BOOTP header
	DHCPHeader* dhcp_hdr = (DHCPHeader*)(&Buffer[DHCP_HEADER_OFFSET]);
	dhcp_hdr->Opcode = Opcode;
	dhcp_hdr->HardwareType = HARDWARE_TYPE_ETHERNET;
	dhcp_hdr->HardwareLength = MAC_ADDRESS_LENGTH;
	dhcp_hdr->Hops = 0;
	dhcp_hdr->TransactionID = HTONL(TransactionID);
	dhcp_hdr->SecondsSinceStartup = HTONS(0);
	dhcp_hdr->Flags = HTONS(Flags);
	dhcp_hdr->ClientIP = ClientIP;
	dhcp_hdr->YourIP = YourIP;
	dhcp_hdr->ServerIP = ServerIP;
	dhcp_hdr->GatewayIP = GatewayIP;

	/// Write our MAC address including the padding zeros
	const uint8_t* mac_addr = enc28j60_get_mac_address();
	for(uint8_t i = 0; i < MAC_ADDRESS_LENGTH; ++i)
		dhcp_hdr->ClientHardwareAddress[i] = mac_addr[i];
	for(uint8_t i = MAC_ADDRESS_LENGTH; i < sizeof(dhcp_hdr->ClientHardwareAddress); ++i)
		dhcp_hdr->ClientHardwareAddress[i] = 0x00;

	/// Fill File with zeros
	for(uint8_t i = 0; i < sizeof(dhcp_hdr->File); ++i)
		dhcp_hdr->File[i] = 0x00;

	// Prepare the DHCP header
	SET_UINT32(Buffer,DHCP_DATA_OFFSET,HTONL(DHCP_MAGIC_COOKIE));

	return DHCP_HEADER_LENGTH + sizeof(uint32_t);
}

/**
 * Sends a DHCP server discovery packet
 * @remark Only for internal use!
 * @param Socket The socket to send the packet to
 * @param PreferredIP An IP address that we'd prefer to use. Can be 0 for no preferred IP
 * @param Hostname Our module's Hostname. Can be NULL for no hostname
 */
void _dhcp_send_discover(UDPSocket Socket, uint32_t PreferredIP, const char* Hostname)
{
	// Generate a new transaction ID
	dhcp_CurrentTransactionID = _dhcp_generate_transaction_id();

	uint8_t* Buffer;
	size_t BufferSize;
	udp_start_packet(Socket,&Buffer,&BufferSize);

	size_t Offset = _dhcp_prepare_packet_header(Buffer,DHCP_OPCODE_REQUEST,dhcp_CurrentTransactionID,DHCP_FLAG_BROADCAST,0,0,0,0);

	// DHCP discover
	SET_UINT8(Buffer,Offset,53);
	Offset += sizeof(uint8_t);

	SET_UINT8(Buffer,Offset,sizeof(uint8_t));
	Offset += sizeof(uint8_t);
	SET_UINT8(Buffer,Offset,DHCP_MESSAGE_TYPE_DISCOVER);
	Offset += sizeof(uint8_t);


	// Requested IP
	if(PreferredIP != 0){
		SET_UINT8(Buffer,Offset,50);
		Offset += sizeof(uint8_t);
		SET_UINT8(Buffer,Offset,sizeof(PreferredIP));
		Offset += sizeof(uint8_t);

		SET_UINT32(Buffer,Offset,PreferredIP);
		Offset += sizeof(uint32_t);
	}


	// Hostname
	if(Hostname){
		size_t Length = strlen(Hostname);

		SET_UINT8(Buffer,Offset,12);
		Offset += sizeof(uint8_t);
		SET_UINT8(Buffer,Offset,Length);
		Offset += sizeof(uint8_t);

		for(uint8_t i = 0; i < Length; ++i)
			Buffer[Offset++] = Hostname[i];
	}


	// Parameter Request List
	SET_UINT8(Buffer,Offset,55);
	Offset += sizeof(uint8_t);
	SET_UINT8(Buffer,Offset,3);
	Offset += sizeof(uint8_t);

	/// Subnet mask
	SET_UINT8(Buffer,Offset,1);
	Offset += sizeof(uint8_t);
	/// Router IP
	SET_UINT8(Buffer,Offset,3);
	Offset += sizeof(uint8_t);
	/// DNS server
	SET_UINT8(Buffer,Offset,6);
	Offset += sizeof(uint8_t);


	// End of options
	SET_UINT8(Buffer,Offset,0xFF);
	Offset += sizeof(uint8_t);


	udp_send(Offset);
}

/**
 * Sends a request to be allowed to use the offered configuration to our DHCP server
 * @remark Only for internal use!
 * @param Socket The socket to send the packet to
 * @param IP The IP address we want to use.
 * @param Hostname Our module's Hostname. Can be NULL for no hostname
 * @param ServerIP The IP address of the DHCP server that offered the configuration to us
 */
void _dhcp_send_request(UDPSocket Socket, uint32_t IP, const char* Hostname, uint32_t ServerIP)
{
	uint8_t* Buffer;
	size_t BufferSize;
	udp_start_packet(Socket,&Buffer,&BufferSize);

	size_t Offset = _dhcp_prepare_packet_header(Buffer,DHCP_OPCODE_REQUEST,dhcp_CurrentTransactionID,DHCP_FLAG_BROADCAST,0,0,0,0);

	// DHCP request
	SET_UINT8(Buffer,Offset,53);
	Offset += sizeof(uint8_t);

	SET_UINT8(Buffer,Offset,sizeof(uint8_t));
	Offset += sizeof(uint8_t);
	SET_UINT8(Buffer,Offset,DHCP_MESSAGE_TYPE_REQUEST);
	Offset += sizeof(uint8_t);


	// Requested IP
	SET_UINT8(Buffer,Offset,50);
	Offset += sizeof(uint8_t);
	SET_UINT8(Buffer,Offset,sizeof(uint32_t));
	Offset += sizeof(uint8_t);

	SET_UINT32(Buffer,Offset,IP);
	Offset += sizeof(uint32_t);

	// Server IP
	SET_UINT8(Buffer,Offset,54);
	Offset += sizeof(uint8_t);
	SET_UINT8(Buffer,Offset,sizeof(uint32_t));
	Offset += sizeof(uint8_t);

	SET_UINT32(Buffer,Offset,ServerIP);
	Offset += sizeof(uint32_t);

	// Hostname
	if(Hostname){
		size_t Length = strlen(Hostname);

		SET_UINT8(Buffer,Offset,12);
		Offset += sizeof(uint8_t);
		SET_UINT8(Buffer,Offset,Length);
		Offset += sizeof(uint8_t);

		for(uint8_t i = 0; i < Length; ++i)
			Buffer[Offset++] = Hostname[i];
	}


	// Parameter Request List
	SET_UINT8(Buffer,Offset,55);
	Offset += sizeof(uint8_t);
	SET_UINT8(Buffer,Offset,3);
	Offset += sizeof(uint8_t);

	/// Subnet mask
	SET_UINT8(Buffer,Offset,1);
	Offset += sizeof(uint8_t);
	/// Router IP
	SET_UINT8(Buffer,Offset,3);
	Offset += sizeof(uint8_t);
	/// DNS server
	SET_UINT8(Buffer,Offset,6);
	Offset += sizeof(uint8_t);


	// End of options
	SET_UINT8(Buffer,Offset,0xFF);
	Offset += sizeof(uint8_t);


	udp_send(Offset);
}

/**
 * Sends a request to release the current configuration to our DHCP server
 * @remark Only for internal use!
 * @param Socket The socket to send the packet to
 * @param ServerIP The IP address of the DHCP server that assigned the configuration to us
 */
void _dhcp_send_release(UDPSocket Socket, uint32_t ServerIP)
{
	// Generate a new transaction ID
	uint32_t TransactionID = _dhcp_generate_transaction_id();

	uint8_t* Buffer;
	size_t BufferSize;
	udp_start_packet(Socket,&Buffer,&BufferSize);

	size_t Offset = _dhcp_prepare_packet_header(Buffer,DHCP_OPCODE_REQUEST,TransactionID,0,dhcp_CurrentIP,0,0,0);

	// DHCP release
	SET_UINT8(Buffer,Offset,53);
	Offset += sizeof(uint8_t);

	SET_UINT8(Buffer,Offset,sizeof(uint8_t));
	Offset += sizeof(uint8_t);
	SET_UINT8(Buffer,Offset,DHCP_MESSAGE_TYPE_RELEASE);
	Offset += sizeof(uint8_t);


	// Server IP
	SET_UINT8(Buffer,Offset,54);
	Offset += sizeof(uint8_t);
	SET_UINT8(Buffer,Offset,sizeof(uint32_t));
	Offset += sizeof(uint8_t);

	SET_UINT32(Buffer,Offset,ServerIP);
	Offset += sizeof(uint32_t);


	// End of options
	SET_UINT8(Buffer,Offset,0xFF);
	Offset += sizeof(uint8_t);


	udp_send(Offset);
}

/**
 * Handles a received DHCP packet
 * @remark Only for internal use! Confer UDPCallbackHandlePacket for further information.
 */
void _dhcp_handle_packet(UDPSocket Socket, const uint8_t* Buffer, size_t Length)
{
	// If this is not the broadcast socket, immediately invalidate it (occurs when we receive a broadcasted packet from someone else)
	if(Socket != dhcp_CurrentSocket)
		udp_disconnect(Socket);

	const DHCPHeader* dhcp_hdr = (DHCPHeader*)(&Buffer[DHCP_HEADER_OFFSET]);

	// Check if the packet is meant for us
	if(dhcp_hdr->Opcode != DHCP_OPCODE_REPLY || NTOHL(dhcp_hdr->TransactionID) != dhcp_CurrentTransactionID)
		return;
	if(Length < DHCP_DATA_OFFSET + sizeof(uint32_t))
		return;
	if(GET_UINT32(Buffer,DHCP_DATA_OFFSET) != HTONL(DHCP_MAGIC_COOKIE))
		return;

	// Read all options that we're interested in
	uint8_t MessageType = 0;
	uint32_t ServerIP = 0;
	uint32_t LeaseTime = 0;
	uint32_t RenewalTime = 0;
	uint32_t RebindingTime = 0;
	uint32_t SubnetMask = 0;
	uint32_t RouterIP = 0;
	uint32_t DNSServerIP = 0;
	uint32_t NTPServerIP = 0;

	size_t Position = DHCP_OPTIONS_OFFSET;
	while(Position < Length){
		// Read the current command
		uint8_t cmd = Buffer[Position];

		// Check if it's the end options command
		if(cmd == 0xFF)
			break;

		// If not, read the command length
		if(Position + 1 >= Length)
			break;
		uint8_t len = Buffer[Position + 1];

		// Now read the command options
		if(Position + 1 + len >= Length)
			break;
		const uint8_t* options = &Buffer[Position + 2];
		Position += 2 + len;
		
		// Handle the command
		switch(cmd)
		{
			// Subnet Mask
			case 1:
				if(len == sizeof(uint32_t))
					SubnetMask = GET_UINT32(options,0);
				break;
			// Router IP
			case 3:
				if(len == sizeof(uint32_t))
					RouterIP = GET_UINT32(options,0);
				break;
			// DNS Server IP
			case 6:
				if(len == sizeof(uint32_t))
					DNSServerIP = GET_UINT32(options,0);
				break;
			// NTP Server IP
			case 42:
				if(len == sizeof(uint32_t))
					NTPServerIP = GET_UINT32(options,0);
				break;
			// Lease Time
			case 51:
				if(len == sizeof(uint32_t))
					LeaseTime = GET_UINT32(options,0);
				break;
			// Message Type
			case 53:
				if(len == sizeof(uint8_t))
					MessageType = GET_UINT8(options,0);
				break;
			// Server IP
			case 54:
				if(len == sizeof(uint32_t))
					ServerIP = GET_UINT32(options,0);
				break;
			// Renewal Time
			case 58:
				if(len == sizeof(uint32_t))
					RenewalTime = GET_UINT32(options,0);
				break;
			// Rebinding Time
			case 59:
				if(len == sizeof(uint32_t))
					RebindingTime = GET_UINT32(options,0);
				break;
		}
	}
	
	// Handle the message
	switch(MessageType)
	{
		case DHCP_MESSAGE_TYPE_OFFER:
		{
			// If it's a valid offer, request the offered IP
			if(dhcp_hdr->YourIP != 0 && ServerIP != 0)
				_dhcp_send_request(dhcp_CurrentSocket,dhcp_hdr->YourIP,dhcp_CurrentHostname,ServerIP);
			break;
		}
		case DHCP_MESSAGE_TYPE_ACK:
		{
			// We may now use the given configuration!
			dhcp_ServerIP = ServerIP;
			dhcp_CurrentIP = dhcp_hdr->YourIP;
			dhcp_LeaseTime = LeaseTime;
			dhcp_RenewalTime = (RenewalTime > 0) ? RenewalTime : (LeaseTime / 2);
			dhcp_RebindingTime = (RebindingTime > 0) ? RebindingTime : ((LeaseTime * 7) / 8);
			dhcp_SubnetMask = SubnetMask;
			dhcp_RouterIP = RouterIP;
			dhcp_DNSServerIP = DNSServerIP;
			dhcp_NTPServerIP = NTPServerIP;
			dhcp_DataValid = true;

			udp_disconnect(dhcp_CurrentSocket);
			dhcp_CurrentSocket = INVALID_UDP_SOCKET;
			break;
		}
		case DHCP_MESSAGE_TYPE_NAK:
		{
			// We must no longer use the given configuration, so we need to invalidate everything we've stored...
			_dhcp_invalidate();

			// ... and start all over again!
			_dhcp_send_discover(dhcp_CurrentSocket,dhcp_CurrentIP,dhcp_CurrentHostname);
			break;
		}
	}
}

/**
 * Sends a request to renew the current configuration to our DHCP server
 * @remark Only for internal use!
 */
void _dhcp_renew(void)
{
	dhcp_CurrentTransactionID = _dhcp_generate_transaction_id();
	dhcp_CurrentSocket = udp_connect_ex(MAKE_IP(255,255,255,255),DHCP_REMOTE_PORT,3000,&_dhcp_handle_packet,DHCP_LOCAL_PORT);
	_dhcp_send_request(dhcp_CurrentSocket,dhcp_CurrentIP,dhcp_CurrentHostname,dhcp_ServerIP);
}

// -----------------------------------------------------------------------------------------------
// ----------------------------- External Function Implementations -------------------------------
// -----------------------------------------------------------------------------------------------
bool dhcp_request(const char* Hostname, uint16_t Timeout, uint32_t* IP, uint32_t* NetMask, uint32_t* RouterIP, uint32_t* DNSServerIP, uint32_t* NTPServerIP)
{
	if(!dhcp_DataValid){
		dhcp_CurrentHostname = Hostname;
		_dhcp_invalidate();
	}

	// Start requesting
	uint16_t Timer = 0;
	dhcp_CurrentSocket = udp_connect_ex(MAKE_IP(255,255,255,255),DHCP_REMOTE_PORT,Timeout,&_dhcp_handle_packet,DHCP_LOCAL_PORT);
	_dhcp_send_discover(dhcp_CurrentSocket,dhcp_CurrentIP,dhcp_CurrentHostname);
	while(dhcp_is_requesting() && (Timeout == 0 || (Timer < Timeout))){
		ethernet_update();
		++Timer;
		_delay_ms(1);
	}

	if(Timeout == 0 || (Timer < Timeout)){
		// Return the requested data
		if(IP)
			*IP = dhcp_CurrentIP;
		if(NetMask)
			*NetMask = dhcp_SubnetMask;
		if(RouterIP)
			*RouterIP = dhcp_RouterIP;
		if(DNSServerIP)
			*DNSServerIP = dhcp_DNSServerIP;
		if(NTPServerIP)
			*NTPServerIP = dhcp_NTPServerIP;
	}else{
		dhcp_DataValid = false;
	}
	return dhcp_DataValid;
}

bool dhcp_is_requesting(void)
{
	return udp_table_is_valid_socket(dhcp_CurrentSocket);
}

void dhcp_release(void)
{
	if(dhcp_DataValid){
		// Tell the DHCP server that we no longer want our configuration
		UDPSocket sock = udp_connect_ex(dhcp_ServerIP,DHCP_REMOTE_PORT,3000,&_dhcp_handle_packet,DHCP_LOCAL_PORT);
		_dhcp_send_release(sock,dhcp_ServerIP);
		udp_disconnect(sock);

		// Invalidate it
		_dhcp_invalidate();
	}
}

void dhcp_second_tick(void)
{
	if(dhcp_RenewalTime != DHCP_INVALID_TIMER){
		if(dhcp_RenewalTime == 0){
			dhcp_RenewalTime = DHCP_INVALID_TIMER;
			_dhcp_renew();
		}else{
			--dhcp_RenewalTime;
		}
	}

	if(dhcp_RebindingTime != DHCP_INVALID_TIMER){
		if(dhcp_RebindingTime == 0){
			dhcp_RebindingTime = DHCP_INVALID_TIMER;
			_dhcp_renew();
		}else{
			--dhcp_RebindingTime;
		}
	}

	if(dhcp_LeaseTime != DHCP_INVALID_TIMER){
		if(dhcp_LeaseTime == 0){
			// Lease time has run out. This should NEVER happen in a properly configured network!
			_dhcp_invalidate();
			dhcp_LeaseTime = DHCP_INVALID_TIMER;
		}else{
			--dhcp_LeaseTime;
		}
	}
}

bool dhcp_has_valid_configuration(void)
{
	return dhcp_DataValid;
}

const char* dhcp_get_hostname(void)
{
	return dhcp_CurrentHostname;
}

uint32_t dhcp_get_ip(void)
{
	return dhcp_CurrentIP;
}

uint32_t dhcp_get_netmask(void)
{
	return dhcp_SubnetMask;
}

uint32_t dhcp_get_router_ip(void)
{
	return dhcp_RouterIP;
}

uint32_t dhcp_get_dns_server_ip(void)
{
	return dhcp_DNSServerIP;
}

uint32_t dhcp_get_ntp_server_ip(void)
{
	return dhcp_NTPServerIP;
}

#endif //IMPLEMENT_DHCP