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

#ifndef LIBETHERNET_UTILS_H__
#define LIBETHERNET_UTILS_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

/// Gets a 8-bit unsigned integer from a buffer at a given index
#define GET_UINT8(buf,pos) *((uint8_t*)&(buf)[pos])

/// Gets a 16-bit unsigned integer (in host endianess) from a buffer at a given index
#define GET_UINT16(buf,pos) *((uint16_t*)&(buf)[pos])

/// Gets a 32-bit unsigned integer (in host endianess) from a buffer at a given index
#define GET_UINT32(buf,pos) *((uint32_t*)&(buf)[pos])

/// Writes a 8-bit unsigned integer to a buffer at a given index
#define SET_UINT8(buf,pos,val) *((uint8_t*)&(buf)[pos]) = (val);

/// Writes a 16-bit unsigned integer (in host endianess) to a buffer at a given index
#define SET_UINT16(buf,pos,val) *((uint16_t*)&(buf)[pos]) = (val);

/// Writes a 32-bit unsigned integer (in host endianess) to a buffer at a given index
#define SET_UINT32(buf,pos,val) *((uint32_t*)&(buf)[pos]) = (val);

/**
 * Generates a pseudorandom 32-bit number
 * @remark Simply combines two pseudorandom 16-bit numbers from rand()
 */
uint32_t rand32(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //LIBETHERNET_UTILS_H__