#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <hce.h>
#include <snfc.h>

static hce_card dummy_card =
{
	.id = 0,
	.expiration_date = 999999999,
	.value = 999
};

static snfc_socket s;

static void create_request(char **argv, char *write_buf, size_t *write_buf_size)
{
	uint8_t op_code = (uint8_t)atoi(argv[1]);

	// Write operation code
	memcpy(write_buf, &op_code, sizeof(op_code));
	write_buf += sizeof(op_code);
	*write_buf_size = sizeof(op_code);

	uint32_t extra_arg = atoi(argv[2]);

	hce_card card;

	switch(op_code)
	{
		case VALIDATE:
		case PURCHASE:
			memcpy(write_buf, &extra_arg, sizeof(extra_arg));
			*write_buf_size += sizeof(extra_arg);
			break;

		case USE:
		case RECHARGE:
			card = dummy_card;
			*write_buf_size += write_hce_card(&card, write_buf);
			break;

		default:
			printf("%s:%d: Invalid operation code, code = %d\n", __func__, __LINE__, op_code);
	}
}

static void process_response(uint8_t op_code, char *buf)
{
	uint8_t response_code = (uint8_t)*buf;
	buf += sizeof(response_code);

	hce_card card;

	if(response_code)
	{
		switch(op_code)
		{
			case VALIDATE:
				//printf("Validate OK\n");
				break;

			case PURCHASE:
			case USE:
			case RECHARGE:
				card = read_hce_card(buf);
				print_card(&card);
				break;

			default:
				printf("%s:%d: Invalid operation code\n", __func__, __LINE__);
		}
	}
	else printf("%s:%d: Failed process_response, op_code = %d\n", __func__, __LINE__, response_code);
}

int main(int argc, char **argv)
{
	int res = 0;

	// Initiate SNFC
	if((res = snfc_initiator_init(&s)) != NFC_SUCCESS)
	{
		printf("%s:%d: Failed snfc_initiator_init\n", __func__, __LINE__);
		return -1;
	}

	// Start initiator SNFC

	if((res = snfc_initiator_start(&s, atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), 0)) < 0)
	{
		printf("%s:%d: Failed snfc_initiator_start, error = %x\n", __func__, __LINE__, res);
		goto ERROR;
	}

	argv += 3;

	// HCE

	size_t recv_buf_size = 1028;
	char *recv_buf = calloc(recv_buf_size, sizeof(char));

	size_t send_buf_size = 1028;
	char *send_buf = calloc(send_buf_size, sizeof(char));

	int op_code = atoi(argv[0]);

	// Create request according to terminal arguments
	create_request(argv, send_buf, &send_buf_size);

	// Send request and receive response
	if((res = snfc_initiator_transceive(&s, send_buf, send_buf_size, recv_buf, recv_buf_size, 0)) <= 0)
	{
		printf("%s:%d: Failed nfc_initiator_transceive_bytes, error = %x\n", __func__, __LINE__, res);
		goto ERROR;
	}
	
	//recv_buf_size = res;

	process_response(op_code, recv_buf);

ERROR:

	snfc_initiator_close(&s);

	return 0;
}
