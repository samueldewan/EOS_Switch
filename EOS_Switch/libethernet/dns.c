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
#include <string.h>

#include "global.h"
#include "ethernet.h"
#include "dns.h"

#ifdef IMPLEMENT_DNS

// -----------------------------------------------------------------------------------------------
// ------------------------------------- Constants / Structs -------------------------------------
// -----------------------------------------------------------------------------------------------
#define DNS_QUESTION_TYPE_HOST 0x01 // A
#define DNS_QUESTION_TYPE_NAMESERVER 0x02 // NS
#define DNS_QUESTION_TYPE_ALIAS 0x05 // CNAME
#define DNS_QUESTION_TYPE_REVERSE_LOOKUP // PTR
#define DNS_QUESTION_TYPE_MAIL_EXCHANGE 0x0F // MX
#define DNS_QUESTION_TYPE_SERVICE 0x21 // SRV
#define DNS_QUESTION_TYPE_INCREMENTAL_ZONE_TRANSFER 0xFB // IXFR
#define DNS_QUESTION_TYPE_STANDARD_ZONE_TRANSFER 0xFC // AXFR
#define DNS_QUESTION_TYPE_ALL 0xFF

#define DNS_HEADER_OFFSET 0
#define DNS_HEADER_LENGTH 12

#define DNS_PORT 53

#define DNS_HEADER_RETURN_CODE_MASK (1 << 0 | 1 << 1 | 1 << 2 | 1 << 3)
#define DNS_HEADER_FLAG_RECURSION_AVAILABLE (1 << 7)
#define DNS_HEADER_FLAG_RECURSION_DESIRED (1 << 8)
#define DNS_HEADER_FLAG_IS_TRUNCATED (1 << 9)
#define DNS_HEADER_FLAG_IS_AUTHORITATIVE_ANSWER (1 << 10)
#define DNS_HEADER_OPCODE_MASK (1 << 11 | 1 << 12 | 1 << 13 | 1 << 14)
#define DNS_HEADER_FLAG_IS_RESPONSE (1 << 15)

typedef struct _DNSHeader
{
	uint16_t TransactionID;
	uint16_t Flags;
	uint16_t QuestionCount;
	uint16_t AnswerCount;
	uint16_t AuthorityCount;
	uint16_t AdditionalCount;
} DNSHeader;

typedef struct _DNSQuestionField
{
	const char* Name;
	uint16_t Type;
	uint16_t Class;
} DNSQuestionField;

typedef struct _DNSResourceField
{
	const char* Name;
	uint16_t Type;
	uint16_t Class;
	uint32_t TimeToLive;
	uint16_t DataLength;
	const uint8_t* Data;
} DNSResourceField;


// -----------------------------------------------------------------------------------------------
// -------------------------------------- Global Variables ---------------------------------------
// -----------------------------------------------------------------------------------------------
static uint32_t dns_NameserverIP = 0;
static const char* dns_CurrentHostname = NULL;
static uint32_t dns_CurrentIP = 0;

// -----------------------------------------------------------------------------------------------
// ----------------------------- Internal Function Implementations -------------------------------
// -----------------------------------------------------------------------------------------------
/**
 * Converts a null-terminated hostname string to the format used in the DNS protocol
 * @remark Only for internal use!
 * @param Hostname Pointer to the first byte of the null-terminated hostname string to be converted. There must be at least strlen(Hostname)+2 bytes reserved.
 * @return The size of the converted string in bytes (excluding the null-byte)
 */
size_t _dns_hostname_to_protocol(char* Hostname)
{
	// Move the entire string one byte forth
	size_t Length = strlen(Hostname);
	for(size_t i = Length + 1; i > 0; --i)
		Hostname[i] = Hostname[i-1];

	// Replace all periods by the length of its following subdomain
	uint8_t Count = 0;
	for(size_t i = Length; i > 0; --i){
		if(Hostname[i] == '.'){
			Hostname[i] = Count;
			Count = 0;
		}else{
			++Count;
		}
	}
	Hostname[0] = Count;

	return Length + 1;
}

