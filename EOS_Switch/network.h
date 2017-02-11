//
//  network.h
//  EOS_Switch
//
//  Created by Samuel Dewan on 2017-02-02.
//  Copyright © 2017 Samuel Dewan. All rights reserved.
//

#ifndef network_h
#define network_h

#include "global.h"

#endif /* network_h */

/**
 *  Initilize the network interface
 */
extern void init_network (void);

/**
 *  Reinitialize the network
 */
extern void reinit_network (void);

/**
 *  Sends a packet to the console
 *  @param address The address of the data in EEPROM
 *  @param length The length of the data buffer
 *  @return The number of bytes which where sent
 */
int network_send_from_eeprom (uint16_t address, int length);

/**
 * Gets the current IP address as an array of bytes
 * @param address An array of at least 4 bytes in which the ip address will be placed
 */
extern void network_get_ip_addr(uint8_t *address);

/**
 *  Network actions to be performed in each main loop
 */
extern void network_service (void);
