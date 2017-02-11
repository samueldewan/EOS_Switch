/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * This file is part of avr-libethernet. For latest information and software updates,    *
 * see http://www.sourceforge.net/p/avrlibethernet. If you have wishes, improvements,    *
 * changes, bugfixes or suggestions regarding this file or any other of avr-libethernet, *
 * the copyright holders (see below) would be most welcome if you share them on the      *
 * project's website for further improvement of this project.                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2012 by Niklas Fritz, Alexander Gladis                                  *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef LIBETHERNET_DHCP_H__
#define LIBETHERNET_DHCP_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#ifdef IMPLEMENT_DHCP
/**
 * Performs a DHCP request and waits until a valid configuration has been received or the request timed out
 * @remark The given pointers will only be modified if true was returned!
 * @param Hostname Our hostname. Can be NULL for no hostname
 * @param Timeout The timeout (in milliseconds) until the request is aborted. Can be 0 so the request will never time out
 * @param IP Ignored if NULL, otherwise, the retrieved IP will be stored there
 * @param NetMask Ignored if NULL, otherwise, the retrieved net mask will be stored there
 * @param RouterIP Ignored if NULL, otherwise, the retrieved router IP will be stored there
 * @param DNSServerIP Ignored if NULL, otherwise, the retrieved DNS server IP will be stored there
 * @param NTPServerIP Ignored if NULL, otherwise, the retrieved NTP server IP will be stored there
 * @return True if we have a valid configuration, False otherwise (or on timeout)
 */
bool dhcp_request(const char* Hostname, uint16_t Timeout, uint32_t* IP, uint32_t* NetMask, uint32_t* RouterIP, uint32_t* DNSServerIP, uint32_t* NTPServerIP);

/**
 * Checks if we are currently requesting a DHCP configuration
 * @return True if we are, False otherwise
 */
bool dhcp_is_requesting(void);

/**
 * Releases the current DHCP configuration (informing the DHCP server that our IP can be reused)
 * @remark After calling this function, you must not use any previously retrieved DHCP configuration!
 */
void dhcp_release(void);

/**
 * Informs the DHCP module that one second has elapsed
 * @remark Do not call this function, ethernet_second_tick does it for you
 */
void dhcp_second_tick(void);

/**
 * Checks if the DHCP module has a valid configuration
 * @return True if valid, False otherwise
 */
bool dhcp_has_valid_configuration(void);

/**
 * Returns the Hostname currently used by the DHCP module
 * @return see above
 */
const char* dhcp_get_hostname(void);

/**
 * Returns the IP address currently assigned to this DHCP module
 * @return see above
 */
uint32_t dhcp_get_ip(void);

/**
 * Returns the subnet mask currently assigned to this DHCP module
 * @return see above
 */
uint32_t dhcp_get_netmask(void);

/**
 * Returns the router IP address currently assigned to this DHCP module
 * @return see above
 */
uint32_t dhcp_get_router_ip(void);

/**
 * Returns the DNS server IP address currently assigned to this DHCP module
 * @return see above
 */
uint32_t dhcp_get_dns_server_ip(void);

/**
 * Returns the NTP server IP address currently assigned to this DHCP module
 * @return see above
 */
uint32_t dhcp_get_ntp_server_ip(void);

#endif //IMPLEMENT_DHCP

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //LIBETHERNET_DHCP_H__