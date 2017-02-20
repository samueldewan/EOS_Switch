//
//  spi.c
//  EOS_Switch
//
//  Created by Samuel Dewan on 2017-02-19.
//  Copyright © 2017 Samuel Dewan. All rights reserved.
//

#include "spi.h"

#include "pindefinitions.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/atomic.h>

struct spi_transfer {
    int16_t bytes_out_left;
    int16_t bytes_in_left;
};

// MARK: Macros
#define SPI_IN_BUFFER_LENGTH    64
#define SPI_OUT_BUFFER_LENGTH   256

#define SPI_CONCURRENT_TRANSFERS 4

// Mark: Variables
static char spi_in_buffer[SPI_IN_BUFFER_LENGTH];
static char spi_out_buffer[SPI_OUT_BUFFER_LENGTH];

static volatile uint8_t in_buffer_insert_p;
static volatile uint8_t in_buffer_withdraw_p;
static volatile uint8_t out_buffer_insert_p;
static volatile uint8_t out_buffer_withdraw_p;

static uint8_t spi_current_transfer;
static spi_transfer_T spi_transfers[SPI_CONCURRENT_TRANSFERS];

// MARK: Static functions
static inline void circular_increment(uint8_t *byte, uint8_t max) {
    *byte = ((*byte + 1) > max) ? 0 : *byte + 1;
}

static inline void increment_buffer_insert (uint8_t* insert_p, uint8_t *withdraw_p, uint8_t buffer_length)
{
    circular_increment(insert_p, buffer_length);
    if (*insert_p == *withdraw_p) {
        (*withdraw_p)++;
    }
}

static inline void increment_buffer_withdraw (uint8_t *withdraw_p, uint8_t buffer_length)
{
    circular_increment(withdraw_p, buffer_length);
}

static void spi_write_byte (char data)
{
    spi_out_buffer[out_buffer_insert_p] = data;
    increment_buffer_insert(&out_buffer_insert_p, &out_buffer_withdraw_p, SPI_OUT_BUFFER_LENGTH);
    spi_service();
}

static void spi_write_byte_from_eeprom (uint16_t address)
{
    spi_out_buffer[out_buffer_insert_p] = eeprom_read_byte(data);
    increment_buffer_insert(&out_buffer_insert_p, &out_buffer_withdraw_p, SPI_OUT_BUFFER_LENGTH);
    spi_service();
}

static char spi_get_next_byte_out (void)
{
    char byte = spi_out_buffer[out_buffer_withdraw_p];
    if (out_buffer_withdraw_p != out_buffer_insert_p) {
        increment_buffer_withdraw(&out_buffer_withdraw_p, SPI_OUT_BUFFER_LENGTH);
    }
    return byte;
}

static void spi_in_append_byte (char data)
{
    spi_in_buffer[in_buffer_insert_p] = data;
    increment_buffer_insert(&in_buffer_insert_p, &in_buffer_withdraw_p, SPI_IN_BUFFER_LENGTH);
}

// MARK : Function definitions
void init_spi(void)
{
    // Pin configuration
    SPI_DDR |= (1<<SPI_SCK_NUM) | (1<<SPI_MOSI_NUM) | (1<<SPI_SS_NUM);
    SPI_DDR &= !(1<<SPI_MISO_NUM);
    
    // Enabe SPI with TX interupt and master mode
    // SPI mode is 0,0
    SPCR |= (1<<SPIE) | (1<<SPE) | (1<<MSTR);
    
    // Set SPI to double speed mode for speed = fosc/2
    SPSR |= (1<<SPI2X);
}

uint8_t spi_in_bytes_free (void)
{
    if (in_buffer_withdraw_p > in_buffer_insert_p) {
        return in_buffer_withdraw_p - in_buffer_insert_p;
    } else {
        return (SPI_IN_BUFFER_LENGTH - (in_buffer_insert_p - in_buffer_withdraw_p));
    }
}

uint8_t spi_in_bytes_avaliable (void)
{
    return SPI_IN_BUFFER_LENGTH - spi_in_bytes_free();
}

uint8_t spi_out_bytes_free (void)
{
    if (out_buffer_withdraw_p > out_buffer_insert_p) {
        return out_buffer_withdraw_p - out_buffer_insert_p;
    } else {
        return (SPI_OUT_BUFFER_LENGTH - (out_buffer_insert_p - out_buffer_withdraw_p));
    }
}

