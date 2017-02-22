/*
 * udp.c
 * EOS_Switch
 *
 * Created by Ian Dewan on 2017-02-21.
 * Copyright © 2017 Ian Dewan. All rights reserved.
 */

#include <avr/pgmspace.h>

#include "udp.h"
#include "spi.h"

#include "pindefinitions.h"

static uint16_t identification;

static inline void udp_send_internal(uint8_t length);

static enum {
    STATE_READY,
    STATE_CALCULATING,
    STATE_TRANSMITTING
} udp_state = STATE_READY;

int udp_buffer_send(const uint8_t *buffer, uint8_t length)
{
    uint8_t i;

    if (udp_state != STATE_READY) {
        return 1;
    }
    
    spi_start_cmd();
    for (i = 0; i < length; i++) {
        spi_transfer(buffer[i]);
    }
    spi_end_cmd();

    udp_send_internal(length);
    
    return 0;
}

int udp_eeprom_send(uint16_t buffer, uint8_t length)
{
    uint8_t i;

    if (udp_state != STATE_READY) {
        return 1;
    }
    
    spi_start_cmd();
    for (i = 0; i < length; i++) {
        spi_transfer_from_eeprom(buffer + i);
    }
    spi_end_cmd();

    udp_send_internal(length);
    
    return 0;
}

static inline void udp_send_internal(uint8_t length)
{
    uint16_t tmp_length;

    /* Move to the IP header Length field. */
    /* Low byte (EWRPTL). */
    spi_start_cmd();
    spi_transfer(SPI_WCR(0x02));
    spi_transfer(0x11);
    spi_end_cmd();
    /* High byte (EWRPTH). */
    spi_start_cmd();
    spi_transfer(SPI_WCR(0x03));
    spi_transfer(0x00);
    spi_end_cmd();

    /* Write IP packet length. */
    tmp_length = length + 28;
    spi_start_cmd();
    spi_transfer(SPI_WBM);
    spi_transfer((uint8_t) tmp_length);
    spi_transfer((uint8_t) (tmp_length >> 8));
    spi_end_cmd();

    /* Write IP Identification field. */
    spi_start_cmd();
    spi_transfer(SPI_WBM);
    spi_transfer((uint8_t) identification);
    spi_transfer((uint8_t) (identification >> 8));
    spi_end_cmd();
    identification++;

    /* Move to the IP header Checksum field. */
    /* Low byte (EWRPTL). */
    spi_start_cmd();
    spi_transfer(SPI_WCR(0x02));
    spi_transfer(0x19);
    spi_end_cmd();

    /* Clear IP Checksum field. */
    spi_start_cmd();
    spi_transfer(SPI_WBM);
    spi_transfer(0x00);
    spi_transfer(0x00);
    spi_end_cmd();

    /* Move to the UDP header Length field. */
    /* Low byte (EWRPTL). */
    spi_start_cmd();
    spi_transfer(SPI_WCR(0x02));
    spi_transfer(0x25);
    spi_end_cmd();

    /* Write UDP packet length. */
    tmp_length = length + 8;
    spi_start_cmd();
    spi_transfer(SPI_WBM);
    spi_transfer((uint8_t) tmp_length);
    spi_transfer((uint8_t) (tmp_length >> 8));
    spi_end_cmd();

    /* Set end of transmission buffer. */
    tmp_length = length + 41;
    /* Low byte (ETXNDL). */
    spi_start_cmd();
    spi_transfer(SPI_WCR(0x06));
    spi_transfer((uint8_t) tmp_length);
    spi_end_cmd();
    /* High byte (ETXNDH). */
    spi_start_cmd();
    spi_transfer(SPI_WCR(0x07));
    spi_transfer((uint8_t) (tmp_length >> 8));
    spi_end_cmd();

    /* Start generating checksum. */
    spi_start_cmd();
    spi_transfer(SPI_BFS(0x1F));
    spi_transfer(0x30);
    spi_end_cmd();

    udp_state = STATE_CALCULATING;
}
void udp_service(void)
{
    uint8_t res_low, res_high;

    switch (udp_state) {
        case STATE_READY: break;
        case STATE_CALCULATING:
            /* Read ECON1. */
            spi_start_cmd();
            spi_transfer(SPI_RCR(0x1F));
            res_low = spi_transfer(SPI_NOP);
            spi_end_cmd();

            if (res_low & 0x02) { // DMAST bit set
                break;
            }
            
            STAT_ONE_PORT |= (1<<STAT_ONE_NUM);
            
            /* Read checksum. */
            /* Read low byte (EDMACSL). */
            spi_start_cmd();
            spi_transfer(SPI_RCR(0x16));
            res_low = spi_transfer(SPI_NOP);
            spi_end_cmd();
            /* Read high byte (EDMACSH). */
            spi_start_cmd();
            spi_transfer(SPI_RCR(0x17));
            res_high = spi_transfer(SPI_NOP);
            spi_end_cmd();

            /* Move to the IP header Checksum field. */
            /* Low byte (EWRPTL). */
            spi_start_cmd();
            spi_transfer(SPI_WCR(0x02));
            spi_transfer(0x19);
            spi_end_cmd();

            /* Write checksum. */
            spi_start_cmd();
            spi_transfer(SPI_WBM);
            spi_transfer(res_high);
            spi_transfer(res_low);
            spi_end_cmd();

            /* Transmit packet. */
            spi_start_cmd();
            spi_transfer(SPI_BFS(0x1F));
            spi_transfer(0x08);
            spi_end_cmd();

            udp_state = STATE_TRANSMITTING;
            break;
        case STATE_TRANSMITTING:
            /* Read ECON1. */
            spi_start_cmd();
            spi_transfer(SPI_RCR(0x1F));
            res_low = spi_transfer(SPI_NOP);
            spi_end_cmd();

            if (res_low & 0x08) { // TXRTS bit set
                break;
            }

            /* Set write pointer (EWRPTL) to beginning of data. */
            spi_start_cmd();
            spi_transfer(SPI_WCR(0x02));
            spi_transfer(0x2B);
            spi_end_cmd();

            break;
        default: break;
    }
}
