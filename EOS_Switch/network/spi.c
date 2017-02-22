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
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

// MARK : Function definitions
void init_spi(void)
{
    // Pin configuration
    SPI_DDR |= (1<<SPI_SCK_NUM) | (1<<SPI_MOSI_NUM) | (1<<SPI_SS_NUM);
    SPI_DDR &= !(1<<SPI_MISO_NUM);
    
    // Enabe SPI in master mode
    // SPI mode is 0,0
    SPCR |= (1<<SPE) | (1<<MSTR);
    
    // Set SPI to double speed mode for speed = fosc/2
    SPSR |= (1<<SPI2X);
}

uint8_t spi_transfer (uint8_t byte_out)
{
    SPDR = byte_out;
    while(!(SPSR & (1<<SPIF))) {
        ;
    }
    return SPDR;
}

uint8_t spi_transfer_P (const uint8_t *byte_out)
{
    SPDR = pgm_read_byte(byte_out);
    while(!(SPSR & (1<<SPIF))) {
        ;
    }
    return SPDR;
}

uint8_t spi_transfer_from_eeprom (uint16_t address)
{
    SPDR = eeprom_read_byte(address);
    while(!(SPSR & (1<<SPIF))) {
        ;
    }
    return SPDR;
}

void spi_start_cmd (void)
{
    SPI_PORT &= !(1<<SPI_SS_NUM);
}

void spi_end_cmd (void)
{
    SPI_PORT |= (1<<SPI_SS_NUM);
}

void spi_service (void)
{
    
}





