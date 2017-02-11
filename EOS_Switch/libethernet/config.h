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

#ifndef LIBETHERNET_CONFIG_H__
#define LIBETHERNET_CONFIG_H__

/**
 * If defined, interrupts will be used to detect incoming packets and link status changes
 * If not defined, the ENC28J60 will continuously be polled for those events
 */
//#define USE_INTERRUPTS

/**
 * If defined, link status changes will be detected via an interrupt and can be handled in a callback
 * @remark This feature takes about 100 bytes in program memory
 */
//#define HANDLE_LINK_STATUS_CHANGES

/// The maximum transmission unit size in bytes (maximum size of a data packet - including headers - that can be sent/received)
#define MTU_SIZE 400

/// Size of the ARP table
#define ARP_TABLE_SIZE 5

/// Time (in seconds) until ARP table entries expire
#define ARP_TABLE_TIMEOUT 30

/**
 * If defined, ICMP will be implemented (recommended!)
 * @remark This feature takes about 250 bytes in program memory
 */
#define IMPLEMENT_ICMP

/// If defined, UDP will be implemented
#define IMPLEMENT_UDP
#ifdef IMPLEMENT_UDP
/// The size of the UDP application table (= how many ports can be listened on at the same time)
#	define UDP_APPLICATION_TABLE_SIZE 1
/// The size of the UDP table (= how many UDP connections can be held at the same time)
#	define UDP_TABLE_SIZE 1
#endif

/// If defined, TCP will be implemented
//#define IMPLEMENT_TCP
#ifdef IMPLEMENT_TCP
/// The size of the TCP application table (= how many ports can be listened on at the same time)
#	define TCP_APPLICATION_TABLE_SIZE 2
/// The size of the TCP table (= how many TCP connections can be held at the same time)
#	define TCP_TABLE_SIZE 2
#endif


/// If defined, DNS name resolving will be implemented
//#define IMPLEMENT_DNS

/// If defined, DHCP will be implemented
#define IMPLEMENT_DHCP

/// If defined, NTP time synchronization will be implemented
//#define IMPLEMENT_NTP

#endif //LIBETHERNET_CONFIG_H__
