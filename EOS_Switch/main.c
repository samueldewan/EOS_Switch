#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "pindefinitions.h"
#include "serial.h"

#include "libethernet/libethernet.h"

//MARK: Constants

// MARK: Function prototypes
void main_loop(void);

inline void print_ipinfo(void);
inline void print_targetinfo(void);
inline void print_payloads(void);
inline void process_set(char* property);

// MARK: Variable Definitions
volatile uint32_t millis;
volatile uint8_t flags;

static uint32_t last_status;

struct trigger_data {
    uint8_t t_one_dirty: 1;
    uint8_t t_two_dirty: 1;
    uint8_t t_one_state: 1;
    uint8_t t_two_state: 1;
} trigger_flags;

enum {NONE, PAYLOAD, SET} menu_status;
uint32_t menu_state;

// MARK: Strings
const char prompt_string[] PROGMEM = "> ";
const char welcome_string[] PROGMEM = "EOS-Switch\tv1.0\n";
const char help_string[] PROGMEM =  "Welcome to Serial Test. The following commands are avaliable:\n"  //62
                                    "\tIPINFO: Displays current IP configuration information\n"   //56
                                    "\tTARGETINFO: Displays current target information\n"   //49
                                    "\tPAYLOAD: Displays current payloads\n"    //37
                                    "\tSET: \"set help\" for more info\n";  //34
const char set_help_string[] PROGMEM =  "set_help\n";

// MARK: Functions

// MARK: Funciton definitions
void initIO(void)
{
    TRIGGER_ONE_DDR &= ~(1<<TRIGGER_ONE_NUM);
    TRIGGER_ONE_PORT |= (1<<TRIGGER_ONE_NUM);
    TRIGGER_TWO_DDR &= ~(1<<TRIGGER_TWO_NUM);
    TRIGGER_TWO_PORT |= (1<<TRIGGER_TWO_NUM);
    
    STAT_ONE_DDR |= (1<<STAT_ONE_NUM);
    STAT_TWO_DDR |= (1<<STAT_TWO_NUM);
    
    EICRA |= (1<<ISC10)|(1<<ISC00);                 // Trigger interupts on an logical
    EIMSK |= (1<<0)|(1<<1);                         // Enable interupts zero and one
}

void init_timers(void)
{
    // Timer 0 (clock)
    TCCR0B |= (1<<WGM01);                           // Set the Timer Mode to CTC
    TIMSK0 |= (1<<OCIE0A);                          // Set the ISR COMPA vector (enables COMP interupt)
    OCR0A = 125;                                    // 1000 Hz
    TCCR0B |= (1<<CS01)|(1<<CS00);                  // set prescaler to 64 and start timer 0
}

int main(void)
{
//    OSCCAL = eeprom_read_byte(OSCCAL_EEPROM_ADDRESS); // Load oscilator callibration from EEPROM
//    
//    if (!(OSCCAL_UP_PIN & (1<<OSCCAL_UP_NUM))) {  // If all settings switches are on except for switch four, enter OSCCAL mode
//        cli();
//        initIO();
//        init_timers();
//        //init_calibration();
//        sei();
//        
//        for (;;) {
//            //calibration_service();
//        }
//        return 0;
//    }
    
    cli();
    
	initIO();
    init_timers();
    init_serial();
    flags |= (1<<FLAG_SERIAL_LOOPBACK);             // Enable serial loopback
    
    sei();
    
//    eeprom_update_block("EOS-Switch", SETTING_HOSTNAME, 11);
//    uint8_t mac[] = {1, 2, 3, 4, 5, 6};
//    eeprom_update_block(mac, SETTING_MAC_ADDR, 6);
//    eeprom_update_dword(SETTING_IP_ADDR, MAKE_IP(192, 168, 1, 75));
//    eeprom_update_dword(SETTING_ROUTER_ADDR, MAKE_IP(192, 168, 1, 1));
//    eeprom_update_dword(SETTING_DNS_ADDR, MAKE_IP(192, 168, 1, 1));
//    eeprom_update_dword(SETTING_NTP_ADDR, MAKE_IP(132,246,11,227));
//    eeprom_update_dword(SETTING_NETMASK, MAKE_IP(255,255,255,0));
//    eeprom_update_byte(SETTING_GMT_OFFSET, 2);
//    eeprom_update_dword(SETTING_TARGET_IP, MAKE_IP(192, 168, 1, 100));
//    eeprom_update_word(SETTING_TARGET_PORT, 56789);
//    
//    eeprom_update_block("Lorem ipsum dolor sit amet, consectetur adipiscing elit. In elementum tincidunt imperdiet. Morbi elementum augue ut nisi venenatis facilisis. Integer sagittis ipsum at eros mattis ullamcorper nullam.", SETTING_T_ONE_RISE, 200);
//    eeprom_update_block("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin luctus nec mi sed rutrum. Vestibulum vitae vehicula eros, quis porttitor neque. Nulla nulla nisl, faucibus sit amet laoreet non posuere.", SETTING_T_ONE_FALL, 200);
//    eeprom_update_block("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus non ante in urna fringilla sodales sit amet non est. Sed eu eros sagittis, pharetra tortor ac, dapibus ex. Ut lacinia ex sed nullam.", SETTING_T_TWO_RISE, 200);
//    eeprom_update_block("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Ut volutpat erat sem. Donec nec magna mauris. Praesent rhoncus pharetra pellentesque. Integer a faucibus odio. In hac habitasse platea nullam.", SETTING_T_TWO_FALL, 200);
    
    trigger_flags.t_one_state = (TRIGGER_ONE_PORT & (1<<TRIGGER_ONE_NUM));
    trigger_flags.t_two_state = (TRIGGER_TWO_PORT & (1<<TRIGGER_TWO_NUM));
    
    serial_put_string_P(welcome_string);
    serial_put_string_P(prompt_string);

    for (;;) {
        main_loop();
	}
	return 0; // never reached
}

