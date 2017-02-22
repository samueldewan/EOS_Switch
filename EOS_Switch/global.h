//
//  global.h
//  EOS_Switch
//
//  Created by Samuel Dewan on 2016-10-17.
//  Copyright © 2016 Samuel Dewan. All rights reserved.
//

#include <stdint.h>                     // Int types are needed everywehre
#include <stdbool.h>                    // Bools are needed for avr-libethernet

#ifndef global_h
#define global_h

// MARK: Definitions
#define TIMER_FREQUENCY     1000

// MARK: Settings locations in EEPROM
#define OSCCAL_EEPROM_ADDRESS   0       // The EEPROM address at which the oscilator calibration is stored

#define SETTING_MAC_ADDR        10      // 6 bytes
#define SETTING_IP_ADDR         16      // 4 bytes
#define SETTING_ROUTER_ADDR     20      // 4 bytes
#define SETTING_NETMASK         24      // 4 bytes
#define SETTING_DNS_ADDR        28      // 4 bytes
#define SETTING_NTP_ADDR        32      // 4 bytes
#define SETTING_GMT_OFFSET      36      // 1 byte
#define SETTING_DCHP            37      // 1 byte
#define SETTING_HOSTNAME        38      // 32 bytes

#define SETTING_TARGET_IP       75      // 4 bytes
#define SETTING_TARGET_PORT     79      // 2 bytes

#define SETTING_T_ONE_RISE      224     // 200 bytes
#define SETTING_T_ONE_FALL      424     // 200 bytes
#define SETTING_T_TWO_RISE      624     // 200 bytes
#define SETTING_T_TWO_FALL      824     // 200 bytes

// MARK: Global variables
extern volatile uint32_t millis;        // Tracks the number of milliseconds elapsed since initilization

extern volatile uint8_t flags;          // Stores some global boolean flags

extern uint16_t stat_one_period;

#endif /* global_h */
