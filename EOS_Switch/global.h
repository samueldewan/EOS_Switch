//
//  global.h
//  EOS_Switch
//
//  Created by Samuel Dewan on 2016-10-17.
//  Copyright © 2016 Samuel Dewan. All rights reserved.
//

#include <stdint.h>                     // Int types are needed everywehre

#ifndef global_h
#define global_h

// MARK: Definitions
#define TIMER_FREQUENCY     1000
#define OSCCAL_EEPROM_ADDRESS 0         // The EEPROM address at which the oscilator calibration is stored

// MARK: Global variables
extern volatile uint32_t millis;        // Tracks the number of milliseconds elapsed since initilization

extern volatile uint8_t flags;          // Stores some global boolean flags

#endif /* global_h */
