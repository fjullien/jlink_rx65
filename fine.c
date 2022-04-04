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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <libjaylink/libjaylink.h>

#include "helpers.h"
#include "fine.h"

extern struct jaylink_device_handle *devh;

const char *fine_strerror(int error_code)
{
	switch (error_code) {
	case FINE_CMD_ERR_NOT_SUPPORTED:
		return "command not supported";
	case FINE_CMD_ERR_PACKET:
		return "packet error";
	case FINE_CMD_ERR_CHECKSUM:
		return "checksum error";
	case FINE_CMD_ERR_FLOW:
		return "flow error";
	case FINE_CMD_ERR_ADDRESS:
		return "bad address";
	case FINE_CMD_ERR_INPUT_FREQU:
		return "bad input frequency";
	case FINE_CMD_ERR_SYS_CLK:
		return "bad system clock frequency";
	case FINE_CMD_ERR_AREA:
		return "area error";
	case FINE_CMD_ERR_BIT_RATE:
		return "bit rate error";
	case FINE_CMD_ERR_ENDIAN:
		return "endian error";
	case FINE_CMD_ERR_PROTECTION:
		return "protection error";
	case FINE_CMD_ERR_WRONG_ID:
		return "ID code mismatch error";
	case FINE_CMD_ERR_SERIAL_CNX:
		return "serial programmer connection prohibition error";
	case FINE_CMD_ERR_NON_BLANK:
		return "non-blank error";
	case FINE_CMD_ERR_ERASE:
		return "error of erasure";
	case FINE_CMD_ERR_PROGRAM:
		return "program error";
	case FINE_CMD_ERR_FLASH_SEQ:
		return "flash sequencer error";
	default:
		return "unknown error";
	}
}

int init_jlink(enum jaylink_target_interface iface, struct jaylink_context *ctx)
{
	size_t count;
	int ret = jaylink_init(&ctx);

	if (ret != JAYLINK_OK) {
		printf("jaylink_init() failed: %s.",jaylink_strerror_name(ret));
		return -1;
	}

	ret = jaylink_discovery_scan(ctx, 0);

	if (ret != JAYLINK_OK) {
		printf("jaylink_discovery_scan() failed: %s.\n",
			jaylink_strerror_name(ret));
		jaylink_exit(ctx);
		return EXIT_FAILURE;
	}

	struct jaylink_device **devs;
	ret = jaylink_get_devices(ctx, &devs, NULL);

	if (ret != JAYLINK_OK) {
		printf("jaylink_get_device_list() failed: %s.\n",
			jaylink_strerror_name(ret));
		jaylink_exit(ctx);
		return EXIT_FAILURE;
	}

	uint32_t tmp;
	bool device_found = false;
	uint32_t serial_number;

	for (int i = 0; devs[i]; i++) {
		devh = NULL;
		ret = jaylink_device_get_serial_number(devs[i], &tmp);

		if (ret != JAYLINK_OK) {
			printf("jaylink_device_get_serial_number() failed: "
				"%s.\n", jaylink_strerror_name(ret));
			continue;
		}

		ret = jaylink_open(devs[i], &devh);

		if (ret == JAYLINK_OK) {
			serial_number = tmp;
			device_found = true;
			break;
		}

		printf("jaylink_open() failed: %s.\n",
			jaylink_strerror_name(ret));
	}

	jaylink_free_devices(devs, true);

	if (!device_found) {
		printf("No J-Link device found.\n");
		jaylink_exit(ctx);
		return EXIT_SUCCESS;
	}

	printf("S/N: %012u\n", serial_number);

	char *firmware_version;
	size_t length;
	ret = jaylink_get_firmware_version(devh, &firmware_version, &length);

	if (ret != JAYLINK_OK) {
		printf("jaylink_get_firmware_version() failed: %s.\n",
			jaylink_strerror_name(ret));
		jaylink_close(devh);
		jaylink_exit(ctx);
		return EXIT_FAILURE;
	} else if (length > 0) {
		printf("Firmware: %s\n", firmware_version);
		free(firmware_version);
	}

	uint32_t interfaces;
	ret = jaylink_get_available_interfaces(devh, &interfaces);

	if (ret != JAYLINK_OK) {
		printf("jaylink_get_available_interfaces() failed: %s",
			jaylink_strerror(ret));
		return EXIT_FAILURE;
	}

	if (!(interfaces & (1 << iface))) {
		printf("Selected transport (FINE) is not supported by the device");
		return EXIT_FAILURE;
	}

	jaylink_clear_reset(devh);
	jaylink_set_reset(devh);

	ret = jaylink_select_interface(devh, iface, NULL);

	if (ret < 0) {
		printf("jaylink_select_interface() failed: %s",
			jaylink_strerror(ret));
		return EXIT_FAILURE;
	}

	jaylink_set_reset(devh);
	jaylink_jtag_set_trst(devh);
}

