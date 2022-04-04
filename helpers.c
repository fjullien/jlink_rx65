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
#include <stdlib.h>

#include "helpers.h"

char *buf_to_hex_str(const void *_buf, unsigned buf_len)
{
	unsigned len_bytes = DIV_ROUND_UP(buf_len, 8);
	char *str = calloc(len_bytes * 2 + 1, 1);

	const uint8_t *buf = _buf;
	for (unsigned i = 0; i < len_bytes; i++) {
		uint8_t tmp = buf[len_bytes - i - 1];
		if ((i == 0) && (buf_len % 8))
			tmp &= (0xff >> (8 - (buf_len % 8)));
		str[2 * i] = hex_digits[tmp >> 4];
		str[2 * i + 1] = hex_digits[tmp & 0xf];
	}

	return str;
}