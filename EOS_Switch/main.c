#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "global.h"
#include "pindefinitions.h"

//MARK: Constants

// MARK: Function prototypes
void main_loop(void);

// MARK: Variable Definitions
volatile uint32_t millis;

volatile uint8_t flags;

// MARK: Funciton definitions
void initIO(void) {
    // LEDs (outputs)
    STAT_ONE_DDR |= (1<<STAT_ONE_NUM);          // Set status LED 1 as an output
    STAT_TWO_DDR |= (1<<STAT_TWO_NUM);          // Set status LED 2 as an output
    
    // Settings Switches
    SET_ONE_DDR &= ~(1<<SET_ONE_NUM);           // Set SET switch 1 as an input
    SET_ONE_PORT |= (1<<SET_ONE_NUM);           // Enable pullup on SET switch 1
    SET_TWO_DDR &= ~(1<<SET_TWO_NUM);           // Set SET switch 1 as an input
    SET_TWO_PORT |= (1<<SET_TWO_NUM);           // Enable pullup on SET switch 1
    SET_THREE_DDR &= ~(1<<SET_THREE_NUM);       // Set SET switch 1 as an input
    SET_THREE_PORT |= (1<<SET_THREE_NUM);       // Enable pullup on SET switch 1
    
    // Trigger Inputs
    TRIG_ONE_DDR &= ~(1<<TRIG_ONE_NUM);         // Set TRIG 1 as an input
    TRIG_ONE_PORT |= (1<<TRIG_ONE_NUM);         // Enable pullup on TRIG 1
    TRIG_TWO_DDR &= ~(1<<TRIG_TWO_NUM);         // Set TRIG 1 as an input
    TRIG_TWO_PORT |= (1<<TRIG_TWO_NUM);         // Enable pullup on TRIG 1
    TRIG_THREE_DDR &= ~(1<<TRIG_THREE_NUM);     // Set TRIG 1 as an input
    TRIG_THREE_PORT |= (1<<TRIG_THREE_NUM);     // Enable pullup on TRIG 1
    TRIG_FOUR_DDR &= ~(1<<TRIG_FOUR_NUM);       // Set TRIG 1 as an input
    TRIG_FOUR_PORT |= (1<<TRIG_FOUR_NUM);       // Enable pullup on TRIG 1
    TRIG_FIVE_DDR &= ~(1<<TRIG_FIVE_NUM);       // Set TRIG 1 as an input
    TRIG_FIVE_PORT |= (1<<TRIG_FIVE_NUM);       // Enable pullup on TRIG 1
    TRIG_SIX_DDR &= ~(1<<TRIG_SIX_NUM);         // Set TRIG 1 as an input
    TRIG_SIX_PORT |= (1<<TRIG_SIX_NUM);         // Enable pullup on TRIG 1
    
    // Trigger Interupt (PCINT8-13)
    PCICR |= (1<<PCIE1)                         // Enable PCINT8-14
    
    
    // Settings Interupt (PCINT18-20)
    PCICR |= (1<<PCIE2)                         // Enable PCINT16-23
}

void init_timers(void) {
    // Timer 1 (clock)
    TCCR1B |= (1 << WGM12);                     // Set the Timer Mode to CTC
    TIMSK1 |= (1 << OCIE1A);                    // Set the ISR COMPA vector (enables COMP interupt)
    OCR1A = 1000;                               // Set timer COMP to 1000 microseconds
    TCCR1B |= (1 << CS11);                      // set prescaler to 8 and start timer 1
}

int main(void) {
    OSCCAL = eeprom_read_byte(OSCCAL_EEPROM_ADDRESS); // Load oscilator callibration from EEPROM
    
    if ((SETTINGS_REG & SETTINGS_MASK) == SETTINGS_OSCAL_TRIG) {  // If all settings switches are on except for switch four, enter OSCCAL mode
        cli();
        initIO();
        init_timers();
        //init_calibration();
        sei();
        
        for (;;) {
            //calibration_service();
        }
        return 0;
    }
    
    cli();
	initIO();
    init_timers();
    
    sei();

    for (;;) {
        main_loop();
	}
	return 0; // never reached
}

void main_loop () {
    if (flags & (1<<FLAG_SETTINGS_DIRTY)) {
        
    }
    
    if (flags & (1<<FLAG_TRIGGERS_DIRTY)) {
        
    }
}

ISR (TIMER1_COMPA_vect) {                       // Timer 1, called every millisecond
    millis++;
    if (flags & (1 << FLAG_OSCAL_MODE)) {
        OSCCAL_OUT_PORT ^= (1 << OSCCAL_OUT_NUM);
    }
}
