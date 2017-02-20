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


// MARK: Variables

// MARK: Functions

int init_network (void)
{
    //uint8_t mac[6];
    //eeprom_read_block(SETTING_MAC_ADDR, mac, 6);
    
    //char hostname[32];
    //eeprom_read_block(hostname, SETTING_HOSTNAME, 32);
    
    //ethernet_initialise_dhcp(hostname, 1000);
    //ethernet_initialise(eeprom_read_dword(SETTING_IP_ADDR), eeprom_read_dword(SETTING_NETMASK), eeprom_read_dword(SETTING_ROUTER_ADDR));

    
    // Timer 1 (network clock)
    TCCR1B |= (1<<WGM11);                           // Set the Timer Mode to CTC
    TIMSK1 |= (1<<OCIE1A);                          // Set the ISR COMPA vector (enables COMP interupt)
    OCR1A = 31250;                                  // 1 Hz
    TCCR1B |= (1<<CS12);                            // set prescaler to 256 and start timer 1
    
    //eos_connection = udp_connect(eeprom_read_dword(SETTING_TARGET_IP), eeprom_read_dword(SETTING_TARGET_PORT), 5, NULL);
    
    return 0;
}

int network_send_from_eeprom (uint16_t address, int length)
{
    return 0;
}

int network_send_string_P (const char *str)
{
    return 0;
}

int network_send_string (const char *str)
{
    return 0;
}

uint32_t network_get_ip_addr()
{
    return 0;
}

uint32_t network_get_router_addr()
{
    return 0;
}

uint32_t network_get_netmask()
{
    return 0;
}

void network_service (void)
{
    
}

// MARK: Interupt Service Routines
ISR(TIMER1_COMPA_vect)
{
    
}
