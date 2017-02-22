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
#define SPI_BFC(a) (0xA0 & (a))
#define SPI_WBM 0x7A
#define SPI_NOP 0xFF

/**
 * Start the initialization process of the ENC28J60.
 */
extern void init_enc28j60(void);

#endif /* enc28j60_h */
