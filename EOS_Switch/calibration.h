//
//  calibration.h
//  DMX-ISPLIT
//
//  Created by Samuel Dewan on 2016-10-09.
//  Copyright © 2016 Samuel Dewan. All rights reserved.
//

#ifndef debounce_h
#define debounce_h

#include "global.h"
#include "pindefinitions.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>

/**
 *  Initilize the device in oscilator calibration mode
 */
void init_calibration (void);

/**
 *  IO for oscilator calibration
 */
void calibration_service (void);

#endif /* debounce_h */
