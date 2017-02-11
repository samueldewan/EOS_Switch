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

#ifndef LIBETHERNET_ENC28J60_H__
#define LIBETHERNET_ENC28J60_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus


/**
 * Initialises the ENC28J60
 * @remark You must not call any other function before this one!
 * @param MACAddr Pointer to a six-byte-array containing the MAC address the ENC28J60 should use
 * @param FullDuplex Specifies whether the ENC28J60 should work in full duplex mode (true) or half duplex mode (false)
 */
void enc28j60_initialise(const uint8_t* MACAddr, bool FullDuplex);

/**
 * Deinitialises the ENC28J60
 * @remark You must not call any other function (except initialise) after this one!
 */
void enc28j60_deinitialise(void);

/**
 * Resets and reinitialises the ENC28J60
 */
void enc28j60_reset(void);

/**
 * Retrieves the chip's revision ID
 * @return The RevisionID
 */
uint8_t enc28j60_get_revision_id(void);

/**
 * Retrieves the MAC Address the chip uses currently
 * @return A six-byte-array containing the MAC address
 */
const uint8_t* enc28j60_get_mac_address(void);

/**
 * Returns the current link status
 * @return True if link is up
 */
bool enc28j60_get_link_status(void);

/**
 * Enables interrupts coming from the ENC
 */
void enc28j60_enable_interrupts(void);

/**
 * Disables interrupts coming from the ENC
 */
void enc28j60_disable_interrupts(void);

/**
 * Checks the controller's interrupt register for the PKTIF flag
 * @return True if PKTIF is set
 */
bool enc28j60_has_packet_interrupt(void);

#ifdef HANDLE_LINK_STATUS_CHANGES
/**
 * Checks the controller's interrupt register for the LINKIF flag
 * @remark The LINKIF flag will be cleared if it was set
 * @return True if LINKIF is set
 */
bool enc28j60_has_link_status_interrupt(void);
#endif

/**
 * Sends data from a buffer
 * @remark Waits up to 100ms for the previous transmission to finish. If it does not finish within that time frame, it will be terminated!
 * @param Buffer The data to be sent
 * @param Length The length of Buffer
 */
void enc28j60_send(const uint8_t* Buffer, size_t Length);

/**
 * Tries to receive data to a buffer
 * @param Buffer The buffer that will take the data
 * @param BufferSize The length of Buffer
 * @return The number of bytes received and written into Buffer. 0 if no data was received.
 */
size_t enc28j60_receive(uint8_t* Buffer, size_t BufferSize);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //LIBETHERNET_ENC28J60_H__