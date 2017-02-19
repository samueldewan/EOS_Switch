//
//  network.c
//  EOS_Switch
//
//  Created by Samuel Dewan on 2017-02-02.
//  Copyright © 2017 Samuel Dewan. All rights reserved.
//

#include "network.h"

#include "pindefinitions.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include <avr/pgmspace.h>
#include <string.h>

#include "serial.h"

#include "./libethernet/libethernet.h"

// MARK: Variables
static UDPSocket eos_connection;

// MARK: Functions

int init_network (void)
{
    spi_initialise(&PORTB, &DDRB, PB5, PB4, PB3, PB2);
    uint8_t mac[6];
    eeprom_read_block(SETTING_MAC_ADDR, mac, 6);
    enc28j60_initialise(mac, true);
    
    char hostname[32];
    eeprom_read_block(hostname, SETTING_HOSTNAME, 32);
    
    // Initialise all enabled modules of the ethernet stack
#ifdef IMPLEMENT_DHCP
    if (eeprom_read_byte(SETTING_DCHP)) {
        ethernet_initialise_dhcp(hostname, 1000);
        //if (ethernet_get_router_ip() == 0) {
        //    return -1;
        //}
    } else {
        ethernet_initialise(eeprom_read_dword(SETTING_IP_ADDR), eeprom_read_dword(SETTING_NETMASK), eeprom_read_dword(SETTING_ROUTER_ADDR));
        ethernet_wait_for_link_status(0);
    }
#else
    ethernet_initialise(eeprom_read_dword(SETTING_IP_ADDR), eeprom_read_dword(SETTING_NETMASK), eeprom_read_dword(SETTING_ROUTER_ADDR));
    ethernet_wait_for_link_status(0);
#endif // IMPLEMENT_DHCP

    
    // Timer 1 (network clock)
    TCCR1B |= (1<<WGM11);                           // Set the Timer Mode to CTC
    TIMSK1 |= (1<<OCIE1A);                          // Set the ISR COMPA vector (enables COMP interupt)
    OCR1A = 31250;                                  // 1 Hz
    TCCR1B |= (1<<CS12);                            // set prescaler to 256 and start timer 1
    
    
    // Initialise the DNS resolver module
#ifdef IMPLEMENT_DNS
#	ifdef IMPLEMENT_DHCP
    dns_initialise(dhcp_get_dns_server_ip());
#	else
    dns_initialise(eeprom_read_dword(SETTING_DNS_ADDR));
#	endif //IMPLEMENT_DHCP
#endif //IMPLEMENT_DNS
    
    // Initialise the NTP client module
//#ifdef IMPLEMENT_NTP
//#	ifdef IMPLEMENT_DHCP
//    if (eeprom_read_byte(SETTING_DCHP)) {
//        ntp_initialise(dhcp_get_ntp_server_ip(), eeprom_read_byte(SETTING_GMT_OFFSET));
//    } else {
//        ntp_initialise(eeprom_read_dword(SETTING_NTP_ADDR), eeprom_read_byte(SETTING_GMT_OFFSET));
//    }
//#	else
//    ntp_initialise(eeprom_read_dword(SETTING_NTP_ADDR), eeprom_read_byte(SETTING_GMT_OFFSET));
//#	endif //IMPLEMENT_DHCP
//#endif //IMPLEMENT_DNS
    
    eos_connection = udp_connect(eeprom_read_dword(SETTING_TARGET_IP), eeprom_read_dword(SETTING_TARGET_PORT), 5, NULL);
    
    return 0;
}

int network_send_from_eeprom (uint16_t address, int length)
{
    uint8_t* buffer;
    size_t buffer_size;
    udp_start_packet(eos_connection, &buffer, &buffer_size);
    length = (length < buffer_size) ? length : buffer_size;
    
    eeprom_read_block(address, buffer, length);
    
    udp_send(length);
    
    return length;
}

int network_send_string_P (const char *str)
{
    uint8_t* buffer;
    size_t buffer_size;
    udp_start_packet(eos_connection, &buffer, &buffer_size);
    
    int length = (strlen_P(str) < buffer_size) ? strlen_P(str) : buffer_size;
    
    memcpy_P(buffer, str, length);
    
    udp_send(length);
    
    return length;
}

int network_send_string (const char *str)
{
    uint8_t* buffer;
    size_t buffer_size;
    udp_start_packet(eos_connection, &buffer, &buffer_size);
    
    int length = (strlen_P(str) < buffer_size) ? strlen_P(str) : buffer_size;
    
    char temp[10];
    serial_put_string("Length: ");
    ltoa(length, temp, 10);
    serial_put_string(temp);
    serial_put_string(" strlen: ");
    ltoa(strlen_P(str), temp, 10);
    serial_put_string(temp);
    serial_put_string(" buffer size: ");
    ltoa(buffer_size, temp, 10);
    serial_put_string(temp);
    serial_put_byte('\n');
    
    memcpy(buffer, str, length);
    
    udp_send(length);
    
    return length;
}

uint32_t network_get_ip_addr()
{
    return ethernet_get_ip();
}

uint32_t network_get_router_addr()
{
    return ethernet_get_router_ip();
}

uint32_t network_get_netmask()
{
    return ethernet_get_netmask();
}

void network_service (void)
{
    ethernet_update();
}

// MARK: Interupt Service Routines
ISR(TIMER1_COMPA_vect)
{
    ethernet_second_tick();
}
