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

#endif /* enc28j60_h */
