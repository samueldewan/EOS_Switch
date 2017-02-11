//
//  serial.h
//  EOS_Switch
//
//  Created by Samuel Dewan on 2017-01-30.
//  Copyright © 2017 Samuel Dewan. All rights reserved.
//

#ifndef serial_h
#define serial_h

#include "global.h"
#include <avr/pgmspace.h>

/**
 *  Initilize the UART for serial I/O
 */
extern void init_serial (void);

/**
 *  Writes a string to the serial output
 *  @param str A nul terminated string to be written via serial
 */
extern void serial_put_string (char *str);

/**
 *  Writes a string to the serial output from program memory
 *  @param str A pointer to a programs space pointer to where the string is stored
 */
extern void serial_put_string_P (const char *str);

/**
 *  Writes a string to the serial output from EEPROM
 *  @param addr The addres of the string in EEPROM
 *  @param length The length of the string
 */
extern void serial_put_from_eeprom (uint16_t addr);

/**
 *  Write a character to the serial output
 *  @param c The character to be written
 */
extern void serial_put_byte (char c);

/**
 *  Read a bytes from the serial input as a string
 *  @param str The string in which the data should be stored
 *  @param len The maximum number of chars to be read from the serial input
 */
extern void serial_get_string (char *str, int len);

/**
 *  Determin if there is a full line avaliable to be read from the serial input
 *  @return 0 if there is no line avaliable, 1 if a line is avaliable
 */
extern int serial_has_line (void);

/**
 *  Read a bytes from the serial input as a string up to the next newline character
 *  @param str The string in which the data should be stored
 *  @param len The maximum number of chars to be read from the serial input
 */
extern void serial_get_line (char *str, int len);


/**
 *  Get a character from the serial input without consuming it
 *  @return The least recently recieved character on the serial buffer
 */
extern char serial_peak_byte (void);

/**
 *  Get a character from the serial input
 *  @return The least recently recieved character on the serial buffer
 */
extern char serial_get_byte (void);

/**
 *  Service to be run in each iteration of the main loop
 */
extern void serial_service (void);

#endif /* serial_h */
