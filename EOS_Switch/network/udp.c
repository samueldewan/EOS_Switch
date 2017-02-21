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

static const uint8_t move_ip_length_l[2] PROGMEM = {SPI_WCR(0x02), 0x11}; // set EWRPTL to beginning of IP Length
static const uint8_t move_ip_length_h[2] PROGMEM = {SPI_WCR(0x03), 0x00}; // set EWRPTH to beginning of IP Length
static const uint8_t move_ip_csum_l[2] PROGMEM = {SPI_WCR(0x02), 0x19}; // set EWRPTL to beginning of IP Checksum
static const uint8_t clear_checksum[3] PROGMEM = {SPI_WBM, 0x00, 0x00}; // Write zeros over the checksum
static const uint8_t generate_checksum[2] PROGMEM = {SPI_BFS(0x1F), 0x30}; // Generate IP Checksum
static const uint8_t read_ECON1[1] PROGMEM = {SPI_RCR(0x1F)}; // read ECON1
static const uint8_t read_EDMACSL[1] PROGMEM = {SPI_RCR(0x16)}; // read EDMACSL
static const uint8_t read_EDMACSH[1] PROGMEM = {SPI_RCR(0x16)}; // read EDMACSL
static const uint8_t move_udp_length_l[2] PROGMEM = {SPI_WCR(0x02), 0x25}; // set EWRPTL to beginning of UDP Length
static const uint8_t transmit[2] PROGMEM = {SPI_BFS(0x1F), 0x08}; // Transmit packet
static const uint8_t move_data_l[2] PROGMEM = {SPI_WCR(0x02), 0x2B}; // set EWRPTL to beginning of data

static uint8_t udp_length[3] = {SPI_WBM, 0x00, 0x00};
static uint8_t ip_length[5] = {SPI_WBM, 0x00, 0x00, 0x00, 0x00}; // IP header Length AND Identification Fields
static uint8_t ip_csum[3] = {SPI_WBM, 0x00, 0x00};
static uint8_t set_ETXNDL[2] = {SPI_WCR(0x06), 0x00};
static uint8_t set_ETXNDH[2] = {SPI_WCR(0x07), 0x00};

static enum {
    STATE_READY,
    STATE_BEGIN,
    STATE_IP_LENGTH_PART,
    STATE_IP_LENGTH_POS,
    STATE_IP_LENGTH_WRITTEN,
    STATE_IP_CSUM_POS,
    STATE_IP_CSUM_CLEAR,
    STATE_CSUM_CALCULATING,
    STATE_CSUM_CALCULATING_2,
    STATE_CSUM_CALCULATED,
    STATE_CSUM_READ_1,
    STATE_CSUM_READ_2,
    STATE_CSUM_READ_3,
    STATE_CSUM_READ,
    STATE_IP_CSUM_POS_2,
    STATE_IP_CSUM_WRITTEN,
    STATE_UDP_LENGTH_POS,
    STATE_UDP_LENGTH_WRITTEN,
    STATE_ETXNDL_SET,
    STATE_ETXNDH_SET,
    STATE_TRANSMITTING,
    STATE_TRANSMITTING_2,
    STATE_SENT
} udp_state = STATE_READY;

int udp_buffer_send(const uint8_t *buffer, uint8_t length)
{
    uint16_t tmp_length;
    
    if (udp_state != STATE_READY) {
        return 1;
    }
    
    tmp_length = length + 8;
    udp_length[1] = (uint8_t) (tmp_length >> 8);
    udp_length[2] = (uint8_t) tmp_length;
    tmp_length += 20;
    ip_length[1] = (uint8_t) (tmp_length >> 8);
    ip_length[2] = (uint8_t) tmp_length;
    tmp_length += 13;
    set_ETXNDH[1] = (uint8_t) (tmp_length >> 8);
    set_ETXNDL[1] = (uint8_t) tmp_length;
    
    spi_start_transfer_with_cmd(SPI_WBM, length, buffer, 0);
    udp_state = STATE_BEGIN;
    
    return 0;
}

int udp_eeprom_send(uint16_t buffer, uint8_t length)
{
    uint16_t tmp_length;
    
    if (udp_state != STATE_READY) {
        return 1;
    }
    
    tmp_length = length + 8;
    udp_length[1] = (uint8_t) (tmp_length >> 8);
    udp_length[2] = (uint8_t) tmp_length;
    tmp_length += 20;
    ip_length[1] = (uint8_t) (tmp_length >> 8);
    ip_length[2] = (uint8_t) tmp_length;
    
    spi_start_transfer_with_cmd_from_eeprom(SPI_WBM, length, buffer, 0);
    udp_state = STATE_BEGIN;
    
    return 0;
}

