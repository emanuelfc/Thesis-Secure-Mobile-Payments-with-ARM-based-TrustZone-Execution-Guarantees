#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <hce.h>
#include <snfc.h>
#include <trusted_hce_ta_entry.h>

static snfc_socket s;

static void print_help()
{
	printf("Usage:\n");
	printf("\n");
	printf("\t SNFC Arguments\n");
	printf("\t\t spi\n");
	printf("\t\t kei\n");
	printf("\t\t flags\n");
	printf("\n");
	printf("\t Trusted HCE Arguments\n");
	printf("\t\t op_code\n");
	printf("\t\t card_id\n");
	printf("\t\t value\n");
}

int main(int argc, char **argv)
{
	if(argc < 5)
	{
		print_help();
		return -1;
	}
	
	int res = 0;

	size_t recv_buf_size = 1028;
	char *recv_buf = calloc(recv_buf_size, sizeof(char));

	size_t send_buf_size = 1028;
	char *send_buf = calloc(send_buf_size, sizeof(char));

	if((res = init_thce_ta()) != TEEC_SUCCESS)
	{
		printf("%s:%s:%d: Failed init_thce_ta, error = %d\n", __FILE__, __func__, __LINE__, res);
		return -1;
	}

	// Initiate SNFC
	if((res = snfc_initiator_init(&s)) != NFC_SUCCESS)
	{
		printf("%s:%s:%d: Failed snfc_initiator_init\n", __FILE__, __func__, __LINE__);
		return -1;
	}

	// Start initiator SNFC
	if((res = snfc_initiator_start(&s, atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), 0)) < 0)
	{
		printf("%s:%s:%d: Failed snfc_initiator_start, error = %x\n", __FILE__, __func__, __LINE__, res);
		goto ERROR;
	}

	argv += 4;
	argc -= 4;

	// HCE

	uint8_t op_code = atoi(argv[0]);
	uint32_t card_id = atoi(argv[1]);
	uint32_t value = atoi(argv[2]);

	// Create request according to terminal arguments
	if((res = create_request(op_code, card_id, value, send_buf, &send_buf_size)) != TEEC_SUCCESS)
	{
		printf("%s:%s:%d: Failed create_request, error = %x\n", __FILE__, __func__, __LINE__, res);
		goto ERROR;
	}

	// Send request and receive response
	if((res = snfc_initiator_transceive_raw_bytes(&s, send_buf, send_buf_size, recv_buf, recv_buf_size, 1000 * 3)) <= 0)
	{
		printf("%s:%s:%d: Failed nfc_initiator_transceive_bytes, error = %x\n", __FILE__, __func__, __LINE__, res);
		goto ERROR;
	}

	// Process response
	if((res = process_response(op_code, recv_buf, res)) != TEEC_SUCCESS)
	{
		printf("%s:%s:%d: Failed process_response, error = %x\n", __FILE__, __func__, __LINE__, res);
		goto ERROR;
	}

ERROR:

	if(send_buf) free(send_buf);
	if(recv_buf) free(recv_buf);

	snfc_initiator_close(&s);
	exit_thce_ta();

	return 0;
}
