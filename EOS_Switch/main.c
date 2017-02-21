#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "pindefinitions.h"
#include "serial.h"
#include "network/enc28j60.h"
#include "network/udp.h"

//MARK: Constants

// MARK: Function prototypes
static void main_loop(void);

static inline void print_prompt(void);
static inline void print_ipinfo(void);
static inline void print_targetinfo(void);
static inline void print_payloads(void);
static inline void print_dhcp(void);
static inline void process_set(char* property);

// MARK: Variable Definitions
volatile uint32_t millis;
volatile uint8_t flags;

static uint32_t last_stat_one_time;
static uint16_t stat_one_period;

struct trigger_data {
    uint8_t t_one_dirty: 1;
    uint8_t t_two_dirty: 1;
    uint8_t t_one_state: 1;
    uint8_t t_two_state: 1;
} trigger_flags;

enum {NONE, PAYLOAD, SET} menu_status;
uint32_t menu_state;
static char menu_buffer[200];


// MARK: Strings
static const char prompt_string[] PROGMEM = "> ";
static const char prompt_string_offline[] PROGMEM = "(offline)> ";
static const char welcome_string[] PROGMEM = "EOS-Switch\tv1.0\n";
static const char help_string[] PROGMEM = "The following commands are avaliable:\n"  //43
                                          "\tIPINFO: Displays current IP configuration information.\n"   //57
                                          "\tTARGETINFO: Displays current target information.\n"   //50
                                          "\tDHCP: Display DCHP configutation.\n"     //35
                                          "\tPAYLOAD: Displays current payloads.\n"    //38
                                          "\tSET: See \"set help\".\n";  //22
static const char set_help_string[] PROGMEM = "Synopsis: set <class>.<key>\n"
                                              "Classes are as follows:\n"
                                              "\tIP: Network settings. (\"set help ip 1\" for more)\n"
                                              "\tTARGET: Address information for target. (\"set help target\" for more)\n"
                                              "\tPAYLOAD: Payloads to be transmitted. (\"set help payload\" for more)\n";
static const char set_help_ip_1_string[] PROGMEM = "The following keys are under ip:\n"
                                                   "\tIP: IP address.\n"
                                                   "\tDHCP: Enable DHCP.\n"
                                                   "\tMAC: MAC address.\n"
                                                   "\tROUTER: Router Address.\n"
                                                   "\tNETMASK: Netmask.\n"
                                                   "\t(\"set help ip 2 for more\")\n";
static const char set_help_ip_2_string[] PROGMEM = "The following additional keys are under ip:\n"
                                                   "\tDNS: DNS server address.\n"
                                                   "\tNTP: NTP server address.\n"
                                                   "\tGMT: Offset from GMT.\n"
                                                   "\tHOSTNAME: Hostname.\n"
                                                   "\t(Device must be restarted for IP changes to take effect).\n";
static const char set_help_target_string[] PROGMEM = "The following keys are under target:\n"
                                                     "\tIP: IP Address of target.\n"
                                                     "\tPORT: Port to which payloads should be sent.\n"
                                                     "\t(Device must be restarted for target changes to take effect).\n";
static const char set_help_payload_string[] PROGMEM = "The following keys are under payload:\n"
                                                      "\tONERISE: Packet sent on trigger one rising edge.\n"
                                                      "\tONEFALL: Packet sent on trigger one falling edge.\n"
                                                      "\tTWORISE: Packet sent on trigger two rising edge.\n"
                                                      "\tTWOFALL: Packet sent on trigger two falling edge.\n";

static const char set_ip_prompt_string[] PROGMEM =      "Enter address (a.b.c.d): ";
static const char set_mac_prompt_string[] PROGMEM =     "Enter address (a:b:c:d:e:f): ";
static const char set_port_prompt_string[] PROGMEM =    "Enter port: ";
static const char set_gmt_prompt_string[] PROGMEM =     "Enter gmt offset (eg. +2): ";
static const char set_payload_prompt_string[] PROGMEM = "Enter payload (max 199 chars, # = enter): ";
static const char set_dhcp_prompt_string[] PROGMEM =    "Enable DHCP? (0 = no, 1 = yes): ";

