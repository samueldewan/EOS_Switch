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

#include <util/delay.h>

#include "global.h"
#include "spi.h"
#include "enc28j60.h"

// -----------------------------------------------------------------------------------------------
// -------------------------------------------- Macros -------------------------------------------
// -----------------------------------------------------------------------------------------------
#define LO(x) ( (uint8_t) ((x) & 0xFF) )
#define HI(x) ( (uint8_t) (((x) >> 8) & 0xFF) )
#define MAKE_WORD(hi,lo) ( (((uint16_t)hi) << 8) | lo )
#define MAKE_DWORD(b1,b2,b3,b4) ( (((unsigned long)b1) << 24) | (((unsigned long)b2) << 16) | (((unsigned long)b3) << 8) | b4 )

// -----------------------------------------------------------------------------------------------
// ------------------------------------------ Constants ------------------------------------------
// -----------------------------------------------------------------------------------------------
#define ENC28J60_RX_BUFFER_START 0x0000
#define ENC28J60_RX_BUFFER_END 0x19FF
#define ENC28J60_TX_BUFFER_START 0x1A00
#define ENC28J60_TX_BUFFER_END 0x1FFF
#define ENC28J60_MAX_FRAMELENGTH 1518

// Register masks
#define ENC28J60_ADDR_MASK 0x1F
#define ENC28J60_BANK_MASK 0x60
#define ENC28J60_SPRD_MASK 0x80

// All-bank registers
#define ENC28J60_EIE 0x1B
#define ENC28J60_EIR 0x1C
#define ENC28J60_ESTAT 0x1D
#define ENC28J60_ECON2 0x1E
#define ENC28J60_ECON1 0x1F

// Bank 0 registers
#define ENC28J60_ERDPTL (0x00|0x00)
#define ENC28J60_ERDPTH (0x01|0x00)
#define ENC28J60_EWRPTL (0x02|0x00)
#define ENC28J60_EWRPTH (0x03|0x00)
#define ENC28J60_ETXSTL (0x04|0x00)
#define ENC28J60_ETXSTH (0x05|0x00)
#define ENC28J60_ETXNDL (0x06|0x00)
#define ENC28J60_ETXNDH (0x07|0x00)
#define ENC28J60_ERXSTL (0x08|0x00)
#define ENC28J60_ERXSTH (0x09|0x00)
#define ENC28J60_ERXNDL (0x0A|0x00)
#define ENC28J60_ERXNDH (0x0B|0x00)
#define ENC28J60_ERXRDPTL (0x0C|0x00)
#define ENC28J60_ERXRDPTH (0x0D|0x00)
#define ENC28J60_ERXWRPTL (0x0E|0x00)
#define ENC28J60_ERXWRPTH (0x0F|0x00)
#define ENC28J60_EDMASTL (0x10|0x00)
#define ENC28J60_EDMASTH (0x11|0x00)
#define ENC28J60_EDMANDL (0x12|0x00)
#define ENC28J60_EDMANDH (0x13|0x00)
#define ENC28J60_EDMADSTL (0x14|0x00)
#define ENC28J60_EDMADSTH (0x15|0x00)
#define ENC28J60_EDMACSL (0x16|0x00)
#define ENC28J60_EDMACSH (0x17|0x00)

