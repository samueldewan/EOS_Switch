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

typedef struct spi_transfer spi_transfer_T;


/**
 *  Initialize the SPI interface
 */
extern void init_spi(void);

/**
 *  Determin how much space is avaliable in the input buffer
 *  @return The number of free bytes in the SPI input buffer
 */
extern uint8_t spi_in_bytes_free (void);

/*  Determin how many bytes are avaliable to be read from the SPI input buffer
 *  @return The number bytes that are avaliable to be read from the SPI input buffer
 */
extern uint8_t spi_in_bytes_available (void);

/**
 *  Determin how much space is avaliable in the output buffer
 *  @return The number of free bytes in the SPI output buffer
 */
extern uint8_t spi_out_bytes_free (void);

/**
 *  Determin if there is an SPI transfer slot free
 *  @return 0 if there is no free slot, 1 if a slot is free
 */
extern uint8_t spi_transfer_available (void);

/**
 *  Start a transfer on the SPI interface
 *  @param bytes_out The number of bytes to be transmitted
 *  @param out_buffer A pointer the data to be sent
 *  @param bytes_in The number of response bytes to store
 */
extern void spi_start_transfer (uint8_t bytes_out, const uint8_t* out_buffer, uint8_t bytes_in);

/**
 *  Start a transfer on the SPI interface, prefixed by a command byte
 *  @param cmd The command byte to be prefixed to the transfer
 *  @param bytes_out The number of bytes to be transmitted (not including command byte)
 *  @param out_buffer A pointer the data to be sent
 *  @param bytes_in The number of response bytes to store
 */
extern void spi_start_transfer_with_cmd (uint8_t cmd, uint8_t bytes_out, const uint8_t* out_buffer, uint8_t bytes_in);

/**
 *  Start a transfer on the SPI interface with source data from program space
 *  @param bytes_out The number of bytes to be transmitted
 *  @param out_buffer A program space pointer the data to be sent
 *  @param bytes_in The number of response bytes to store
 */
extern void spi_start_transfer_P (uint8_t bytes_out, const uint8_t* out_buffer, uint8_t bytes_in);

/**
 *  Start a transfer on the SPI interface with source data from program space, prefixed by a command byte
 *  @param cmd The command byte to be prefixed to the transfer
 *  @param bytes_out The number of bytes to be transmitted (not including command byte)
 *  @param out_buffer A program space pointer the data to be sent
 *  @param bytes_in The number of response bytes to store
 */
extern void spi_start_transfer_with_cmd_P (uint8_t cmd, uint8_t bytes_out, const uint8_t* out_buffer, uint8_t bytes_in);

/**
 *  Start a transfer on the SPI interface with source data from EEPROM
 *  @param bytes_out The number of bytes to be transmitted
 *  @param address An EEPROM pointer to the data to be sent
 *  @param bytes_in The number of response bytes to store
 */
extern void spi_start_transfer_from_eeprom (uint8_t bytes_out, uint16_t address, uint8_t bytes_in);

/**
 *  Start a transfer on the SPI interface with source data from EEPROM, prefixed by a command byte
 *  @param cmd The command byte to be prefixed to the transfer
 *  @param bytes_out The number of bytes to be transmitted (not including command byte)
 *  @param address An EEPROM pointer to the data to be sent
 *  @param bytes_in The number of response bytes to store
 */
extern void spi_start_transfer_with_cmd_from_eeprom (uint8_t cmd, uint8_t bytes_out, uint16_t address, uint8_t bytes_in);

/**
 *  Get a byte from the input buffer
 *  @return The oldest byte from the input buffer
 */
extern char spi_read_byte (void);

/**
 *  Read a block of bytes from the input buffer
 *  @param buffer The buffer in which to store the data from the buffer
 *  @param length The numberof bytes to read from the buffer
 */
void spi_read_block (char* buffer, int length);

/**
 *  Service to be run in each main loop
 */
extern void spi_service (void);

#endif /* spi_h */