static const char set_unkown_property_string[] PROGMEM = "Unkown property:";
static const char menu_unkown_cmd_prt1[] PROGMEM = "Unkown command: ";
static const char menu_unkown_cmd_prt2[] PROGMEM = "Try \"help\" for a list of avaliable commands.\n";

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
    
    flags |= (1<<FLAG_STAT_ONE_ON);
    
    init_enc28j60();
    
//    eeprom_update_block("EOS-Switch", SETTING_HOSTNAME, 11);
//    uint8_t mac[] = {0xDD, 0x66, 0x0C, 0x0A, 0x2A, 0x79};
//    eeprom_update_block(mac, SETTING_MAC_ADDR, 6);
//    eeprom_update_dword(SETTING_IP_ADDR, MAKE_IP(169, 254, 6, 150));
//    eeprom_update_dword(SETTING_ROUTER_ADDR, MAKE_IP(169, 254, 6, 133));
//    eeprom_update_dword(SETTING_DNS_ADDR, MAKE_IP(169, 254, 6, 133));
//    eeprom_update_dword(SETTING_NTP_ADDR, MAKE_IP(132,246,11,227));
//    eeprom_update_dword(SETTING_NETMASK, MAKE_IP(255,255,255,0));
//    eeprom_update_byte(SETTING_GMT_OFFSET, 2);
//    eeprom_update_byte(SETTING_DCHP, 0);
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
    print_prompt();

    for (;;) {
        main_loop();
    }
    return 0; // never reached
}

static const char menu_cmd_help[] PROGMEM =         "help";
static const char menu_cmd_clear[] PROGMEM =        "clear";
static const char menu_cmd_ipinfo[] PROGMEM =       "ipinfo";
static const char menu_cmd_targetinfo[] PROGMEM =   "targetinfo";
static const char menu_cmd_dhcp[] PROGMEM =         "dhcp";
static const char menu_cmd_payoad[] PROGMEM =       "payload";
static const char menu_cmd_set[] PROGMEM =          "set";
static const char menu_cmd_testnet[] PROGMEM =      "testnet";

