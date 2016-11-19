//
//  pindefinitions.h
//  EOS_Switch
//
//  Created by Samuel Dewan on 2016-10-17.
//  Copyright © 2016 Samuel Dewan. All rights reserved.
//

#ifndef pindefinitions_h
#define pindefinitions_h

// MARK: Status LEDs
#define STAT_ONE_DDR    DDRD
#define STAT_ONE_PORT   PORTD
#define STAT_ONE_NUM    PIND5

#define STAT_TWO_DDR    DDRD
#define STAT_TWO_PORT   PORTD
#define STAT_TWO_NUM    PIND6

// MARK: Settings Switches
#define SET_ONE_DDR     DDRD
#define SET_ONE_PORT    PORTD
#define SET_ONE_PIN     PIND
#define SET_ONE_NUM     PIND2

#define SET_TWO_DDR     DDRD
#define SET_TWO_PORT    PORTD
#define SET_TWO_PIN     PIND
#define SET_TWO_NUM     PIND3

#define SET_THREE_DDR   DDRD
#define SET_THREE_PORT  PORTD
#define SET_THREE_PIN   PIND
#define SET_THREE_NUM   PIND4

// MARK: Trigger Switches
#define TRIG_ONE_DDR    DDRC
#define TRIG_ONE_PORT   PORTC
#define TRIG_ONE_PIN    PINC
#define TRIG_ONE_NUM    PINC0

#define TRIG_TWO_DDR    DDRC
#define TRIG_TWO_PORT   PORTC
#define TRIG_TWO_PIN    PINC
#define TRIG_TWO_NUM    PINC1

#define TRIG_THREE_DDR  DDRC
#define TRIG_THREE_PORT PORTC
#define TRIG_THREE_PIN  PINC
#define TRIG_THREE_NUM  PINC2

#define TRIG_FOUR_DDR   DDRC
#define TRIG_FOUR_PORT  PORTC
#define TRIG_FOUR_PIN   PINC
#define TRIG_FOUR_NUM   PINC3

#define TRIG_FIVE_DDR   DDRC
#define TRIG_FIVE_PORT  PORTC
#define TRIG_FIVE_PIN   PINC
#define TRIG_FIVE_NUM   PINC4

#define TRIG_SIX_DDR    DDRC
#define TRIG_SIX_PORT   PORTC
#define TRIG_SIX_PIN    PINC
#define TRIG_SIX_NUM    PINC5

#define TRIGGERS_REG    PINC

// MARK: OSCCAL
#define OSCCAL_OUT_DDR  DDRB
#define OSCCAL_OUT_PORT PORTB
#define OSCCAL_OUT_NUM  PINB0

#define OSCCAL_UP_PIN       SET_ONE_PIN
#define OSCCAL_UP_NUM       SET_ONE_NUM

#define OSCCAL_DOWN_PIN     SET_TWO_PIN
#define OSCCAL_DOWN_NUM     SET_TWO_NUM

#define OSCCAL_SAVE_PIN     SET_THREE_PIN
#define OSCCAL_SAVE_NUM     SET_THREE_NUM

#define OSCCAL_USE_INDICATORS

#define OSCCAL_INDICATOR_ONE_PORT   STAT_ONE_PORT
#define OSCCAL_INDICATOR_ONE_NUM    STAT_ONE_NUM

#define OSCCAL_INDICATOR_TWO_PORT   STAT_TWO_PORT
#define OSCCAL_INDICATOR_TWO_NUM    STAT_TWO_NUM

// MARK: Settings
#define SETTINGS_REG        PIND

#define SETTING_MODE        SET_ONE_NUM
#define SETTING_SWAP        SET_TWO_NUM
#define SETTING_MERGE_MODE  SET_THREE_NUM

#define SETTINGS_MASK       0b00011100
#define SETTINGS_OSCAL_TRIG 0b00010000

// MARK: Flags
#define FLAG_SETTINGS_DIRTY 0
#define FLAG_TRIGGERS_DIRTY 1
#define FLAG_OSCAL_MODE     7

#endif /* pindefinitions_h */
