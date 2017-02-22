/*
 * udp.h
 * EOS_Switch
 *
 * Created by Ian Dewan on 2017-02-21.
 * Copyright © 2017 Ian Dewan. All rights reserved.
 */

#ifndef udp_h
#define udp_h

#include "enc28j60.h"

/**
 * Send the given buffer via UDP from the EEPROM.
 * @param buffer The address of the buffer in EEPROM.
 * @param length The length of the buffer.
 * @return 0 on success, 1 on error.
 */
extern int udp_eeprom_send(uint16_t buffer, uint8_t length);

/**
 * Send the given buffer via UDP from.
 * @param buffer The address of the buffer.
 * @param length The length of the buffer.
 * @return 0 on success, 1 on error.
 */
extern int udp_buffer_send(const uint8_t *buffer, uint8_t length);

/**
 * Actions to be performed in the main loop.
 */
extern void udp_service(void);

#endif /* enc28j60_h */