static void main_loop ()
{
    
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
    
    serial_service();
    
    // Menu
    switch (menu_status) {
        case NONE:
            if (serial_has_line()) {
                menu_state = 0;
                
                serial_get_line(menu_buffer, 200);
                
                if (!strncasecmp_P(menu_buffer, menu_cmd_help, 4)) {
                    serial_put_string_P(help_string);
                } else if (!strncasecmp_P(menu_buffer, menu_cmd_clear, 5)) {
                    // Clear screen
                    serial_put_byte(0x1B);
                    serial_put_string("[2J");
                    // Bring cursor home
                    serial_put_byte(0x1B);
                    serial_put_string("[H");
                } else if (!strncasecmp_P(menu_buffer, menu_cmd_ipinfo, 6)) {
                    print_ipinfo();
                } else if (!strncasecmp_P(menu_buffer, menu_cmd_targetinfo, 10)) {
                    print_targetinfo();
                } else if (!strncasecmp_P(menu_buffer, menu_cmd_dhcp, 4)) {
                    print_dhcp();
                }else if (!strncasecmp_P(menu_buffer, menu_cmd_payoad, 7)) {
                    menu_status = PAYLOAD;
                    print_payloads();
                } else if (!strncasecmp_P(menu_buffer, menu_cmd_set, 3)) {
                    menu_status = SET;
                    process_set(menu_buffer + 3);
                } else if (!strncasecmp_P(menu_buffer, menu_cmd_testnet, 3)) {
                    while (!udp_ready()) {
                        // TODO: Remove busy-waiting
                    }
                    udp_buffer_send((uint8_t*)(menu_buffer + 8), strlen(menu_buffer) - 8);
                } else {
                    serial_put_string_P(menu_unkown_cmd_prt1);
                    serial_put_byte('"');
                    serial_put_string(menu_buffer);
                    serial_put_byte('"');
                    serial_put_byte('\n');
                    serial_put_string_P(menu_unkown_cmd_prt2);
                }
                
                if (menu_status == NONE) {
                    print_prompt();
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

    // STAT_ONE
    if (flags & (1<<FLAG_STAT_ONE_ON)) {
        if (stat_one_period != 0) {
            if ((millis - last_stat_one_time) > stat_one_period) {
                last_stat_one_time = millis;
                STAT_ONE_PORT ^= (1<<STAT_ONE_NUM);
            }
        } else {
            STAT_ONE_PORT |= (1<<STAT_ONE_NUM);
        }
    } else {
        STAT_ONE_PORT &= !(1<<STAT_ONE_NUM);
    }
    
    // Network
    enc28j60_service();
    udp_service();
    
    if (enc28j60_ready()) {
        stat_one_period = 500;
    }
}

static inline void print_prompt(void) {
    if (flags & (1<<FLAG_ONLINE)) {
        serial_put_string_P(prompt_string);
    } else {
        serial_put_string_P(prompt_string_offline);
    }
}

static inline void print_addr(uint16_t addr, char delim, int len, int radix, char *temp)
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

static const char menu_ipinfo_dhcp_string[] PROGMEM =      "\tDHCP Enabled:\t";
static const char menu_ipinfo_dhcp_string_yes[] PROGMEM =  "YES\n";
static const char menu_ipinfo_dhcp_string_no[] PROGMEM =   "NO\n";
static const char menu_ipinfo_ip_string[] PROGMEM =        "\tIP Address:\t";
static const char menu_ipinfo_mac_string[] PROGMEM =       "\tMAC Address:\t";
static const char menu_ipinfo_router_string[] PROGMEM =    "\tRouter Address:\t";
static const char menu_ipinfo_netmask_string[] PROGMEM =   "\tNetmask:\t";
static const char menu_ipinfo_dns_string[] PROGMEM =       "\tDNS Address:\t";
static const char menu_ipinfo_ntp_string[] PROGMEM =       "\tNTP Address:\t";
static const char menu_ipinfo_gmt_string[] PROGMEM =       "\tGMT Offset:\t";
static const char menu_ipinfo_hostname_string[] PROGMEM =  "\tHostname:\t";


static inline void print_ipinfo(void)
{
    char tmp[4];
    serial_put_string_P(menu_ipinfo_dhcp_string);
    serial_put_string_P(eeprom_read_byte(SETTING_DCHP) ? menu_ipinfo_dhcp_string_yes : menu_ipinfo_dhcp_string_no);
    
    serial_put_string_P(menu_ipinfo_ip_string);
    print_addr(SETTING_IP_ADDR, '.', 4, 10, tmp);
    
    serial_put_string_P(menu_ipinfo_mac_string);
    print_addr(SETTING_MAC_ADDR, ':', 6, 16, tmp);
    
    serial_put_string_P(menu_ipinfo_router_string);
    print_addr(SETTING_ROUTER_ADDR, '.', 4, 10, tmp);
    
    serial_put_string_P(menu_ipinfo_netmask_string);
    print_addr(SETTING_NETMASK, '.', 4, 10, tmp);
    
    serial_put_string_P(menu_ipinfo_dns_string);
    print_addr(SETTING_DNS_ADDR, '.', 4, 10, tmp);
    
    serial_put_string_P(menu_ipinfo_ntp_string);
    print_addr(SETTING_NTP_ADDR, '.', 4, 10, tmp);
    
    serial_put_string_P(menu_ipinfo_gmt_string);
    if (eeprom_read_byte(SETTING_GMT_OFFSET) >= 0) {
        serial_put_byte('+');
    }
    itoa(eeprom_read_byte(SETTING_GMT_OFFSET), tmp, 10);
    serial_put_string(tmp);
    serial_put_byte('\n');
    
    serial_put_string_P(menu_ipinfo_hostname_string);
    serial_put_from_eeprom(SETTING_HOSTNAME);
    serial_put_byte('\n');
}

static const char menu_targetinfo_addr_string[] PROGMEM = "\tAddress:\t";
static const char menu_targetinfo_port_string[] PROGMEM = "\tPort:\t\t";

static inline void print_targetinfo(void)
{
    char tmp[6];
    serial_put_string_P(menu_targetinfo_addr_string);
    print_addr(SETTING_TARGET_IP, '.', 4, 10, tmp);
    
    serial_put_string_P(menu_targetinfo_port_string);
    utoa(eeprom_read_word(SETTING_TARGET_PORT), tmp, 10);
    serial_put_string(tmp);
    serial_put_byte('\n');
}

static const char menu_payload_t1r_string[] PROGMEM = "\tTrigger one, rising edge:\n\t\t";
static const char menu_payload_t1f_string[] PROGMEM = "\tTrigger one, falling edge:\n\t\t";
static const char menu_payload_t2f_string[] PROGMEM = "\tTrigger two, rising edge:\n\t\t";
static const char menu_payload_t2r_string[] PROGMEM = "\tTrigger two, falling edge:\n\t\t";

static inline void print_payloads(void)
{
    switch (menu_state) {
        case 0:
            serial_put_string_P(menu_payload_t1r_string);
            serial_put_from_eeprom(SETTING_T_ONE_RISE);
            serial_put_byte('\n');
            menu_state++;
            break;
        case 1:
            serial_put_string_P(menu_payload_t1f_string);
            serial_put_from_eeprom(SETTING_T_ONE_FALL);
            serial_put_byte('\n');
            menu_state++;
            break;
        case 2:
            serial_put_string_P(menu_payload_t2f_string);
            serial_put_from_eeprom(SETTING_T_TWO_RISE);
            serial_put_byte('\n');
            menu_state++;
            break;
        case 3:
            serial_put_string_P(menu_payload_t2r_string);
            serial_put_from_eeprom(SETTING_T_TWO_FALL);
            serial_put_byte('\n');
            menu_state = 0;
            menu_status = NONE;
            print_prompt();
            break;
    }
}

static inline void print_dhcp(void)
{
    char tmp[4];
    serial_put_string_P(menu_ipinfo_ip_string);
    //print_addr(network_get_ip_addr(), '.', 4, 10, tmp);
    print_addr(0, '.', 4, 10, tmp);
    
    serial_put_string_P(menu_ipinfo_router_string);
    //print_addr(network_get_router_addr(), '.', 4, 10, tmp);
    print_addr(0, '.', 4, 10, tmp);
    
    serial_put_string_P(menu_ipinfo_netmask_string);
    //print_addr(network_get_netmask(), '.', 4, 10, tmp);
    print_addr(0, '.', 4, 10, tmp);
}

static const char menu_set_key_i_ip[] PROGMEM =       " ip.ip";
static const char menu_set_key_i_dhcp[] PROGMEM =     " ip.dhcp";
static const char menu_set_key_i_mac[] PROGMEM =      " ip.mac";
static const char menu_set_key_i_router[] PROGMEM =   " ip.router";
static const char menu_set_key_i_netmask[] PROGMEM =  " ip.netmask";
static const char menu_set_key_i_dns[] PROGMEM =      " ip.dns";
static const char menu_set_key_i_ntp[] PROGMEM =      " ip.ntp";
static const char menu_set_key_i_gmt[] PROGMEM =      " ip.gmt";
static const char menu_set_key_i_hostname[] PROGMEM = " ip.hostname";

static const char menu_set_key_t_ip[] PROGMEM =    " target.ip";
static const char menu_set_key_t_port[] PROGMEM =  " target.port";

static const char menu_set_key_p_1r[] PROGMEM = " payload.onerise";
static const char menu_set_key_p_1f[] PROGMEM = " payload.onefall";
static const char menu_set_key_p_2r[] PROGMEM = " payload.tworise";
static const char menu_set_key_p_2f[] PROGMEM = " payload.twofall";

static inline void interpret_key (char *key, uint16_t *address, uint8_t *length, const char **prompt)
{
    if (!strcasecmp_P(key, menu_set_key_i_ip)) {
        *address = SETTING_IP_ADDR;
        *length = 4;
        *prompt = set_ip_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_i_dhcp)) {
        *address = SETTING_DCHP;
        *length = 1;
        *prompt = set_dhcp_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_i_mac)) {
        *address = SETTING_MAC_ADDR;
        *length = 6;
        *prompt = set_mac_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_i_router)) {
        *address = SETTING_ROUTER_ADDR;
        *length = 4;
        *prompt = set_ip_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_i_netmask)) {
        *address = SETTING_NETMASK;
        *length = 4;
        *prompt = set_ip_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_i_dns)) {
        *address = SETTING_DNS_ADDR;
        *length = 4;
        *prompt = set_ip_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_i_ntp)) {
        *address = SETTING_NTP_ADDR;
        *length = 4;
        *prompt = set_ip_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_i_gmt)) {
        *address = SETTING_GMT_OFFSET;
        *length = 1;
        *prompt = set_gmt_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_i_hostname)) {
        *address = SETTING_HOSTNAME;
        *length = 32;
        *prompt = set_ip_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_t_ip)) {
        *address = SETTING_TARGET_IP;
        *length = 4;
        *prompt = set_ip_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_t_port)) {
        *address = SETTING_TARGET_PORT;
        *length = 2;
        *prompt = set_port_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_p_1r)) {
        *address = SETTING_T_ONE_RISE;
        *length = 200;
        *prompt = set_payload_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_p_1f)) {
        *address = SETTING_T_ONE_FALL;
        *length = 200;
        *prompt = set_payload_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_p_2r)) {
        *address = SETTING_T_TWO_RISE;
        *length = 200;
        *prompt = set_payload_prompt_string;
    } else if (!strcasecmp_P(key, menu_set_key_p_2f)) {
        *address = SETTING_T_TWO_FALL;
        *length = 200;
        *prompt = set_payload_prompt_string;
    } else {
        *address = 0;
        *length = 0;
        *prompt = NULL;
    }
}