int fine_get_chip_id(void)
{
	uint8_t out[16];
	uint8_t in[8];
	uint8_t expected[8];

	int ret;
	int retry = 0;

	buf_set_u32_be(out, 0, FINE_START_SEQ);

	memset(in, 0, 2);
	buf_set_u32(expected, 0, 16, 0x0223);

	/* When target receive FINE_START_SEQ, it responds 0x23 0x02 */
	while ((retry < FINE_RETRY_ID_COUNT) && memcmp(in, expected, 2)) {
		ret = jaylink_fine_io(devh, out, in, 4, 2, FINE_TIMEOUT);
		if (ret != JAYLINK_OK) {
			printf("Error during FINE xfer");
			return ret;
		}
		retry++;
	}

	if (retry == FINE_RETRY_ID_COUNT) {
		printf("Couldn't init FINE");
		return EXIT_FAILURE;
	}

	out[0] = FINE_GET_CHIP_ID;
	ret = jaylink_fine_io(devh, out, in, 1, 2, FINE_TIMEOUT);
	if (ret != JAYLINK_OK) {
		printf("Error during FINE xfer");
		return ret;
	}

	buf_bswap16(in, in, 2);
	printf("Found chip id %s\n", buf_to_hex_str(in, 16));

	return EXIT_SUCCESS;
}