void main_loop ()
{
    serial_service();
    
    // Menu
    switch (menu_status) {
        case NONE:
            if (serial_has_line()) {
                menu_state = 0;
                
                char str[48];
                serial_get_line(str, 48);
                
                if (!strncasecmp(str, "help", 4)) {
                    serial_put_string_P(help_string);
                } else if (!strncasecmp(str, "clear", 5)) {
                    // Clear screen
                    serial_put_byte(0x1B);
                    serial_put_string("[2J");
                    // Bring cursor home
                    serial_put_byte(0x1B);
                    serial_put_string("[H");
                } else if (!strncasecmp(str, "ipinfo", 6)) {
                    print_ipinfo();
                } else if (!strncasecmp(str, "targetinfo", 10)) {
                    print_targetinfo();
                } else if (!strncasecmp(str, "payload", 7)) {
                    menu_status = PAYLOAD;
                    print_payloads();
                } else if (!strncasecmp(str, "set", 3)) {
                    menu_status = SET;
                    process_set(strtok(str, " "));
                } else {
                    serial_put_byte('"');
                    serial_put_string(str);
                    serial_put_byte('"');
                    serial_put_byte('\n');
                }
                
                if (menu_status == NONE) {
                    serial_put_string_P(prompt_string);
                }
            }
            break;
        case PAYLOAD:
            if (!(flags & (1<<FLAG_SERIAL_TX_LOCK))) {
                print_payloads();
            }
            break;
        case SET:
            process_set(NULL);
            break;
    }
    
    // Trigger One
    if (trigger_flags.t_one_dirty) {
        trigger_flags.t_one_dirty  = 0;
        
        if (trigger_flags.t_one_state && !(TRIGGER_ONE_PORT & (1<<TRIGGER_ONE_NUM))) {
            // was off, now on - Rising Edge
        } else if (!trigger_flags.t_one_state && (TRIGGER_ONE_PORT & (1<<TRIGGER_ONE_NUM))) {
            // was on, now off - Falling Edge
        }
        
        trigger_flags.t_one_state = (TRIGGER_ONE_PORT & (1<<TRIGGER_ONE_NUM));
    }
    
    // Trigger Two
    if (trigger_flags.t_two_dirty) {
        trigger_flags.t_two_dirty  = 0;
        
        if (trigger_flags.t_two_state && !(TRIGGER_TWO_PORT & (1<<TRIGGER_TWO_NUM))) {
            // was off, now on - Rising Edge
        } else if (!trigger_flags.t_two_state && (TRIGGER_TWO_PORT & (1<<TRIGGER_TWO_NUM))) {
            // was on, now off - Falling Edge
        }
        
        trigger_flags.t_two_state = (TRIGGER_TWO_PORT & (1<<TRIGGER_TWO_NUM));
    }
    
    // heart beat
    if ((millis - last_status) > 500) {
        last_status = millis;
        STAT_ONE_PORT ^= (1<<STAT_ONE_NUM);
    }
}