/**
 * Converts a hostname string in the format used in the DNS protocol to a null-terminated format
 * @remark Only for internal use!
 * @param Hostname Pointer to the first byte of the string to be converted.
 * @return The size of the converted string in bytes (excluding the null-byte)
 */
size_t _dns_hostname_from_protocol(char* Hostname)
{
	size_t Length = 0;
	while(*Hostname){
		// Read all subdomain strings and replace the sizes by periods
		uint8_t len = *Hostname;
		for(uint8_t i = 0; i < len; ++i)
			Hostname[i] = Hostname[i+1];
		Hostname[len] = '.';
		Hostname += len + 1;
		Length += len + 1;
	}
	*(Hostname-1) = 0x00;
	return Length - 1;
}

/**
 * Reads a DNS question field into a DNSQuestionField struct
 * @remark Only for internal use!
 * @param Buffer Pointer to the first byte of the question field
 * @param Field The data struct where the information will be stored
 * @return The size of the field in the buffer (in bytes)
 */
size_t _dns_read_question_field(uint8_t* Buffer, DNSQuestionField* Field)
{
	size_t len = _dns_hostname_from_protocol((char*)Buffer);
	Field->Name = (char*)Buffer;
	Field->Type = *((uint16_t*)&Buffer[len+2]);
	Field->Class = *((uint16_t*)&Buffer[len+4]);
	return len + 6;
}

/**
 * Writes a DNSQuestionField struct into a buffer
 * @remark Only for internal use!
 * @param Buffer Pointer to the first byte of the question field
 * @param Field The data struct containing the information
 * @return The size of the field in the buffer (in bytes)
 */
size_t _dns_write_question_field(uint8_t* Buffer, const DNSQuestionField* Field)
{
	uint8_t Length = strlen(Field->Name);
	for(uint8_t i = 0; i <= Length; ++i)
		Buffer[i] = Field->Name[i];
	size_t len = _dns_hostname_to_protocol((char*)Buffer);
	*((uint16_t*)&Buffer[len + 1]) = Field->Type;
	*((uint16_t*)&Buffer[len + 3]) = Field->Class;
	return len + 5;
}

/**
 * Reads a DNS resource field into a DNSResourceField struct
 * @remark Only for internal use!
 * @param Buffer Pointer to the start of the entire DNS packet
 * @param Position The position where the resource field starts. After the function call, it will indicate the position of the first byte after the resource field.
 * @return The DNSResourceField that was read
 */
DNSResourceField _dns_read_resource_field(uint8_t* Buffer, size_t* Position)
{
	DNSResourceField result;

	if(Buffer[*Position] & 0xC0){
		// Pointer to another string in the packet
		result.Name = (const char*)&Buffer[ Buffer[*Position+1] & ~0xC0 ];
		*Position += 2;
	}else{
		// String stored here
		result.Name = (const char*)&Buffer[*Position];
		*Position += _dns_hostname_from_protocol((char*)result.Name);
		*Position += 2;
	}

	result.Type = HTONS(*((uint16_t*)&Buffer[*Position]));
	*Position += sizeof(uint16_t);
	result.Class = HTONS(*((uint16_t*)&Buffer[*Position]));
	*Position += sizeof(uint16_t);
	result.TimeToLive = HTONS(*((uint32_t*)&Buffer[*Position]));
	*Position += sizeof(uint32_t);
	result.DataLength = HTONS(*((uint16_t*)&Buffer[*Position]));
	*Position += sizeof(uint16_t);
	result.Data = &Buffer[*Position];
	*Position += result.DataLength;

	return result;
}

/**
 * Handles a received DNS packet
 * @remark only for internal use!
 * @param Buffer The packet buffer
 * @param Length The packet buffer's length in bytes
 */