static inline void parse_value(char* str, uint16_t address, uint8_t length) {
    char *next;
    
    switch (length) {
        case 1:
            // gmt offset or dchp flag
            eeprom_write_byte(address, (uint8_t)atoi(str));
            break;
        case 2:
            // port
            eeprom_write_word(address, (uint16_t)atoi(str));
            break;
        case 4:
            // ip addr
            next = str + strlen(str);
            eeprom_write_byte(address, strtol(str, &next, 10));
            eeprom_write_byte(address + 1, strtol(next + 1, &next, 10));
            eeprom_write_byte(address + 2, strtol(next + 1, &next, 10));
            eeprom_write_byte(address + 3, strtol(next + 1, &next, 10));
            break;
        case 6:
            // mac addr
            next = str + strlen(str);
            eeprom_write_byte(address, strtol(str, &next, 16));
            eeprom_write_byte(address + 1, strtol(next + 1, &next, 16));
            eeprom_write_byte(address + 2, strtol(next + 1, &next, 16));
            eeprom_write_byte(address + 3, strtol(next + 1, &next, 16));
            eeprom_write_byte(address + 4, strtol(next + 1, &next, 16));
            eeprom_write_byte(address + 5, strtol(next + 1, &next, 16));
            break;
        default:
            eeprom_update_block(str, address, strlen(str) + 1);
            // string
            break;
    }
}