uint8_t spi_transfer_avaliable (void)
{
    uint8_t transfer_addr = ((spi_current_transfer + 1) >= SPI_CONCURRENT_TRANSFERS) ? 0 : spi_current_transfer + 1;
    
    if ((spi_transfers[transfer_addr].bytes_in_left <= 0) && (spi_transfers[transfer_addr].bytes_out_left <= 0)) {
        return 1;
    } else {
        return 0;
    }
}

void spi_start_transfer (uint8_t bytes_out, char* out_buffer, uint8_t bytes_in)
{
    uint8_t transfer_addr = ((spi_current_transfer + 1) >= SPI_CONCURRENT_TRANSFERS) ? 0 : spi_current_transfer + 1;
    
    spi_transfers[transfer_addr].bytes_in_left = (bytes_in == 0) ? -1 : bytes_in;
    spi_transfers[transfer_addr].bytes_out_left = bytes_out;
    
    for (int i = 0; i < bytes_out; i++) {
        spi_write_byte(out_buffer[i]);
    }
    
    spi_service();
}

void spi_start_transfer_from_eeprom (uint8_t bytes_out, uint16_t address, uint8_t bytes_in)
{
    uint8_t transfer_addr = ((spi_current_transfer + 1) >= SPI_CONCURRENT_TRANSFERS) ? 0 : spi_current_transfer + 1;
    
    spi_transfers[transfer_addr].bytes_in_left = (bytes_in == 0) ? -1 : bytes_in;
    spi_transfers[transfer_addr].bytes_out_left = bytes_out;
    
    for (int i = 0; i < bytes_out; i++) {
        spi_write_byte_from_eeprom(address + i);
    }
    
    spi_service();
}

char spi_read_byte (void)
{
    char byte = spi_in_buffer[in_buffer_withdraw_p];
    if (in_buffer_withdraw_p != in_buffer_insert_p) {
        increment_buffer_withdraw(&in_buffer_withdraw_p, SPI_IN_BUFFER_LENGTH);
    }
    return byte;
}

void spi_read_block (char* buffer, int length)
{
    for (int i = 0; (i < length) && (in_buffer_withdraw_p != in_buffer_insert_p); i++) {
        buffer[i] = spi_read_byte();
    }
}

void spi_service (void)
{
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        if ((!(flags & (1<<FLAG_SPI_LOCK))) && (out_buffer_withdraw_p != out_buffer_insert_p)) {
            flags |= (1<<FLAG_SPI_LOCK);
            SPDR = serial_out_buffer[out_buffer_withdraw_p];
            spi_transfers[spi_current_transfer].bytes_out_left--;
            increment_buffer_withdraw(&out_buffer_withdraw_p, SPI_OUT_BUFFER_LENGTH);
        }
    }
}

// MARK: Interupts
ISR (SPI_STC_vect)
{
    uint8_t status = SPSR;
    char data = SPDR;
    
    if (spi_transfers[spi_current_transfer].bytes_out_left > 0) {
        // Transmit next byte
        SPDR = spi_get_next_byte_out();
        spi_transfers[spi_current_transfer].bytes_out_left--;
    } else if (spi_transfers[spi_current_transfer].bytes_out_left == 0) {
        // TX done, send first dummy byte
        SPDR = 0xFF;
        spi_transfers[spi_current_transfer].bytes_out_left--;
    } else (spi_transfers[spi_current_transfer].bytes_out_left == -1) {
        // No bytes left to be transmitted
        if (spi_transfers[spi_current_transfer].bytes_in_left == -1) {
            // No bytes to be recieved
            circular_increment(&spi_current_transfer, SPI_CONCURRENT_TRANSFERS);
            if (spi_transfers[spi_current_transfer].bytes_out_left > 0) {
                SPDR = spi_get_next_byte_out();
                spi_transfers[spi_current_transfer].bytes_out_left--;
            } else {
                flags &= !(1<<FLAG_SPI_LOCK);
            }
        } else if (spi_transfers[spi_current_transfer].bytes_in_left == 0) {
            // Receive last byte
            spi_in_append_byte(data);
            
            circular_increment(&spi_current_transfer, SPI_CONCURRENT_TRANSFERS);
            if (spi_transfers[spi_current_transfer].bytes_out_left > 0) {
                SPDR = spi_get_next_byte_out();
                spi_transfers[spi_current_transfer].bytes_out_left--;
            } else {
                flags &= !(1<<FLAG_SPI_LOCK);
            }
        } else (spi_transfers[spi_current_transfer].bytes_in_left > 0) {
            // Recive byte and send next dummy byte
            spi_in_append_byte(data);
            spi_transfers[spi_current_transfer].bytes_in_left--;
            SPDR = 0xFF;
        }
    }
}