void _dns_handle_packet_impl(const uint8_t* Buffer,size_t Length)
{
	// Read the header
	DNSHeader* dns_hdr = (DNSHeader*)(&Buffer[DNS_HEADER_OFFSET]);
	if(!(dns_hdr->Flags & DNS_HEADER_FLAG_IS_RESPONSE))
		return;

	// Skip all question fields
	size_t pos = DNS_HEADER_OFFSET + DNS_HEADER_LENGTH;
	for(uint16_t i = 0; i < HTONS(dns_hdr->QuestionCount); ++i){
		DNSQuestionField field;
		pos += _dns_read_question_field((uint8_t*)&Buffer[pos],&field);
	}

	// Read the answers and check if they're for the correct host (TODO: is there a better way of checking this?)
	bool IsCorrectHostname = false;
	size_t pos_backup = pos;
	for(uint16_t i = 0; i < HTONS(dns_hdr->AnswerCount); ++i){
		// Read the record
		DNSResourceField field = _dns_read_resource_field((uint8_t*)Buffer,&pos);

		// TODO: implement aliases and so on
		if(strcmp(field.Name,dns_CurrentHostname) == 0){
			IsCorrectHostname = true;
			break;
		}
	}

	if(!IsCorrectHostname)
		return;

	// Now read the first IP we can find
	pos = pos_backup;
	for(uint16_t i = 0; i < HTONS(dns_hdr->AnswerCount); ++i){
		// Read the record
		DNSResourceField field = _dns_read_resource_field((uint8_t*)Buffer,&pos);

		if(field.Type == DNS_QUESTION_TYPE_HOST && field.DataLength == sizeof(uint32_t)){
			dns_CurrentIP = *((uint32_t*)field.Data);
			break;
		}
	}
}

/**
 * Writes a DNS query packet for a given Hostname to a given Buffer
 * @remark Only for internal use!
 * @param Hostname The hostname to query
 * @param Buffer Pointer to the packet buffer
 * @param BufferSize Size of the packet buffer in bytes
 * @return The number of bytes written to the buffer
 */
size_t _dns_prepare_query_packet(const char* Hostname,uint8_t* Buffer,size_t BufferSize)
{
	DNSHeader* dns_hdr = (DNSHeader*)(&Buffer[DNS_HEADER_OFFSET]);
	dns_hdr->TransactionID = HTONS(1);
	dns_hdr->Flags = 0x0000;
	dns_hdr->QuestionCount = HTONS(1);
	dns_hdr->AnswerCount = HTONS(0);
	dns_hdr->AuthorityCount = HTONS(0);
	dns_hdr->AdditionalCount = HTONS(0);

	DNSQuestionField field;
	field.Name = Hostname;
	field.Type = HTONS(DNS_QUESTION_TYPE_HOST);
	field.Class = HTONS(0x0001);
	
	size_t len = DNS_HEADER_OFFSET + DNS_HEADER_LENGTH;
	len += _dns_write_question_field(&Buffer[len],&field);

	return len;
}

#ifdef IMPLEMENT_UDP
void _dns_udp_handle_packet(UDPSocket Socket,const uint8_t* Buffer,size_t Length)
{
	_dns_handle_packet_impl(Buffer,Length);
}

void _dns_udp_send_query_packet(UDPSocket Socket,const char* Hostname)
{
	uint8_t* Buffer;
	size_t BufferSize;
	udp_start_packet(Socket,&Buffer,&BufferSize);
	size_t Length = _dns_prepare_query_packet(Hostname,Buffer,BufferSize);
	udp_send(Length);
}
#endif //IMPLEMENT_UDP

// -----------------------------------------------------------------------------------------------
// ----------------------------- External Function Implementations -------------------------------
// -----------------------------------------------------------------------------------------------
void dns_initialise(uint32_t NameserverIP)
{
	dns_NameserverIP = NameserverIP;
}

uint32_t dns_query_hostname(const char* Hostname, uint16_t Timeout)
{
	UDPSocket sck = udp_connect(dns_NameserverIP,DNS_PORT,Timeout,&_dns_udp_handle_packet);

	uint16_t Timer = 0;
	dns_CurrentIP = 0;
	dns_CurrentHostname = Hostname;
	while(Timer <= Timeout){
		// Send a new request every 500ms (in case the previous one got lost)
		if((Timer % 500) == 0)
			_dns_udp_send_query_packet(sck,Hostname);

		// Update
		ethernet_update();

		// Check if we have received an answer
		if(dns_CurrentIP != 0)
			break;

		// Wait 1ms and continue
		_delay_ms(1);
		++Timer;
	}
	
	udp_disconnect(sck);
	return dns_CurrentIP;
}

#endif //IMPLEMENT_DNS