// Bank 1 registers
#define ENC28J60_EHT0 (0x00|0x20)
#define ENC28J60_EHT1 (0x01|0x20)
#define ENC28J60_EHT2 (0x02|0x20)
#define ENC28J60_EHT3 (0x03|0x20)
#define ENC28J60_EHT4 (0x04|0x20)
#define ENC28J60_EHT5 (0x05|0x20)
#define ENC28J60_EHT6 (0x06|0x20)
#define ENC28J60_EHT7 (0x07|0x20)
#define ENC28J60_EPMM0 (0x08|0x20)
#define ENC28J60_EPMM1 (0x09|0x20)
#define ENC28J60_EPMM2 (0x0A|0x20)
#define ENC28J60_EPMM3 (0x0B|0x20)
#define ENC28J60_EPMM4 (0x0C|0x20)
#define ENC28J60_EPMM5 (0x0D|0x20)
#define ENC28J60_EPMM6 (0x0E|0x20)
#define ENC28J60_EPMM7 (0x0F|0x20)
#define ENC28J60_EPMCSL (0x10|0x20)
#define ENC28J60_EPMCSH (0x11|0x20)
#define ENC28J60_EPMOL (0x14|0x20)
#define ENC28J60_EPMOH (0x15|0x20)
#define ENC28J60_EWOLIE (0x16|0x20)
#define ENC28J60_EWOLIR (0x17|0x20)
#define ENC28J60_ERXFCON (0x18|0x20)
#define ENC28J60_EPKTCNT (0x19|0x20)

// Bank 2 registers
#define ENC28J60_MACON1 (0x00|0x40|0x80)
#define ENC28J60_MACON2 (0x01|0x40|0x80)
#define ENC28J60_MACON3 (0x02|0x40|0x80)
#define ENC28J60_MACON4 (0x03|0x40|0x80)
#define ENC28J60_MABBIPG (0x04|0x40|0x80)
#define ENC28J60_MAIPGL (0x06|0x40|0x80)
#define ENC28J60_MAIPGH (0x07|0x40|0x80)
#define ENC28J60_MACLCON1 (0x08|0x40|0x80)
#define ENC28J60_MACLCON2 (0x09|0x40|0x80)
#define ENC28J60_MAMXFLL (0x0A|0x40|0x80)
#define ENC28J60_MAMXFLH (0x0B|0x40|0x80)
#define ENC28J60_MAPHSUP (0x0D|0x40|0x80)
#define ENC28J60_MICON (0x11|0x40|0x80)
#define ENC28J60_MICMD (0x12|0x40|0x80)
#define ENC28J60_MIREGADR (0x14|0x40|0x80)
#define ENC28J60_MIWRL (0x16|0x40|0x80)
#define ENC28J60_MIWRH (0x17|0x40|0x80)
#define ENC28J60_MIRDL (0x18|0x40|0x80)
#define ENC28J60_MIRDH (0x19|0x40|0x80)

// Bank 3 registers
#define ENC28J60_MAADR1 (0x00|0x60|0x80)
#define ENC28J60_MAADR0 (0x01|0x60|0x80)
#define ENC28J60_MAADR3 (0x02|0x60|0x80)
#define ENC28J60_MAADR2 (0x03|0x60|0x80)
#define ENC28J60_MAADR5 (0x04|0x60|0x80)
#define ENC28J60_MAADR4 (0x05|0x60|0x80)
#define ENC28J60_EBSTSD (0x06|0x60)
#define ENC28J60_EBSTCON (0x07|0x60)
#define ENC28J60_EBSTCSL (0x08|0x60)
#define ENC28J60_EBSTCSH (0x09|0x60)
#define ENC28J60_MISTAT (0x0A|0x60|0x80)
#define ENC28J60_EREVID (0x12|0x60)
#define ENC28J60_ECOCON (0x15|0x60)
#define ENC28J60_EFLOCON (0x17|0x60)
#define ENC28J60_EPAUSL (0x18|0x60)
#define ENC28J60_EPAUSH (0x19|0x60)

// PHY registers
#define ENC28J60_PHCON1 0x00
#define ENC28J60_PHSTAT1 0x01
#define ENC28J60_PHHID1 0x02
#define ENC28J60_PHHID2 0x03
#define ENC28J60_PHCON2 0x10
#define ENC28J60_PHSTAT2 0x11
#define ENC28J60_PHIE 0x12
#define ENC28J60_PHIR 0x13
#define ENC28J60_PHLCON 0x14

// EIE bits
#define ENC28J60_EIE_INTIE 0x80
#define ENC28J60_EIE_PKTIE 0x40
#define ENC28J60_EIE_DMAIE 0x20
#define ENC28J60_EIE_LINKIE 0x10
#define ENC28J60_EIE_TXIE 0x08
#define ENC28J60_EIE_WOLIE 0x04
#define ENC28J60_EIE_TXERIE 0x02
#define ENC28J60_EIE_RXERIE 0x01