inline void print_addr(uint16_t addr, char delim, int len, int radix, char *temp)
{
    utoa(eeprom_read_byte(addr), temp, radix);
    serial_put_string(temp);
    for (int i = 1; i < len; i++) {
        serial_put_byte(delim);
        utoa(eeprom_read_byte(addr + i), temp, radix);
        serial_put_string(temp);
    }
    serial_put_byte('\n');
}

inline void print_ipinfo(void)
{
    char tmp[4];
    serial_put_string("\tIP Addr:\t");
    print_addr(SETTING_IP_ADDR, '.', 4, 10, tmp);
    
    serial_put_string("\tMAC Addr:\t");
    print_addr(SETTING_MAC_ADDR, ':', 6, 16, tmp);
    
    serial_put_string("\tRouter Addr:\t");
    print_addr(SETTING_ROUTER_ADDR, '.', 4, 10, tmp);
    
    serial_put_string("\tNetmask:\t");
    print_addr(SETTING_NETMASK, '.', 4, 10, tmp);
    
    serial_put_string("\tDNS Addr:\t");
    print_addr(SETTING_DNS_ADDR, '.', 4, 10, tmp);
    
    serial_put_string("\tNTP Addr:\t");
    print_addr(SETTING_NTP_ADDR, '.', 4, 10, tmp);
    
    serial_put_string("\tGMT Offset:\t");
    if (eeprom_read_byte(SETTING_GMT_OFFSET) >= 0) {
        serial_put_byte('+');
    }
    itoa(eeprom_read_byte(SETTING_GMT_OFFSET), tmp, 10);
    serial_put_string(tmp);
    serial_put_byte('\n');
    
    serial_put_string("\tHostname:\t");
    serial_put_from_eeprom(SETTING_HOSTNAME);
    serial_put_byte('\n');
}

inline void print_targetinfo(void)
{
    char tmp[6];
    serial_put_string("\tAddress:\t");
    print_addr(SETTING_TARGET_IP, '.', 4, 10, tmp);
    
    serial_put_string("\tPort:\t\t");
    utoa(eeprom_read_word(SETTING_TARGET_PORT), tmp, 10);
    serial_put_string(tmp);
    serial_put_byte('\n');
}

inline void print_payloads(void)
{
    switch (menu_state) {
        case 0:
            serial_put_string("\tTrigger one, rising edge:\n\t\t");
            serial_put_from_eeprom(SETTING_T_ONE_RISE);
            serial_put_byte('\n');
            menu_state++;
            break;
        case 1:
            serial_put_string("\tTrigger one, falling edge:\n\t\t");
            serial_put_from_eeprom(SETTING_T_ONE_FALL);
            serial_put_byte('\n');
            menu_state++;
            break;
        case 2:
            serial_put_string("\tTrigger two, rising edge:\n\t\t");
            serial_put_from_eeprom(SETTING_T_TWO_RISE);
            serial_put_byte('\n');
            menu_state++;
            break;
        case 3:
            serial_put_string("\tTrigger two, falling edge:\n\t\t");
            serial_put_from_eeprom(SETTING_T_TWO_FALL);
            serial_put_byte('\n');
            menu_state = 0;
            menu_status = NONE;
            serial_put_string_P(prompt_string);
            break;
    }
}

inline void process_set(char* property)
{
    serial_put_string_P(property);
    serial_put_byte('\n');
    
    if ((property != NULL) && (strlen(property) == 0)) {
        // Empty string
        serial_put_string_P(set_help_string);
        menu_status = NONE;
    } else if (property != NULL) {
        serial_put_string_P(property);
        serial_put_byte('\n');
        serial_put_string_P(set_help_string);
        menu_status = NONE;
    } else {
        serial_put_string_P("null\n");
        menu_status = NONE;
    }
}


// MARK: Interupt Service Routines
ISR (TIMER0_COMPA_vect)                             // Timer 0, called every millisecond
{
    millis++;
    if (flags & (1 << FLAG_OSCAL_MODE)) {
        OSCCAL_OUT_PORT ^= (1 << OSCCAL_OUT_NUM);
    }
}

ISR (INT0_vect)
{
    trigger_flags.t_one_dirty = 1;
}

ISR (INT1_vect)
{
    trigger_flags.t_two_dirty = 1;
}


