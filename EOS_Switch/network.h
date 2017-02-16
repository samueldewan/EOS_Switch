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
 * Gets the current IP address
 * @return The IP address
 */
extern uint32_t network_get_ip_addr(void);

/**
 * Gets the address of the router
 * @return The IP address
 */
extern uint32_t network_get_router_addr(void);

/**
 * Gets the the netmask
 * @return The netmask
 */
extern uint32_t network_get_netmask(void);

/**
 *  Network actions to be performed in each main loop
 */
extern void network_service (void);
