
#define PKT_CMD				0
#define PKT_STATUS			0x80

#define FINE_TIMEOUT			0x64
#define FINE_RETRY_ID_COUNT		10

#define FINE_START_SEQ			0x9D4375C0

#define FINE_GET_CHIP_ID		0xC2
#define FINE_ASK_TARGET_ACK		0xC6
#define FINE_ASK_TARGET_DATA		0xC4

#define FINE_CMD_SYNC			0x00
#define FINE_CMD_GET_AUTH_MODE		0x2C
#define FINE_CMD_CHECK_ID_CODE		0x30
#define FINE_CMD_SET_FREQUENCY		0x32
#define FINE_CMD_SET_BITRATE		0x34
#define FINE_CMD_SET_ENDIANNSESS	0x36
#define FINE_CMD_GET_DEVICE_TYPE	0x38
#define FINE_CMD_GET_AREA_COUNT		0x53
#define FINE_CMD_GET_AREA_INFO		0x54

#define FINE_CMD_SOH			0x01
#define FINE_CMD_ETX			0x03

#define FINE_CMD_ERR_NOT_SUPPORTED	0xC0
#define FINE_CMD_ERR_PACKET		0xC1
#define FINE_CMD_ERR_CHECKSUM		0xC2
#define FINE_CMD_ERR_FLOW		0xC3
#define FINE_CMD_ERR_ADDRESS		0xD0
#define FINE_CMD_ERR_INPUT_FREQU	0xD1
#define FINE_CMD_ERR_SYS_CLK		0xD2
#define FINE_CMD_ERR_AREA		0xD5
#define FINE_CMD_ERR_BIT_RATE		0xD4
#define FINE_CMD_ERR_ENDIAN		0xD7
#define FINE_CMD_ERR_PROTECTION		0xDA
#define FINE_CMD_ERR_WRONG_ID		0xDB
#define FINE_CMD_ERR_SERIAL_CNX		0xDC
#define FINE_CMD_ERR_NON_BLANK		0xE0
#define FINE_CMD_ERR_ERASE		0xE1
#define FINE_CMD_ERR_PROGRAM		0xE2
#define FINE_CMD_ERR_FLASH_SEQ		0xE7

#define TARGET_LITTLE_ENDIAN		2

int init_jlink(enum jaylink_target_interface iface, struct jaylink_context *ctx);
int fine_get_chip_id(void);
int fine_init_chip(void);
int fine_get_device_type(void);
int fine_set_endianness(int endianness);
int fine_set_frequency(int in_freq, int sys_freq);
int fine_set_bitrate(int bitrate);
int fine_send_sync(void);
int fine_get_serial_protect_state(void);
int fine_check_id_code(uint8_t *id);
int fine_get_device_mem_info(void);
