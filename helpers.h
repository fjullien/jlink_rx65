/***************************************************************************
 *   Copyright (C) 2004, 2005 by Dominic Rath                              *
 *   Dominic.Rath@gmx.de                                                   *
 *                                                                         *
 *   Copyright (C) 2007,2008 Øyvind Harboe                                 *
 *   oyvind.harboe@zylin.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include <stdint.h>
#include <stddef.h>

char *buf_to_hex_str(const void *_buf, unsigned buf_len);

static inline uint32_t buf_get_u32_be(const uint8_t *_buffer, int offset)
{
	const uint8_t *buffer = _buffer;
	return (buffer[offset] << 24) + (buffer[offset + 1] << 16) + (buffer[offset + 2] << 8) + buffer[offset + 3];
}

static inline void buf_set_u32_be(uint8_t *_buffer, int offset, uint32_t val)
{
	uint8_t *buffer = _buffer;
	buffer[offset]     = (val >> 24) & 0xFF;
	buffer[offset + 1] = (val >> 16) & 0xFF;
	buffer[offset + 2] = (val >> 8) & 0xFF;
	buffer[offset + 3] = val & 0xFF;
}

static inline void buf_set_u32(uint8_t *_buffer,
	unsigned first, unsigned num, uint32_t value)
{
	uint8_t *buffer = _buffer;

	if ((num == 32) && (first == 0)) {
		buffer[3] = (value >> 24) & 0xff;
		buffer[2] = (value >> 16) & 0xff;
		buffer[1] = (value >> 8) & 0xff;
		buffer[0] = (value >> 0) & 0xff;
	} else {
		for (unsigned i = first; i < first + num; i++) {
			if (((value >> (i - first)) & 1) == 1)
				buffer[i / 8] |= 1 << (i % 8);
			else
				buffer[i / 8] &= ~(1 << (i % 8));
		}
	}
}

#define DIV_ROUND_UP(m, n)	(((m) + (n) - 1) / (n))

static const char hex_digits[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f'
};

static inline uint32_t be_to_h_u32(const uint8_t *buf)
{
	return (uint32_t)((uint32_t)buf[3] | (uint32_t)buf[2] << 8 | (uint32_t)buf[1] << 16 | (uint32_t)buf[0] << 24);
}

static inline uint32_t be_to_h_u24(const uint8_t *buf)
{
	return (uint32_t)((uint32_t)buf[2] | (uint32_t)buf[1] << 8 | (uint32_t)buf[0] << 16);
}

static inline uint16_t be_to_h_u16(const uint8_t *buf)
{
	return (uint16_t)((uint16_t)buf[1] | (uint16_t)buf[0] << 8);
}

static inline void h_u32_to_le(uint8_t *buf, int val)
{
	buf[3] = (uint8_t) (val >> 24);
	buf[2] = (uint8_t) (val >> 16);
	buf[1] = (uint8_t) (val >> 8);
	buf[0] = (uint8_t) (val >> 0);
}

static inline void h_u32_to_be(uint8_t *buf, int val)
{
	buf[0] = (uint8_t) (val >> 24);
	buf[1] = (uint8_t) (val >> 16);
	buf[2] = (uint8_t) (val >> 8);
	buf[3] = (uint8_t) (val >> 0);
}

static inline void h_u24_to_le(uint8_t *buf, int val)
{
	buf[2] = (uint8_t) (val >> 16);
	buf[1] = (uint8_t) (val >> 8);
	buf[0] = (uint8_t) (val >> 0);
}

static inline void h_u24_to_be(uint8_t *buf, int val)
{
	buf[0] = (uint8_t) (val >> 16);
	buf[1] = (uint8_t) (val >> 8);
	buf[2] = (uint8_t) (val >> 0);
}

static inline void h_u16_to_le(uint8_t *buf, int val)
{
	buf[1] = (uint8_t) (val >> 8);
	buf[0] = (uint8_t) (val >> 0);
}

static inline void h_u16_to_be(uint8_t *buf, int val)
{
	buf[0] = (uint8_t) (val >> 8);
	buf[1] = (uint8_t) (val >> 0);
}

static inline void buf_bswap16(uint8_t *dst, const uint8_t *src, size_t len)
{
	for (size_t n = 0; n < len; n += 2) {
		uint16_t x = be_to_h_u16(src + n);
		h_u16_to_le(dst + n, x);
	}
}