// EIR bits
#define ENC28J60_EIR_PKTIF 0x40
#define ENC28J60_EIR_DMAIF 0x20
#define ENC28J60_EIR_LINKIF 0x10
#define ENC28J60_EIR_TXIF 0x08
#define ENC28J60_EIR_WOLIF 0x04
#define ENC28J60_EIR_TXERIF 0x02
#define ENC28J60_EIR_RXERIF 0x01

// ESTAT bits
#define ENC28J60_ESTAT_INT 0x80
#define ENC28J60_ESTAT_LATECOL 0x10
#define ENC28J60_ESTAT_RXBUSY 0x04
#define ENC28J60_ESTAT_TXABRT 0x02
#define ENC28J60_ESTAT_CLKRDY 0x01

// ECON2 bits
#define ENC28J60_ECON2_AUTOINC 0x80
#define ENC28J60_ECON2_PKTDEC 0x40
#define ENC28J60_ECON2_PWRSV 0x20
#define ENC28J60_ECON2_VRPS 0x08

// ECON1 bits
#define ENC28J60_ECON1_TXRST 0x80
#define ENC28J60_ECON1_RXRST 0x40
#define ENC28J60_ECON1_DMAST 0x20
#define ENC28J60_ECON1_CSUMEN 0x10
#define ENC28J60_ECON1_TXRTS 0x08
#define ENC28J60_ECON1_RXEN 0x04
#define ENC28J60_ECON1_BSEL1 0x02
#define ENC28J60_ECON1_BSEL0 0x01

// MACON1 bits
#define ENC28J60_MACON1_LOOPBK 0x10
#define ENC28J60_MACON1_TXPAUS 0x08
#define ENC28J60_MACON1_RXPAUS 0x04
#define ENC28J60_MACON1_PASSALL 0x02
#define ENC28J60_MACON1_MARXEN 0x01

// MACON2 bits
#define ENC28J60_MACON2_MARST 0x80
#define ENC28J60_MACON2_RNDRST 0x40
#define ENC28J60_MACON2_MARXRST 0x08
#define ENC28J60_MACON2_RFUNRST 0x04
#define ENC28J60_MACON2_MATXRST 0x02
#define ENC28J60_MACON2_TFUNRST 0x01

// MACON3 bits
#define ENC28J60_MACON3_PADCFG2 0x80
#define ENC28J60_MACON3_PADCFG1 0x40
#define ENC28J60_MACON3_PADCFG0 0x20
#define ENC28J60_MACON3_TXCRCEN 0x10
#define ENC28J60_MACON3_PHDRLEN 0x08
#define ENC28J60_MACON3_HFRMLEN 0x04
#define ENC28J60_MACON3_FRMLNEN 0x02
#define ENC28J60_MACON3_FULDPX 0x01

// MACON4 bits
#define ENC28J60_MACON4_DEFER 0x40
#define ENC28J60_MACON4_BPEN 0x20
#define ENC28J60_MACON4_NOBKOFF 0x10

// MICMD bits
#define ENC28J60_MICMD_MIISCAN 0x02
#define ENC28J60_MICMD_MIIRD 0x01

// MISTAT bits
#define ENC28J60_MISTAT_NVALID 0x04
#define ENC28J60_MISTAT_SCAN 0x02
#define ENC28J60_MISTAT_BUSY 0x01

// PHCON1 bits
#define ENC28J60_PHCON1_PRST 0x8000
#define ENC28J60_PHCON1_PLOOPBK 0x4000
#define ENC28J60_PHCON1_PPWRSV 0x0800
#define ENC28J60_PHCON1_PDPXMD 0x0100

// PHSTAT1 bits
#define ENC28J60_PHSTAT1_PFDPX 0x1000
#define ENC28J60_PHSTAT1_PHDPX 0x0800
#define ENC28J60_PHSTAT1_LLSTAT 0x0004
#define ENC28J60_PHSTAT1_JBSTAT 0x0002