static const char menu_set_help_key[] PROGMEM =         " help";
static const char menu_set_help_key_ip_2[] PROGMEM =    " help ip 2";
static const char menu_set_help_key_ip[] PROGMEM =      " help ip";
static const char menu_set_help_key_target[] PROGMEM =  " help target";
static const char menu_set_help_key_payload[] PROGMEM = " help payload";

static inline void process_set(char* property)
{
    if ((strlen(property) == 0) || !strcasecmp_P(property, menu_set_help_key)) {
        // Empty string or " help"
        serial_put_string_P(set_help_string);
        menu_status = NONE;
    } else if (!strncasecmp_P(property, menu_set_help_key_ip_2, 10)) {
        serial_put_string_P(set_help_ip_2_string);
        menu_status = NONE;
    } else if (!strncasecmp_P(property, menu_set_help_key_ip, 8)) {
        serial_put_string_P(set_help_ip_1_string);
        menu_status = NONE;
    } else if (!strncasecmp_P(property, menu_set_help_key_target, 12)) {
        serial_put_string_P(set_help_target_string);
        menu_status = NONE;
    } else if (!strncasecmp_P(property, menu_set_help_key_payload, 13)) {
        serial_put_string_P(set_help_payload_string);
        menu_status = NONE;
    } else if (property != NULL) {
        const char *prompt;
        interpret_key(property, (uint16_t*)&menu_state + 1, (uint8_t*)&menu_state, &prompt);
        if (prompt != NULL) {
            serial_put_string_P(prompt);
        } else {
            serial_put_string_P(set_unkown_property_string);
            serial_put_string(property);
            serial_put_byte('\n');
            menu_status = NONE;
        }
    } else {
        if (serial_has_line()) {
            serial_get_line(menu_buffer, 200);
    
            parse_value(menu_buffer, *((uint16_t*)&menu_state + 1), *((uint8_t*)&menu_state));
            
            print_prompt();
            menu_status = NONE;
        }
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


