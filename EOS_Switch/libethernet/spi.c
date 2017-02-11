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

#include <avr/io.h>
#include <util/delay.h>

#include "global.h"

#include "spi.h"

volatile uint8_t* spi_Port;
uint8_t spi_SS_Mask;

void spi_initialise(volatile uint8_t* Port, volatile uint8_t* DDR, uint8_t SCK, uint8_t MISO, uint8_t MOSI, uint8_t SS)
{
	spi_Port = Port;
	spi_SS_Mask = (1 << SS);

	// Set SCK high
	*Port |= (1 << SCK);

	// Set SCK, MOSI and SS as output (we're the SPI master)
	*DDR |= (1 << SCK) | (1 << MOSI) | (1 << SS);

	// Set MISO as input
	*DDR &= ~(1 << MISO);

	// Enable SPI master mode
	SPCR |= (1 << MSTR);

	// We'll use 2X freq/4 = freq/2 bitrate
	SPSR |= (1 << SPI2X);

	// Finally enable SPI
	SPCR |= (1 << SPE);

	// Wait a bit until everything has settled
	_delay_ms(10);
}

void spi_select(bool Selected)
{
	if(Selected)
		*spi_Port &= ~spi_SS_Mask;
	else
		*spi_Port |= spi_SS_Mask;
}

uint8_t spi_transfer(uint8_t Data)
{
	// Send our data
	SPDR = Data;

	// Wait for the transfer to finish
	while(!(SPSR & (1 << SPIF)))
		;

	// Return the received data
	return SPDR;
}