// PHSTAT2 bits
#define ENC28J60_PHSTAT2_TXSTAT 0x4000
#define ENC28J60_PHSTAT2_RXSTAT 0x2000
#define ENC28J60_PHSTAT2_COLSTAT 0x1000
#define ENC28J60_PHSTAT2_LSTAT 0x0400
#define ENC28J60_PHSTAT2_DPXSTAT 0x0200
#define ENC28J60_PHSTAT2_PLRITY 0x0020

// PHCON2 bits
#define ENC28J60_PHCON2_FRCLINK 0x4000
#define ENC28J60_PHCON2_TXDIS 0x2000
#define ENC28J60_PHCON2_JABBER 0x0400
#define ENC28J60_PHCON2_HDLDIS 0x0100

// PHIE bits
#define ENC28J60_PHIE_PGEIE 0x0002
#define ENC28J60_PHIE_PLNKIE 0x0010

// PHIR bits
#define ENC28J60_PHIR_PGIF 0x0004
#define ENC28J60_PHIR_PLNKIF 0x0010

// Operation codes
#define ENC28J60_READ_CTRL_REG 0x00
#define ENC28J60_READ_BUF_MEM 0x3A
#define ENC28J60_WRITE_CTRL_REG 0x40
#define ENC28J60_WRITE_BUF_MEM 0x7A
#define ENC28J60_BIT_FIELD_SET 0x80
#define ENC28J60_BIT_FIELD_CLR 0xA0
#define ENC28J60_SOFT_RESET 0xFF

// -----------------------------------------------------------------------------------------------
// -------------------------------------- Global Variables ---------------------------------------
// -----------------------------------------------------------------------------------------------
// Internal variables
static uint8_t enc28j60_MACAddress[6];
static uint8_t enc28j60_RevisionID;
static bool enc28j60_FullDuplex;
static uint8_t enc28j60_CurrentBank;
static uint16_t enc28j60_NextPacketPtr;


// -----------------------------------------------------------------------------------------------
// ----------------------------- Internal Function Implementations -------------------------------
// -----------------------------------------------------------------------------------------------
/**
 * Performs a read operation
 * @remark Only for internal use!
 * @param Opcode The operation code
 * @param Address The destined register's address
 * @return The data read from the register
 */
uint8_t _enc28j60_read_op(uint8_t Opcode, uint8_t Address)
{
	spi_select(true);
	
	// issue read command
	spi_transfer(Opcode | (Address & ENC28J60_ADDR_MASK));

	// read data
	uint8_t res = spi_transfer(0);

	// do dummy read if needed
	if(Address & 0x80)
		res = spi_transfer(0);
	
	spi_select(false);
	return res;
}

/**
 * Performs a write operation
 * @remark Only for internal use!
 * @param Opcode The operation code
 * @param Address The destined register's address
 * @param Data The data to write
 */
void _enc28j60_write_op(uint8_t Opcode, uint8_t Address, uint8_t Data)
{
	spi_select(true);
	
	// issue write command
	spi_transfer(Opcode | (Address & ENC28J60_ADDR_MASK));

	// write data
	spi_transfer(Data);

	spi_select(false);
}

/**
 * Sets a bit mask in a register
 * @remark Only for internal use!
 * @param Address The register's address
 * @param Bits The bits to be set
 */
void _enc28j60_set_bits(uint8_t Address,uint8_t Bits)
{
	_enc28j60_write_op(ENC28J60_BIT_FIELD_SET,Address,Bits);
}

/**
 * Clears a bit mask in a register
 * @remark Only for internal use!
 * @param Address The register's address
 * @param Bits The bits to be cleared
 */
void _enc28j60_clr_bits(uint8_t Address,uint8_t Bits)
{
	_enc28j60_write_op(ENC28J60_BIT_FIELD_CLR,Address,Bits);
}

/**
 * Adjusts the current register bank to match the register we're about to use
 * @remark Only for internal use!
 * @param Address The register's address
 */