int fine_init_chip(void)
{
	uint8_t out[16];
	uint8_t in[8];

	int ret;
	int retry = 0;

	out[0] = 0x88;
	out[1] = 0x01;
	out[2] = 0x00;
	out[3] = 0x88;
	out[4] = 0x03;
	out[5] = 0x00;
	out[6] = 0x88;
	out[7] = 0x02;
	out[8] = 0x00;
	out[9] = FINE_ASK_TARGET_ACK;

	ret = jaylink_fine_io(devh, out, in, 10, 2, FINE_TIMEOUT);
	if (ret != JAYLINK_OK) {
		printf("Error during FINE xfer");
		return ret;
	}

	/* Target responds 0x2000 after power-up, 0x0000 after that */
/*	buf_set_u32(expected, 0, 16, 0x0000);
	if (memcmp(expected, in, 2)) {
		buf_set_u32(expected, 0, 16, 0x0020);
		if (memcmp(expected, in, 2)) {
			printf("Error during FINE initialization");
			return EXIT_FAILURE;
		}
	}
*/
	out[0] = 0x84;
	out[1] = 0x55;
	out[2] = 0x00;
	out[3] = 0x00;
	out[4] = 0x00;

	ret = jaylink_fine_io(devh, out, in, 5, 1, FINE_TIMEOUT);
	if (ret != JAYLINK_OK) {
		printf("Error during FINE xfer");
		return ret;
	}

	if (in[0]) {
		printf("Error during FINE initialization");
		return EXIT_FAILURE;
	}

	out[0] = FINE_ASK_TARGET_DATA;
	in[0] = 0x0E;
	while ((in[0] == 0x0E) && (retry < 100)) {
		ret = jaylink_fine_io(devh, out, in, 1, 1, 0x64);
		if (ret != JAYLINK_OK) {
			printf("jaylink_fine_io failed: %s", jaylink_strerror(ret));
			return EXIT_FAILURE;
		}
		retry++;
		usleep(10000);
	}

	if (retry == 100) {
		printf("FINE: device timeout");
		return EXIT_FAILURE;
	}

	out[0] = FINE_ASK_TARGET_ACK;
	ret = jaylink_fine_io(devh, out, in, 1, 2, 0x64);
	if (ret != JAYLINK_OK) {
		printf("jaylink_fine_io failed: %s", jaylink_strerror(ret));
		return EXIT_FAILURE;
	}

	if (in[0] || in[1])
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

static int fine_send_cmd_ack(void)
{
	uint8_t out = FINE_ASK_TARGET_ACK;
	uint8_t in[2];
	int ret;

	ret = jaylink_fine_io(devh, &out, in, 1, 2, 0x64);
	if (ret != JAYLINK_OK) {
		printf("fine_send_cmd_continue failed: %s", jaylink_strerror(ret));
		return EXIT_FAILURE;
	}

	if (in[0] || in[1])
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

static int fine_send_cmd(uint8_t cmd_status, uint8_t cmd, uint8_t *data, uint16_t data_len)
{
	uint8_t out[5];
	uint8_t in[2];
	int idx = 0;
	int ret;

	out[0] = 0x84;
	out[1] = FINE_CMD_SOH | cmd_status;
	out[2] = (data_len + 1) >> 8;
	out[3] = (data_len + 1) & 0xFF;
	out[4] = cmd;

	uint8_t crc = 0;
	for (int i = 0; i < 3; i++)
		crc += out[2 + i];
	for (int i = 0; i < data_len; i++)
		crc += data[i];
	crc = ~crc + 1;

	ret = jaylink_fine_io(devh, out, in, 5, 1, 0x64);
	if (ret != JAYLINK_OK) {
		printf("jaylink_fine_io failed: %s", jaylink_strerror(ret));
		return EXIT_FAILURE;
	}

	fine_send_cmd_ack();

	while (data_len > 3) {
		memcpy(&out[1], &data[idx], 4);
		ret = jaylink_fine_io(devh, out, in, 5, 1, 0x64);
		if (ret != JAYLINK_OK) {
			printf("jaylink_fine_io failed: %s", jaylink_strerror(ret));
			return EXIT_FAILURE;
		}

		idx += 4;
		data_len -= 4;

		fine_send_cmd_ack();
	}

	memset(&out[1], 0, 4);
	switch (data_len) {
	case 0:
		out[1] = crc;
		out[2] = FINE_CMD_ETX;
		break;
	case 1:
		out[1] = data[idx];
		out[2] = crc;
		out[3] = FINE_CMD_ETX;
		break;
	case 2:
		out[1] = data[idx];
		out[2] = data[idx + 1];
		out[3] = crc;
		out[4] = FINE_CMD_ETX;
		break;
	case 3:
		out[1] = data[idx];
		out[2] = data[idx + 1];
		out[3] = data[idx + 2];
		out[4] = crc;
		break;
	}

	ret = jaylink_fine_io(devh, out, in, 5, 1, 0x64);

	if (data_len == 3) {
		fine_send_cmd_ack();
		out[1] = FINE_CMD_ETX;
		out[2] = 0;
		out[3] = 0;
		out[4] = 0;
		ret = jaylink_fine_io(devh, out, in, 5, 1, 0x64);
	}

	return EXIT_SUCCESS;
}

static int fine_get_status_packet(void)
{
	uint8_t out[5];
	uint8_t in[5];
	uint8_t status[8];

	int ret;

	out[0] = FINE_ASK_TARGET_DATA;
	ret = jaylink_fine_io(devh, out, in, 1, 5, 0x64);
	if (ret != JAYLINK_OK) {
		printf("fine_get_status_packet failed: %s", jaylink_strerror(ret));
		return EXIT_FAILURE;
	}

	memcpy(status, &in[1], 4);

	out[0] = FINE_ASK_TARGET_DATA;
	ret = jaylink_fine_io(devh, out, in, 1, 5, 0x64);
	if (ret != JAYLINK_OK) {
		printf("fine_get_status_packet failed: %s", jaylink_strerror(ret));
		return EXIT_FAILURE;
	}

	memcpy(&status[4], &in[1], 4);

	ret = fine_send_cmd_ack();
	if (ret != EXIT_SUCCESS)
		return ret;

	if (!(status[3] & 0x80))
		return EXIT_SUCCESS;

	return status[4];
}

static int fine_get_data(uint8_t *buffer)
{
	uint8_t out[5];
	uint8_t in[5];
	int nb_xfer;
	int ret;

	out[0] = FINE_ASK_TARGET_DATA;
	ret = jaylink_fine_io(devh, out, in, 1, 5, 0x64);
	if (ret != JAYLINK_OK) {
		printf("fine_get_status_packet failed: %s", jaylink_strerror(ret));
		return EXIT_FAILURE;
	}

	memcpy(buffer, &in[1], 4);

	int len = 4;

	nb_xfer = ((in[2] << 8) + in[3] + 1) / 4;
	if ((in[3] + 1) % 4)
		nb_xfer++;

	for (int i = 0; i < nb_xfer; i++) {
		out[0] = FINE_ASK_TARGET_DATA;
		ret = jaylink_fine_io(devh, out, in, 1, 5, 0x64);
		if (ret != JAYLINK_OK) {
			printf("fine_get_status_packet failed: %s", jaylink_strerror(ret));
			return EXIT_FAILURE;
		}

		memcpy(&buffer[4 + (i * 4)], &in[1], 4);

		len += 4;
	}

	fine_send_cmd_ack();

	return len;
}

int fine_get_device_type(void)
{
	int ret;
	uint8_t buff[200];

	printf("FINE: Get device type\n");

	ret = fine_send_cmd(PKT_CMD, FINE_CMD_GET_DEVICE_TYPE, NULL, 0);
	if (ret != EXIT_SUCCESS)
		return ret;

	ret = fine_get_status_packet();
	if (ret != EXIT_SUCCESS) {
		printf("FINE error: %s", fine_strerror(ret));
		return ret;
	}

	ret = fine_send_cmd(PKT_STATUS, FINE_CMD_GET_DEVICE_TYPE, NULL, 0);
	if (ret != EXIT_SUCCESS)
		return ret;

	if (fine_get_data(buff) < 0)
		return EXIT_FAILURE;

	printf("max_input_clk_freq = %d\n", buf_get_u32_be(buff, 12));
	printf("min_input_clk_freq = %d\n", buf_get_u32_be(buff, 16));
	printf("max_sys_clk_freq   = %d\n", buf_get_u32_be(buff, 20));
	printf("min_sys_clk_freq   = %d\n", buf_get_u32_be(buff, 24));


	//memcpy(rx_device->type, &buff[4], 8);


	return EXIT_SUCCESS;
}

int fine_set_endianness(int endianness)
{
	uint8_t val = endianness == TARGET_LITTLE_ENDIAN ? 1 : 0;
	int ret;

	printf("FINE: Set endianness\n");

	ret = fine_send_cmd(PKT_CMD, FINE_CMD_SET_ENDIANNSESS, &val, 1);
	if (ret != EXIT_SUCCESS)
		return ret;

	ret = fine_get_status_packet();
	if (ret != EXIT_SUCCESS) {
		printf("FINE command error: %s\n", fine_strerror(ret));
		return ret;
	}

	return EXIT_SUCCESS;
}

int fine_set_frequency(int in_freq, int sys_freq)
{
	uint8_t buff[100];
	uint8_t out[8];
	int ret;

	printf("FINE: Set frequency\n");

	buf_set_u32_be(out, 0, in_freq * 1000000);
	buf_set_u32_be(out, 4, sys_freq * 1000000);

	ret = fine_send_cmd(PKT_CMD, FINE_CMD_SET_FREQUENCY, out, 8);
	if (ret != EXIT_SUCCESS)
		return ret;

	ret = fine_get_status_packet();
	if (ret != EXIT_SUCCESS) {
		printf("FINE command error: %s", fine_strerror(ret));
		return ret;
	}

	ret = fine_send_cmd(PKT_STATUS, FINE_CMD_SET_FREQUENCY, NULL, 0);
	if (ret != EXIT_SUCCESS)
		return ret;


	if (fine_get_data(buff) < 0)
		return EXIT_FAILURE;

	printf("System frequency set to     %d\n", buf_get_u32_be(buff, 4));
	printf("Peripheral frequency set to %d\n", buf_get_u32_be(buff, 8));

	return EXIT_SUCCESS;
}

int fine_set_bitrate(int bitrate)
{
	uint8_t out[4];
	int ret;

	printf("FINE: Set bitrate\n");

	buf_set_u32_be(out, 0, bitrate);

	ret = fine_send_cmd(PKT_CMD, FINE_CMD_SET_BITRATE, out, 4);
	if (ret != EXIT_SUCCESS)
		return ret;

	ret = fine_get_status_packet();
	if (ret != EXIT_SUCCESS) {
		printf("FINE command error: %s\n", fine_strerror(ret));
		return ret;
	}

	return EXIT_SUCCESS;
}

int fine_send_sync(void)
{
	int ret;

	printf("FINE: Send sync\n");

	ret = fine_send_cmd(PKT_CMD, FINE_CMD_SYNC, NULL, 0);
	if (ret != EXIT_SUCCESS)
		return ret;

	ret = fine_get_status_packet();
	if (ret != EXIT_SUCCESS) {
		printf("FINE command error: %s\n", fine_strerror(ret));
		return ret;
	}

	return EXIT_SUCCESS;
}

int fine_get_serial_protect_state(void)
{
	uint8_t buff[8];
	int ret;

	printf("FINE: Get authentication mode\n");

	ret = fine_send_cmd(PKT_CMD, FINE_CMD_GET_AUTH_MODE, NULL, 0);
	if (ret != EXIT_SUCCESS)
		return ret;

	ret = fine_get_status_packet();
	if (ret != EXIT_SUCCESS) {
		printf("FINE command error: %s", fine_strerror(ret));
		return ret;
	}

	ret = fine_send_cmd(PKT_STATUS, FINE_CMD_GET_AUTH_MODE, NULL, 0);
	if (ret != EXIT_SUCCESS)
		return ret;

	if (fine_get_data(buff) < 0)
		return EXIT_FAILURE;

	printf("Serial boot allowed = %d\n", buff[4] ? 0 : 1);

	return EXIT_SUCCESS;
}

int fine_check_id_code(uint8_t *id)
{
	int ret;

	printf("FINE: Check ID code\n");

	ret = fine_send_cmd(PKT_CMD, FINE_CMD_CHECK_ID_CODE, id, 16);
	if (ret != EXIT_SUCCESS)
		return ret;

	ret = fine_get_status_packet();
	if (ret != EXIT_SUCCESS) {
		printf("FINE command error: %s\n", fine_strerror(ret));
		return ret;
	}

	return EXIT_SUCCESS;
}

int fine_get_device_mem_info(void)
{
	uint8_t buff[24];
	int ret;

	printf("FINE: Get area informations\n");

	ret = fine_send_cmd(PKT_CMD, FINE_CMD_GET_AREA_COUNT, NULL, 0);
	if (ret != EXIT_SUCCESS)
		return ret;

	ret = fine_get_status_packet();
	if (ret != EXIT_SUCCESS) {
		printf("FINE command error: %s", fine_strerror(ret));
		return ret;
	}

	ret = fine_send_cmd(PKT_STATUS, FINE_CMD_GET_AREA_COUNT, NULL, 0);
	if (ret != EXIT_SUCCESS)
		return ret;

	if (fine_get_data(buff) < 0)
		return EXIT_FAILURE;

	int area_count = buff[4];

	for (uint8_t i = 0; i < area_count; i++) {
		printf("FINE: Area %d Information Acquisition Command\n", i);

		ret = fine_send_cmd(PKT_CMD, FINE_CMD_GET_AREA_INFO, &i, 1);
		if (ret != EXIT_SUCCESS)
			return ret;

		ret = fine_get_status_packet();
		if (ret != EXIT_SUCCESS) {
			printf("FINE command error: %s", fine_strerror(ret));
			return ret;
		}

		ret = fine_send_cmd(PKT_STATUS, FINE_CMD_GET_AREA_INFO, NULL, 0);
		if (ret != EXIT_SUCCESS)
			return ret;

		if (fine_get_data(buff) < 0)
			return EXIT_FAILURE;

		printf("area[%d].koa = %x\n", i, buff[4]);
		printf("area[%d].sad = %x\n", i, buf_get_u32_be(buff, 5));
		printf("area[%d].ead = %x\n", i, buf_get_u32_be(buff, 9));
		printf("area[%d].eau = %x\n", i, buf_get_u32_be(buff, 13));
		printf("area[%d].wau = %x\n", i, buf_get_u32_be(buff, 17));
	}

	return EXIT_SUCCESS;
}