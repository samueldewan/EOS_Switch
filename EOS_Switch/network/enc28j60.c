/*
 * enc28j60.c
 * EOS_Switch
 *
 * Created by Ian Dewan on 2017-02-19.
 * Copyright © 2017 Ian Dewan. All rights reserved.
 */

#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "enc28j60.h"
#include "spi.h"
#include "global.h"
#include "pindefinitions.h"
#include <avr/io.h>

uint32_t debug_num;

static const uint8_t read_ESTAT[1] PROGMEM = {SPI_RCR(0x1D)}; // read ESTAT
static const uint8_t bank_2[2] PROGMEM = {SPI_BFS(0x1F), 0x02}; // switch to control register bank 2
static const uint8_t set_MACON3[2] PROGMEM = {SPI_WCR(0x02), 0xF2}; // set MACON3
static const uint8_t set_MACON4[2] PROGMEM = {SPI_WCR(0x03), 0x40}; // set MACON4
static const uint8_t set_MABBIPG[2] PROGMEM = {SPI_WCR(0x04), 0x12}; // set MABBIPG
static const uint8_t set_MAIPGL[2] PROGMEM = {SPI_WCR(0x06), 0x12}; // set MAIPGL
static const uint8_t set_MAIPGH[2] PROGMEM = {SPI_WCR(0x07), 0x0C}; // set MAIPGH
static const uint8_t bank_3[2] PROGMEM = {SPI_BFS(0x1F), 0x03}; // switch to control register bank 3
static const uint8_t bank_0[2] PROGMEM = {SPI_BFS(0x1F), 0x00}; // switch to control register bank 0
static const uint8_t set_EDMASTL[2] PROGMEM = {SPI_WCR(0x14), 0x0F}; // set checksum start address
static const uint8_t set_EDMANDL[2] PROGMEM = {SPI_WCR(0x14), 0x22}; // set checksum end address