void _enc28j60_set_bank(uint8_t Address)
{
	Address &= ENC28J60_BANK_MASK;
	if(Address != enc28j60_CurrentBank){
		// Set the bank
		_enc28j60_clr_bits(ENC28J60_ECON1,ENC28J60_ECON1_BSEL1|ENC28J60_ECON1_BSEL0);
		_enc28j60_set_bits(ENC28J60_ECON1,Address >> 5);
		enc28j60_CurrentBank = Address;
	}
}

/**
 * Reads from a register
 * @remark Only for internal use!
 * @param Address The register's address
 * @return The register's current data
 */
uint8_t _enc28j60_read_reg(uint8_t Address)
{
	_enc28j60_set_bank(Address);
	return _enc28j60_read_op(ENC28J60_READ_CTRL_REG,Address);
}

/**
 * Writes to a register
 * @remark Only for internal use!
 * @param Address The register's address
 * @param Data The data to write
 */
void _enc28j60_write_reg(uint8_t Address,uint8_t Data)
{
	_enc28j60_set_bank(Address);
	_enc28j60_write_op(ENC28J60_WRITE_CTRL_REG,Address,Data);
}

/**
 * Reads from a physical register
 * @remark Only for internal use!
 * @param Address The register's address
 * @return The register's current data
 */
uint16_t _enc28j60_read_phy(uint8_t Address)
{
	// Set the right address and start the register read operation
	_enc28j60_write_reg(ENC28J60_MIREGADR,Address);
	_enc28j60_write_reg(ENC28J60_MICMD,ENC28J60_MICMD_MIIRD);

	// Wait until the PHY read completes
	while(_enc28j60_read_reg(ENC28J60_MISTAT) & ENC28J60_MISTAT_BUSY)
		;

	// Quit reading
	_enc28j60_write_reg(ENC28J60_MICMD,0x00);
	
	// Get the result
	uint8_t low_byte = _enc28j60_read_reg(ENC28J60_MIRDL);
	uint8_t high_byte = _enc28j60_read_reg(ENC28J60_MIRDH);

	return MAKE_WORD(high_byte,low_byte);
}

/**
 * Writes to a physical register
 * @remark Only for internal use!
 * @param Address The register's address
 * @param Data The data to write
 */
void _enc28j60_write_phy(uint8_t Address,uint16_t Data)
{
	// Set the PHY register address
	_enc28j60_write_reg(ENC28J60_MIREGADR,Address);
	
	// Write the PHY data
	_enc28j60_write_reg(ENC28J60_MIWRL,LO(Data));
	_enc28j60_write_reg(ENC28J60_MIWRH,HI(Data));

	// Wait until the PHY write completes
	while(_enc28j60_read_reg(ENC28J60_MISTAT) & ENC28J60_MISTAT_BUSY)
		;
}

/**
 * Reads from the ENC28J60's internal buffer
 * @remark Only for internal use!
 * @param Buffer The buffer the data will be stored in
 * @param Length The length of Buffer
 */
void _enc28j60_read_buf(uint8_t* Buffer,size_t Length)
{
	spi_select(true);
	spi_transfer(ENC28J60_READ_BUF_MEM);
	for(; Length > 0; --Length)
		*Buffer++ = spi_transfer(0x00);
	spi_select(false);
}

/**
 * Writes to the ENC28J60's internal buffer
 * @remark Only for internal use!
 * @param Buffer The buffer to write
 * @param Length The length of Buffer
 */
void _enc28j60_write_buf(const uint8_t* Buffer,size_t Length)
{
	spi_select(true);
	spi_transfer(ENC28J60_WRITE_BUF_MEM);
	for(; Length > 0; --Length)
		spi_transfer(*Buffer++);
	spi_select(false);
}

/**
 * Initialises the ENC28J60
 * @remark Only for internal use!
 */
