/*
 * enc28j60.h
 * EOS_Switch
 *
 * Created by Ian Dewan on 2017-02-19.
 * Copyright © 2017 Ian Dewan. All rights reserved.
 */

#ifndef enc28j60_h
#define enc28j60_h

/* Defines for the SPI commands of the ENC28J60. */
#define SPI_WCR(a) (0x40 & (a))
#define SPI_RCR(a) (a)
#define SPI_BFS(a) (0x80 & (a))
#define SPI_WBM 0x7A

extern uint32_t debug_num;

/**
 * Start the initialization process of the ENC28J60.
 */
extern void init_enc28j60(void);

/**
 * Return whether the ENC28J60 is ready to transmit packets.
 * @return Whether the chip is ready.
 */
extern int enc28j60_ready(void);

/**
 * Actions to be performed in the main loop.
 */
extern void enc28j60_service(void);

extern enum tmp_enum {
    STATE_UNREADY,
    STATE_CLOCK_STABILIZED,
    STATE_BANK_2,
    STATE_MACON3_SET,
    STATE_MACON4_SET,
    STATE_MABBIPG_SET,
    STATE_MAIPGL_SET,
    STATE_MAIPGH_SET,
    STATE_BANK_3,
    STATE_MADDR_1_SET,
    STATE_MADDR_2_SET,
    STATE_MADDR_3_SET,
    STATE_MADDR_4_SET,
    STATE_MADDR_5_SET,
    STATE_MADDR_6_SET,
    STATE_PACKET_1_WRITTEN,
    STATE_SRC_MAC_WRITTEN,
    STATE_PACKET_2_WRITTEN,
    STATE_DEST_IP_WRITTEN,
    STATE_SRC_PORT_WRITTEN,
    STATE_DEST_PORT_WRITTEN,
    STATE_HEADERS_WRITTEN,
    STATE_BANK_0,
    STATE_EDMASTL_SET,
    STATE_DONE
} enc28j60_state;

extern char debug;

#endif /* enc28j60_h */