static const uint8_t packet_1[8] PROGMEM = {SPI_WBM, // Chunk 1 of the packet
    0x00, // ENC28J60 control byte
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF // Destination MAC Address
};
static const uint8_t packet_2[19] PROGMEM = {SPI_WBM, // Chunk 2 of the packet
    0x08, 0x00, // Ethernet packet type
    0x45, // IP Version && IHL
    0x00, // Type of Service
    0x00, 0x00, // Placeholder for total length
    0x00, 0x00, // Placeholder for packet id
    0x00, 0x00, // Flags and fragment offset
    0x0F, // TTL
    0x11, // Protocol
    0x00, 0x00, // Placeholder for header checksum
    0x00, 0x00, 0x00, 0x00 // Source address
};
static uint8_t addresses[7] = {SPI_WBM, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // buffer to write addresses from

enum tmp_enum enc28j60_state = STATE_UNREADY;
char debug = ' ';

void init_enc28j60(void)
{
    if (enc28j60_state != STATE_UNREADY) {
        return;
    }
    
    eeprom_read_block(addresses + 1, SETTING_MAC_ADDR, 6);
    spi_start_transfer_P(1, read_ESTAT, 1);
}

void enc28j60_service(void)
{
    uint16_t port;
    switch (enc28j60_state) {
        case STATE_UNREADY:
            if (spi_in_bytes_available() > 0) {
                debug = 'b';
                //debug_num = spi_read_byte();
                if (spi_read_byte() & 1) {
                    enc28j60_state = STATE_CLOCK_STABILIZED;
                } else {
                    debug_num++;
                    spi_start_transfer_P(1, read_ESTAT, 1);
                }
            }
            break;
        case STATE_CLOCK_STABILIZED:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, bank_2, 0);
                enc28j60_state = STATE_BANK_2;
            }
            break;
        case STATE_BANK_2:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, set_MACON3, 0);
                enc28j60_state = STATE_MACON3_SET;
            }
            break;
        case STATE_MACON3_SET:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, set_MACON4, 0);
                enc28j60_state = STATE_MACON4_SET;
            }
            break;
        case STATE_MACON4_SET:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, set_MABBIPG, 0);
                enc28j60_state = STATE_MABBIPG_SET;
            }
            break;
        case STATE_MABBIPG_SET:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, set_MAIPGL, 0);
                enc28j60_state = STATE_MAIPGL_SET;
            }
            break;
        case STATE_MAIPGL_SET:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, set_MAIPGH, 0);
                enc28j60_state = STATE_MAIPGH_SET;
            }
            break;
        case STATE_MAIPGH_SET:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, bank_3, 0);
                enc28j60_state = STATE_BANK_3;
            }
            break;
        case STATE_BANK_3:
            if (spi_transfer_available()) {
                spi_start_transfer_with_cmd(SPI_WCR(0x04), 1, &addresses[1], 0);
                enc28j60_state = STATE_MADDR_1_SET;
            }
            break;
        case STATE_MADDR_1_SET:
            if (spi_transfer_available()) {
                spi_start_transfer_with_cmd(SPI_WCR(0x05), 1, &addresses[2], 0);
                enc28j60_state = STATE_MADDR_2_SET;
            }
            break;
        case STATE_MADDR_2_SET:
            if (spi_transfer_available()) {
                spi_start_transfer_with_cmd(SPI_WCR(0x02), 1, &addresses[3], 0);
                enc28j60_state = STATE_MADDR_3_SET;
            }
            break;
        case STATE_MADDR_3_SET:
            if (spi_transfer_available()) {
                spi_start_transfer_with_cmd(SPI_WCR(0x03), 1, &addresses[4], 0);
                enc28j60_state = STATE_MADDR_4_SET;
            }
            break;
        case STATE_MADDR_4_SET:
            if (spi_transfer_available()) {
                spi_start_transfer_with_cmd(SPI_WCR(0x00), 1, &addresses[5], 0);
                enc28j60_state = STATE_MADDR_5_SET;
            }
            break;
        case STATE_MADDR_5_SET:
            if (spi_transfer_available()) {
                spi_start_transfer_with_cmd(SPI_WCR(0x01), 1, &addresses[6], 0);
                enc28j60_state = STATE_MADDR_6_SET;
            }
            break;
        case STATE_MADDR_6_SET:
            if (spi_transfer_available()) {
                spi_start_transfer_P(8, packet_1, 0);
                enc28j60_state = STATE_PACKET_1_WRITTEN;
            }
            break;
        case STATE_PACKET_1_WRITTEN:
            if (spi_transfer_available()) {
                spi_start_transfer(7, addresses, 0);
                enc28j60_state = STATE_SRC_MAC_WRITTEN;
            }
            break;
        case STATE_SRC_MAC_WRITTEN:
            if (spi_transfer_available()) {
                spi_start_transfer_P(19, packet_2, 0);
                enc28j60_state = STATE_PACKET_2_WRITTEN;
            }
            break;
        case STATE_PACKET_2_WRITTEN:
            if (spi_transfer_available()) {
                eeprom_read_block(addresses + 1, SETTING_TARGET_IP, 4);
                spi_start_transfer(5, addresses, 0);
                enc28j60_state = STATE_DEST_IP_WRITTEN;
            }
            break;
        case STATE_DEST_IP_WRITTEN:
            if (spi_transfer_available()) {
                addresses[1] = 0x00;
                addresses[2] = 0x00;
                spi_start_transfer(3, addresses, 0);
                enc28j60_state = STATE_SRC_PORT_WRITTEN;
            }
            break;
        case STATE_SRC_PORT_WRITTEN:
            if (spi_transfer_available()) {
                eeprom_read_block(&port, SETTING_TARGET_PORT, 2);
                addresses[1] = (uint8_t) port;
                addresses[2] = (uint8_t) (port >> 8);
                spi_start_transfer(3, addresses, 0);
                enc28j60_state = STATE_DEST_PORT_WRITTEN;
            }
            break;
        case STATE_DEST_PORT_WRITTEN:
            if (spi_transfer_available()) {
                addresses[1] = 0x00;
                addresses[2] = 0x00;
                addresses[3] = 0x00;
                addresses[4] = 0x00;
                spi_start_transfer(5, addresses, 0);
                enc28j60_state = STATE_HEADERS_WRITTEN;
            }
            break;
        case STATE_HEADERS_WRITTEN:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, bank_0, 0);
                enc28j60_state = STATE_BANK_0;
            }
            break;
        case STATE_BANK_0:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, set_EDMASTL, 0);
                enc28j60_state = STATE_EDMASTL_SET;
            }
            break;
        case STATE_EDMASTL_SET:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, set_EDMANDL, 0);
                enc28j60_state = STATE_DONE;
            }
            break;
        default: return;
    }
}

int enc28j60_ready(void)
{
    return (enc28j60_state == STATE_DONE) && spi_transfer_available();
}