void _enc28j60_initialise(void)
{
	// Perform a soft reset
	enc28j60_reset();

	// Write MAC address (byte-backward)
	_enc28j60_write_reg(ENC28J60_MAADR5,enc28j60_MACAddress[0]);
	_enc28j60_write_reg(ENC28J60_MAADR4,enc28j60_MACAddress[1]);
	_enc28j60_write_reg(ENC28J60_MAADR3,enc28j60_MACAddress[2]);
	_enc28j60_write_reg(ENC28J60_MAADR2,enc28j60_MACAddress[3]);
	_enc28j60_write_reg(ENC28J60_MAADR1,enc28j60_MACAddress[4]);
	_enc28j60_write_reg(ENC28J60_MAADR0,enc28j60_MACAddress[5]);

	// Get the chip's revision ID
	enc28j60_RevisionID = _enc28j60_read_reg(ENC28J60_EREVID);

	// Initialise the receive buffer
	enc28j60_NextPacketPtr = ENC28J60_RX_BUFFER_START;
	_enc28j60_write_reg(ENC28J60_ERXSTL,LO(ENC28J60_RX_BUFFER_START));
	_enc28j60_write_reg(ENC28J60_ERXSTH,HI(ENC28J60_RX_BUFFER_START));
	_enc28j60_write_reg(ENC28J60_ERXNDL,LO(ENC28J60_RX_BUFFER_END));
	_enc28j60_write_reg(ENC28J60_ERXNDH,HI(ENC28J60_RX_BUFFER_END));

	// Initialise the transmit buffer
	_enc28j60_write_reg(ENC28J60_ETXSTL,LO(ENC28J60_TX_BUFFER_START));
	_enc28j60_write_reg(ENC28J60_ETXSTH,HI(ENC28J60_TX_BUFFER_START));
	_enc28j60_write_reg(ENC28J60_ETXNDL,LO(ENC28J60_TX_BUFFER_END));
	_enc28j60_write_reg(ENC28J60_ETXNDH,HI(ENC28J60_TX_BUFFER_END));

	// Bring MAC out of reset
	_enc28j60_write_reg(ENC28J60_MACON2,0x00);

	// Enable MAC receive
	if(enc28j60_FullDuplex)
		_enc28j60_write_reg(ENC28J60_MACON1,ENC28J60_MACON1_MARXEN|ENC28J60_MACON1_TXPAUS|ENC28J60_MACON1_RXPAUS);
	else
		_enc28j60_write_reg(ENC28J60_MACON1,ENC28J60_MACON1_MARXEN);

	// Enable automatic padding and CRC operations
	if(enc28j60_FullDuplex)
		_enc28j60_set_bits(ENC28J60_MACON3,ENC28J60_MACON3_PADCFG0|ENC28J60_MACON3_TXCRCEN|ENC28J60_MACON3_FRMLNEN|ENC28J60_MACON3_FULDPX);
	else
		_enc28j60_set_bits(ENC28J60_MACON3,ENC28J60_MACON3_PADCFG0|ENC28J60_MACON3_TXCRCEN|ENC28J60_MACON3_FRMLNEN);

	// Set the maximum packet size which the controller will accept
	_enc28j60_write_reg(ENC28J60_MAMXFLL,LO(ENC28J60_MAX_FRAMELENGTH));	
	_enc28j60_write_reg(ENC28J60_MAMXFLH,HI(ENC28J60_MAX_FRAMELENGTH));

	// Sets MACON4 IEEE 802.3 conformance
	_enc28j60_set_bits(ENC28J60_MACON4,ENC28J60_MACON4_DEFER);

	// Set inter-frame gap (back-to-back)
	_enc28j60_write_reg(ENC28J60_MABBIPG,enc28j60_FullDuplex ? 0x15 : 0x12);

	// Set inter-frame gap (non-back-to-back)
	_enc28j60_write_reg(ENC28J60_MAIPGL,0x12);
	if(!enc28j60_FullDuplex)
		_enc28j60_write_reg(ENC28J60_MAIPGH,0x0C);

	// Disable clock-out pin
	_enc28j60_write_reg(ENC28J60_ECOCON,0x00);

	if(enc28j60_FullDuplex)
		_enc28j60_write_phy(ENC28J60_PHCON1,ENC28J60_PHCON1_PDPXMD);
	else
		_enc28j60_write_phy(ENC28J60_PHCON2,ENC28J60_PHCON2_HDLDIS);

	// Switch to bank 0
	_enc28j60_set_bank(ENC28J60_ECON1);

	// Enable interrupts
#ifdef USE_INTERRUPTS
#ifdef HANDLE_LINK_STATUS_CHANGES
	_enc28j60_set_bits(ENC28J60_EIE,ENC28J60_EIE_INTIE|ENC28J60_EIE_PKTIE|ENC28J60_EIE_LINKIE);
	_enc28j60_write_phy(ENC28J60_PHIE,ENC28J60_PHIE_PGEIE|ENC28J60_PHIE_PLNKIE);
#else
	_enc28j60_set_bits(ENC28J60_EIE,ENC28J60_EIE_INTIE|ENC28J60_EIE_PKTIE);
#endif //HANDLE_LINK_STATUS_CHANGES
#endif //USE_INTERRUPTS

	// Disable all filters
	_enc28j60_write_reg(ENC28J60_ERXFCON,0x00);

	// Enable packet reception
	_enc28j60_set_bits(ENC28J60_ECON1,ENC28J60_ECON1_RXEN);
}

