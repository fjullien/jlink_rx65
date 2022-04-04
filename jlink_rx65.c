/*
 * Copyright (C) 2021 Franck Jullien <franck.jullien@collshade.fr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This code uses this branch of libjaylink:
 * https://gitlab.zapb.de/libjaylink/libjaylink/-/tree/fine_support
 * There is also a fixed that is needed in this branch:
 * diff --git a/libjaylink/fine.c b/libjaylink/fine.c
 * index 102515f..a745cce 100644
 * --- a/libjaylink/fine.c
 * +++ b/libjaylink/fine.c
 * @@ -81,7 +81,7 @@ JAYLINK_API int jaylink_fine_io(struct jaylink_device_handle *devh,
 *         buffer_set_u32(buf, in_length, 5);
 *         buffer_set_u32(buf, other_param, 9);
 *  
 * -       ret = transport_write(devh, buf, 9);
 * +       ret = transport_write(devh, buf, 13);
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include <libjaylink/libjaylink.h>
#include "helpers.h"
#include "fine.h"

struct jaylink_device_handle *devh;
static struct jaylink_context *ctx;

uint8_t id_code[16] = {
	0x33, 0x22, 0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

int main(void)
{
	int ret;

	init_jlink(JAYLINK_TIF_FINE, ctx);

	ret = fine_get_chip_id();
	if (ret != EXIT_SUCCESS)
		return EXIT_FAILURE;

	ret = fine_init_chip();
	if (ret != EXIT_SUCCESS)
		return EXIT_FAILURE;

	ret = fine_get_device_type();
	if (ret != EXIT_SUCCESS)
		return EXIT_FAILURE;

	ret = fine_set_endianness(TARGET_LITTLE_ENDIAN);
	if (ret != EXIT_SUCCESS)
		return EXIT_FAILURE;

	ret = fine_set_frequency(16, 120);
	if (ret != EXIT_SUCCESS)
		return EXIT_FAILURE;

	ret = fine_set_bitrate(1000000);
	if (ret != EXIT_SUCCESS)
		return EXIT_FAILURE;

	ret = fine_send_sync();
	if (ret != EXIT_SUCCESS)
		return EXIT_FAILURE;

	ret = fine_get_serial_protect_state();
	if (ret != EXIT_SUCCESS)
		return EXIT_FAILURE;

	ret = fine_check_id_code(id_code);
	if (ret != EXIT_SUCCESS)
		return EXIT_FAILURE;

	ret = fine_get_device_mem_info();
	if (ret != EXIT_SUCCESS)
		return EXIT_FAILURE;

	jaylink_close(devh);
	jaylink_exit(ctx);

	return 0;
}