void udp_service(void)
{
    switch (udp_state) {
        case STATE_READY: return;
        case STATE_BEGIN:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, move_ip_length_l, 0);
                udp_state = STATE_IP_LENGTH_PART;
            }
            break;
        case STATE_IP_LENGTH_PART:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, move_ip_length_h, 0);
                udp_state = STATE_IP_LENGTH_POS;
            }
            break;
        case STATE_IP_LENGTH_POS:
            if (spi_transfer_available()) {
                ip_length[4]++;
                if (ip_length[4] == 0) {
                    ip_length[3]++;
                }
                spi_start_transfer(5, ip_length, 0);
                udp_state = STATE_IP_LENGTH_WRITTEN;
            }
            break;
        case STATE_IP_LENGTH_WRITTEN:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, move_ip_csum_l, 0);
                udp_state = STATE_IP_CSUM_POS;
            }
            break;
        case STATE_IP_CSUM_POS:
            if (spi_transfer_available()) {
                spi_start_transfer_P(3, clear_checksum, 0);
                udp_state = STATE_IP_CSUM_CLEAR;
            }
            break;
        case STATE_IP_CSUM_CLEAR:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, generate_checksum, 0);
                udp_state = STATE_CSUM_CALCULATING;
            }
            break;
        case STATE_CSUM_CALCULATING:
            if (spi_transfer_available()) {
                spi_start_transfer_P(1, read_ECON1, 1);
                udp_state = STATE_CSUM_CALCULATING_2;
            }
            break;
        case STATE_CSUM_CALCULATING_2:
            if (spi_in_bytes_available() > 1) {
                if (!(spi_read_byte() && 0x20)) { // DMAST bit unset
                    udp_state = STATE_CSUM_CALCULATED;
                } else {
                    spi_start_transfer_P(1, read_ECON1, 1);
                }
            }
            break;
        case STATE_CSUM_CALCULATED:
            if (spi_transfer_available()) {
                spi_start_transfer_P(1, read_EDMACSL, 1);
                udp_state = STATE_CSUM_READ_1;
            }
            break;
        case STATE_CSUM_READ_1:
            if (spi_in_bytes_available() > 1) {
                ip_csum[2] = spi_read_byte();
                udp_state = STATE_CSUM_READ_2;
            }
            break;
        case STATE_CSUM_READ_2:
            if (spi_transfer_available()) {
                spi_start_transfer_P(1, read_EDMACSH, 1);
                udp_state = STATE_CSUM_READ_3;
            }
            break;
        case STATE_CSUM_READ_3:
            if (spi_in_bytes_available() > 1) {
                ip_csum[1] = spi_read_byte();
                udp_state = STATE_CSUM_READ;
            }
            break;
        case STATE_CSUM_READ:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, move_ip_csum_l, 0);
                udp_state = STATE_IP_CSUM_POS_2;
            }
            break;
        case STATE_IP_CSUM_POS_2:
            if (spi_transfer_available()) {
                spi_start_transfer(3, ip_csum, 0);
                udp_state = STATE_IP_CSUM_WRITTEN;
            }
            break;
        case STATE_IP_CSUM_WRITTEN:
            if (spi_transfer_available()) {
                spi_start_transfer(2, move_udp_length_l, 0);
                udp_state = STATE_UDP_LENGTH_POS;
            }
            break;
        case STATE_UDP_LENGTH_POS:
            if (spi_transfer_available()) {
                spi_start_transfer(3, udp_length, 0);
                udp_state = STATE_UDP_LENGTH_WRITTEN;
            }
            break;
        case STATE_UDP_LENGTH_WRITTEN:
            if (spi_transfer_available()) {
                spi_start_transfer(2, set_ETXNDL, 0);
                udp_state = STATE_ETXNDL_SET;
            }
            break;
        case STATE_ETXNDL_SET:
            if (spi_transfer_available()) {
                spi_start_transfer(2, set_ETXNDH, 0);
                udp_state = STATE_ETXNDH_SET;
            }
            break;
        case STATE_ETXNDH_SET:
            if (spi_transfer_available()) {
                spi_start_transfer_P(2, transmit, 0);
                udp_state = STATE_TRANSMITTING;
            }
            break;
        case STATE_TRANSMITTING:
            if (spi_transfer_available()) {
                spi_start_transfer_P(1, read_ECON1, 1);
                udp_state = STATE_TRANSMITTING_2;
            }
            break;
        case STATE_TRANSMITTING_2:
            if (spi_in_bytes_available() > 1) {
                if (!(spi_read_byte() && 0x08)) { // TXRTS bit unset
                    udp_state = STATE_SENT;
                } else {
                    spi_start_transfer_P(1, read_ECON1, 1);
                }
            }
            break;
        case STATE_SENT:
            if (spi_transfer_available()) {
                spi_start_transfer_P(1, move_data_l, 10);
                udp_state = STATE_READY;
            }
            break;
        default: return;
    }
}

int udp_ready(void)
{
    return (udp_state == STATE_READY) && spi_transfer_available();
}