// -----------------------------------------------------------------------------------------------
// ----------------------------- External Function Implementations -------------------------------
// -----------------------------------------------------------------------------------------------
void enc28j60_initialise(const uint8_t* MACAddr, bool FullDuplex)
{
	// Store configuration
	for(uint8_t i = 0; i < sizeof(enc28j60_MACAddress); ++i)
		enc28j60_MACAddress[i] = MACAddr[i];

	enc28j60_FullDuplex = FullDuplex;
	enc28j60_CurrentBank = 0;

	// Initialise the ENC28J60
	_enc28j60_initialise();
}

void enc28j60_deinitialise(void)
{
	// TODO
}

void enc28j60_reset(void)
{
	// Send the reset command
	spi_select(true);
	spi_transfer(ENC28J60_SOFT_RESET);
	spi_select(false);

	_delay_ms(1);

	// Wait for the ENC28J60 to finish startup
	while(!(_enc28j60_read_reg(ENC28J60_ESTAT) & ENC28J60_ESTAT_CLKRDY))
		;
}

uint8_t enc28j60_get_revision_id(void)
{
	return enc28j60_RevisionID;
}

const uint8_t* enc28j60_get_mac_address(void)
{
	return enc28j60_MACAddress;
}

bool enc28j60_get_link_status(void)
{
	return (_enc28j60_read_phy(ENC28J60_PHSTAT2) & ENC28J60_PHSTAT2_LSTAT) == ENC28J60_PHSTAT2_LSTAT;
}

void enc28j60_enable_interrupts(void)
{
	_enc28j60_set_bits(ENC28J60_EIE,ENC28J60_EIE_INTIE);
}

void enc28j60_disable_interrupts(void)
{
	_enc28j60_clr_bits(ENC28J60_EIE,ENC28J60_EIE_INTIE);
}

bool enc28j60_has_packet_interrupt(void)
{
	return (_enc28j60_read_reg(ENC28J60_EIR) & ENC28J60_EIR_PKTIF) == ENC28J60_EIR_PKTIF;
}

#ifdef HANDLE_LINK_STATUS_CHANGES
bool enc28j60_has_link_status_interrupt(void)
{
	return (_enc28j60_read_phy(ENC28J60_PHIR) & ENC28J60_PHIR_PLNKIF) == ENC28J60_PHIR_PLNKIF;
}
#endif

