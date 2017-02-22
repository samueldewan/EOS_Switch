//
//  spi.h
//  EOS_Switch
//
//  Created by Samuel Dewan on 2017-02-19.
//  Copyright © 2017 Samuel Dewan. All rights reserved.
//

#ifndef spi_h
#define spi_h

#include "global.h"

extern volatile uint8_t out_buffer_insert_p;
extern volatile uint8_t out_buffer_withdraw_p;

typedef struct spi_transfer spi_transfer_T;

/**
 *  Initialize the SPI interface
 */
extern void init_spi(void);

/**
 *  Send and recieve a byte over the SPI bus
 *  @param byte_out The byte to be sent
 *  @return The byte recieved
 */
extern uint8_t spi_transfer (uint8_t byte_out);

/**
 *  Send a byte from program space and recieve a byte over the SPI bus
 *  @param byte_out A program space pointer to the byte to be sent
 *  @return The byte recieved
 */
extern uint8_t spi_transfer_P (const uint8_t *byte_out);

/**
 *  Send a bytes from eeprom and recieve a byte over the SPI bus
 *  @param address The eeprom address from which a byte should be sent
 *  @return The byte recieved
 */
extern uint8_t spi_transfer_from_eeprom (uint16_t address);

/**
 *  Service to be run in each main loop
 */
extern void spi_service (void);

#endif /* spi_h */
