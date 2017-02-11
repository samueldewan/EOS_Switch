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

#ifndef LIBETHERNET_SPI_H__
#define LIBETHERNET_SPI_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

/**
 * Initializes the SPI bus
 * @remark This function must be implemented by the user!
 * @param Port The PORT register for the pins
 * @param DDR The DDR register for the pins
 * @param SCK The offset for the SCK pin
 * @param MISO The offset for the MISO pin
 * @param MOSI The offset for the MOSI pin
 * @param SS The offset for the SS pin
 */
extern void spi_initialise(volatile uint8_t* Port, volatile uint8_t* DDR, uint8_t SCK, uint8_t MISO, uint8_t MOSI, uint8_t SS);
    
/**
 * Sets the pin connected to the ENC28J60's SS port
 * @remark This function must be implemented by the user!
 * @param Selected Indicates whether the SS port should be enabled or not
 */
extern void spi_select(bool Selected);

/**
 * Sends one byte via SPI and receives one byte simultaneously.
 * @remark This function must be implemented by the user!
 * @param Data The byte to send
 * @return The byte received
 */
extern uint8_t spi_transfer(uint8_t Data);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //LIBETHERNET_SPI_H__