void enc28j60_send(const uint8_t* Buffer, size_t Length)
{
	bool PrevTxFinished = false;

	// Wait up to 100ms for the previous transmission to finish
	for(uint8_t i = 0; i < 500; ++i){
		if(!(_enc28j60_read_reg(ENC28J60_ECON1) & ENC28J60_ECON1_TXRST)){
			PrevTxFinished = true;
			break;
		}
		_delay_ms(1);
	}

	// Full Duplex: reset tx logic if TXRTS is still active
	// Half Duplex: reset tx logic
	if(!enc28j60_FullDuplex || !PrevTxFinished){
		_enc28j60_set_bits(ENC28J60_ECON1,ENC28J60_ECON1_TXRST);
		_enc28j60_clr_bits(ENC28J60_ECON1,ENC28J60_ECON1_TXRST);
	}

	// Set start write ptr
	_enc28j60_write_reg(ENC28J60_EWRPTL,LO(ENC28J60_TX_BUFFER_START));
	_enc28j60_write_reg(ENC28J60_EWRPTH,HI(ENC28J60_TX_BUFFER_START));

	// Set end write ptr
	_enc28j60_write_reg(ENC28J60_ETXNDL,LO(ENC28J60_TX_BUFFER_START+Length));
	_enc28j60_write_reg(ENC28J60_ETXNDH,HI(ENC28J60_TX_BUFFER_START+Length));

	// Write 1 control byte
	uint8_t ctrl = 0;
	_enc28j60_write_buf(&ctrl,1);

	// Write the data
	_enc28j60_write_buf(Buffer,Length);

	// Clear TXIF flag
	_enc28j60_clr_bits(ENC28J60_EIR,ENC28J60_EIR_TXIF);

	// Start transmission and wait for it to start
	_enc28j60_set_bits(ENC28J60_ECON1,ENC28J60_ECON1_TXRTS);
	_delay_ms(1);
}

size_t enc28j60_receive(uint8_t* Buffer, size_t BufferSize)
{
	// Check rx packet count
	uint8_t PacketCount = _enc28j60_read_reg(ENC28J60_EPKTCNT);
	if(PacketCount == 0)
		return 0;

	// Set read ptr
	_enc28j60_write_reg(ENC28J60_ERDPTL,LO(enc28j60_NextPacketPtr));
	_enc28j60_write_reg(ENC28J60_ERDPTH,HI(enc28j60_NextPacketPtr));

	// Read header
	uint8_t rx_header[6];
	_enc28j60_read_buf(rx_header,sizeof(rx_header));
	enc28j60_NextPacketPtr = MAKE_WORD(rx_header[1],rx_header[0]);
	size_t Length = MAKE_WORD(rx_header[3],rx_header[2]);
	size_t Status = MAKE_WORD(rx_header[5],rx_header[4]);

	// Reset the ENC28J60 if anything went wrong
	if(!(Status & 0x0080) || (Status & 0x8000) || (Length > BufferSize))
		_enc28j60_initialise();

	// Skip the checksum (4 bytes) at the end
	Length -= 4;

	// Simply truncate the data if Buffer is too short
	if(Length > BufferSize)
		Length = BufferSize;

	// Read packet data
	_enc28j60_read_buf(Buffer,Length);

	// Adjust the ERXRDPT pointer (free the packet in the rx buffer)
	if(enc28j60_NextPacketPtr-1 > ENC28J60_RX_BUFFER_END || enc28j60_NextPacketPtr-1 < ENC28J60_RX_BUFFER_START){
		_enc28j60_write_reg(ENC28J60_ERXRDPTL,LO(ENC28J60_RX_BUFFER_END));
		_enc28j60_write_reg(ENC28J60_ERXRDPTH,HI(ENC28J60_RX_BUFFER_END));
	} else {
		_enc28j60_write_reg(ENC28J60_ERXRDPTL,LO(enc28j60_NextPacketPtr-1));
		_enc28j60_write_reg(ENC28J60_ERXRDPTH,HI(enc28j60_NextPacketPtr-1));
	}

	// Decrement the rx packet counter (will clear PKTIF if EPKTCNT reaches 0)
	_enc28j60_set_bits(ENC28J60_ECON2,ENC28J60_ECON2_PKTDEC);
	_delay_ms(1);

	return Length;
}
