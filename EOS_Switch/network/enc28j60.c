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
#include "../global.h"

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

void init_enc28j60(void)
{
    uint8_t clock_stabilized, i;

    do {
        /* Read ESTAT. */
        spi_start_command();
        spi_transfer(SPI_RCR(0x1D));
        clock_stabilized = spi_transfer(SPI_NOP) & 1;
        spi_end_command();
    } while (!clock_stabilized);
    
    /* Switch to control register bank 2. */
    spi_start_command();
    spi_transfer(SPI_BFS(0x1F));
    spi_transfer(0x02);
    spi_end_command();
    
    /* Set MACON3. */
    spi_start_command();
    spi_transfer(SPI_WCR(0x02));
    spi_transfer(0xF2);
    spi_end_command();
    
    /* Set MACON4. */
    spi_start_command();
    spi_transfer(SPI_WCR(0x03));
    spi_transfer(0x40);
    spi_end_command();
    
    /* Set MABBIPG. */
    spi_start_command();
    spi_transfer(SPI_WCR(0x04));
    spi_transfer(0x12);
    spi_end_command();
    
    /* Set MAIPGL. */
    spi_start_command();
    spi_transfer(SPI_WCR(0x06));
    spi_transfer(0x12);
    spi_end_command();
    
    /* Set MAIPGH. */
    spi_start_command();
    spi_transfer(SPI_WCR(0x07));
    spi_transfer(0x0C);
    spi_end_command();
    
    /* Switch to control register bank 3. */
    spi_start_command();
    spi_transfer(SPI_BFS(0x1F));
    spi_transfer(0x03);
    spi_end_command();
    
    /* Copy MAC address. */
    /* Byte 1. */
    spi_start_command();
    spi_transfer(SPI_WCR(0x04));
    spi_transfer_from_eeprom(SETTING_MAC_ADDR);
    spi_end_command();
    /* Byte 2. */
    spi_start_command();
    spi_transfer(SPI_WCR(0x05));
    spi_transfer_from_eeprom(SETTING_MAC_ADDR + 1);
    spi_end_command();
    /* Byte 3. */
    spi_start_command();
    spi_transfer(SPI_WCR(0x02));
    spi_transfer_from_eeprom(SETTING_MAC_ADDR + 2);
    spi_end_command();
    /* Byte 4. */
    spi_start_command();
    spi_transfer(SPI_WCR(0x03));
    spi_transfer_from_eeprom(SETTING_MAC_ADDR + 3);
    spi_end_command();
    /* Byte 5. */
    spi_start_command();
    spi_transfer(SPI_WCR(0x00));
    spi_transfer_from_eeprom(SETTING_MAC_ADDR + 4);
    spi_end_command();
    /* Byte 6. */
    spi_start_command();
    spi_transfer(SPI_WCR(0x00))1
    spi_transfer_from_eeprom(SETTING_MAC_ADDR + 5);
    spi_end_command();

    /* Write chunk 1 of the packet. */
    spi_start_command();
    for (i = 0; i < 8; i++) {
        spi_transfer_P(packet_1 + i);
    }
    spi_end_command();

    /* Write the source MAC address. */
    spi_start_command();
    spi_transfer(SPI_WBM);
    for (i = 0; i < 6; i++) {
        spi_transfer_from_eeprom(SETTING_MAC_ADDR + i);
    }
    spi_end_command();

    /* Write chunk 2 of the packet. */
    spi_start_command();
    for (i = 0; i < 19; i++) {
        spi_transfer_P(packet_2 + i);
    }
    spi_end_command();

    /* Write the destination IP address. */
    spi_start_command();
    spi_transfer(SPI_WBM);
    for (i = 0; i < 4; i++) {
        spi_transfer_from_eeprom(SETTING_TARGET_IP + i);
    }
    spi_end_command();

    /* Write the source port. */
    spi_start_command();
    spi_transfer(SPI_WBM);
    spi_transfer(0x00);
    spi_transfer(0x00);
    spi_end_command();

    /* Write the destimation port. */
    spi_start_command();
    spi_transfer(SPI_WBM);
    spi_transfer_from_eeprom(SETTING_TARGET_PORT + 1);
    spi_transfer_from_eeprom(SETTING_TARGET_PORT);
    spi_end_command();

    /* Write placeholders for the UDP length and checksum. */
    spi_start_command();
    spi_transfer(SPI_WBM);
    spi_transfer(0x00);
    spi_transfer(0x00);
    spi_transfer(0x00);
    spi_transfer(0x00);
    spi_end_command();

    /* Switch to control register bank 0. */
    spi_start_command();
    spi_transfer(SPI_BFC(0x1F));
    spi_transfer(0x03);
    spi_end_command();
    
    /* Set the checksum start address (EDMASTL). */
    spi_start_command();
    spi_transfer(SPI_WCR(0x10));
    spi_transfer(0x0F);
    spi_end_command();
    
    /* Set the checksum end address (EDMANDL). */
    spi_start_command();
    spi_transfer(SPI_WCR(0x12));
    spi_transfer(0x22);
    spi_end_command();
}
