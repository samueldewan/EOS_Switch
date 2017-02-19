//
//  pindefinitions.h
//  EOS_Switch
//
//  Created by Samuel Dewan on 2016-10-17.
//  Copyright © 2016 Samuel Dewan. All rights reserved.
//

#ifndef pindefinitions_h
#define pindefinitions_h

#define TRIGGER_ONE_DDR     DDRD
#define TRIGGER_ONE_PORT    PORTD
#define TRIGGER_ONE_PIN     PIND
#define TRIGGER_ONE_NUM     PIND2

#define TRIGGER_TWO_DDR     DDRD
#define TRIGGER_TWO_PORT    PORTD
#define TRIGGER_TWO_PIN     PIND
#define TRIGGER_TWO_NUM     PIND3

#define STAT_ONE_DDR        DDRC
#define STAT_ONE_PORT       PORTC
#define STAT_ONE_NUM        PINC3

#define STAT_TWO_DDR        DDRC
#define STAT_TWO_PORT       PORTC
#define STAT_TWO_NUM        PINC2

// MARK: OSCCAL
#define OSCCAL_OUT_DDR      DDRB
#define OSCCAL_OUT_PORT     PORTB
#define OSCCAL_OUT_NUM      PINB0

#define OSCCAL_UP_DDR       DDRB
#define OSCCAL_UP_PORT      PORTB
#define OSCCAL_UP_PIN       PINB
#define OSCCAL_UP_NUM       PINB6

#define OSCCAL_DOWN_DDR     DDRB
#define OSCCAL_DOWN_PORT    PORTB
#define OSCCAL_DOWN_PIN     PINB
#define OSCCAL_DOWN_NUM     PINB7

#define OSCCAL_SAVE_DDR     PIND
#define OSCCAL_SAVE_PORT    PORTD
#define OSCCAL_SAVE_PIN     PIND
#define OSCCAL_SAVE_NUM     PIND5

//#define OSCCAL_USE_INDICATORS
//
//#define OSCCAL_INDICATOR_ONE_PORT   STAT_ONE_PORT
//#define OSCCAL_INDICATOR_ONE_NUM    STAT_ONE_NUM
//
//#define OSCCAL_INDICATOR_TWO_PORT   STAT_TWO_PORT
//#define OSCCAL_INDICATOR_TWO_NUM    STAT_TWO_NUM


// MARK: Flags
#define FLAG_OSCAL_MODE         7
#define FLAG_SERIAL_LOOPBACK    6
#define FLAG_SERIAL_TX_LOCK     5
#define FLAG_STAT_ONE_ON        1
#define FLAG_ONLINE             0

#endif /* pindefinitions_h */

