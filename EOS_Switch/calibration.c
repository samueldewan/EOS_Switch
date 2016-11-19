//
//  calibration.c
//  DMX-ISPLIT
//
//  Created by Samuel Dewan on 2016-10-09.
//  Copyright © 2016 Samuel Dewan. All rights reserved.
//

#include "calibration.h"

static uint8_t up_button = 2, down_button = 2, save_button = 2;
static uint32_t last_up_button, last_down_button, last_save_button, led_one_time, led_two_time;

void init_calibration (void) {
    OSCCAL_OUT_DDR |= (1<<OSCCAL_OUT_NUM);          // Initialize IO for frequency output
    flags |= (1 << FLAG_OSCAL_MODE);                // Set calibration mode to true
}

void calibration_service (void) {
    if (!(OSCCAL_UP_PIN & (1<<OSCCAL_UP_NUM))) {    // Up Button Debounce code
        if (up_button == 0) {
            up_button = 1;
            last_up_button = millis;
        } else if (up_button == 1){
            if (millis >= (last_up_button + 25)) {  // Check that button has been pressed at least 25ms
                //action
                led_one_time = millis + 50;         // Turn on STAT_ONE for at least 50 milliseconds
                //TODO: Why not OSCCAL++? Is there a good reason?
                OSCCAL = OSCCAL + 1;                // Increment OSCCAL
                up_button = 2;
            }
        }
    } else {
        up_button = 0;
    }
    
    if (!(OSCCAL_DOWN_PIN & (1<<OSCCAL_DOWN_NUM))) {
        if (down_button == 0) {
            down_button = 1;
            last_down_button = millis;
        } else if (down_button == 1){
            if (millis >= (last_down_button + 25)) {
                //action
                led_two_time = millis + 50;
                OSCCAL = OSCCAL - 1;
                down_button = 2;
            }
        }
    } else {
        down_button = 0;
    }
    
    if (!(OSCCAL_SAVE_PIN & (1<<OSCCAL_SAVE_NUM))) {
        if (save_button == 0) {
            save_button = 1;
            last_save_button = millis;
        } else if (save_button == 1){
            if (millis >= (last_save_button + 25)) {
                //action
                led_one_time = millis + 150;
                led_two_time = millis + 150;
                
                eeprom_update_byte(OSCCAL_EEPROM_ADDRESS, OSCCAL);  // Write the OSCCAL value to EEPROM
                
                save_button = 2;
            }
        }
    } else {
        save_button = 0;
    }
    
#ifdef OSCCAL_USE_INDICATORS
    if (led_one_time < millis) {
        OSCCAL_INDICATOR_ONE_PORT &= ~(1<<OSCCAL_INDICATOR_ONE_NUM);
    } else {
        OSCCAL_INDICATOR_ONE_PORT |= (1<<OSCCAL_INDICATOR_ONE_NUM);
    }
    if (led_two_time < millis) {
        OSCCAL_INDICATOR_TWO_PORT &= ~(1<<OSCCAL_INDICATOR_TWO_NUM);
    } else {
        OSCCAL_INDICATOR_TWO_PORT |= (1<<OSCCAL_INDICATOR_TWO_NUM);
    }
#endif
}
