#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <hce.h>
#include <snfc.h>
#include <trusted_hce_ta_entry.h>

static snfc_socket s;

static int execute_operation(uint8_t op_code, uint32_t card_id, uint32_t value, char *send_buf, size_t send_buf_size, char *recv_buf, size_t recv_buf_size)
{
	int res = 0;

	// Create request according to terminal arguments
	if((res = create_request(op_code, card_id, value, send_buf, &send_buf_size)) == TEEC_SUCCESS)
	{
		// Send request and receive response
		if((res = snfc_initiator_transceive_raw_bytes(&s, send_buf, send_buf_size, recv_buf, recv_buf_size, 1000 * 3)) > 0)
		{
			if((res = process_response(op_code, recv_buf, res)) != TEEC_SUCCESS)
			{
				printf("%s:%s:%d: Failed process_response, error = %x\n", __FILE__, __func__, __LINE__, res);
			}
		}
		else printf("%s:%s:%d: Failed snfc_initiator_transceive_raw_bytes, error = %x\n", __FILE__, __func__, __LINE__, res);
	}
	else printf("%s:%s:%d: Failed create_request, error = %x\n", __FILE__, __func__, __LINE__, res);

	return res;
}

const int NANO_SEC_IN_SECS = 1000000000;
static long secs_to_nano(long secs)
{
	return secs * NANO_SEC_IN_SECS;
}

const int NANO_SEC_IN_MILLI_SECS = 1000000;
static double nano_to_milli(long nano)
{
	return nano / NANO_SEC_IN_MILLI_SECS;
}

const int MILLI_SEC_IN_SECS = 1000;
static long secs_to_milli(long secs)
{
	return secs * MILLI_SEC_IN_SECS;
}

static long get_elapsed_time_nanos(struct timespec begin, struct timespec end)
{
	return secs_to_nano(end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec);
}

static double get_elapsed_time_millis(struct timespec begin, struct timespec end)
{
	return secs_to_milli(end.tv_sec - begin.tv_sec) + nano_to_milli(end.tv_nsec - begin.tv_nsec);
}

// https://btorpey.github.io/blog/2014/02/18/clock-sources-in-linux/
// https://books.google.pt/books?id=93iaDwAAQBAJ&pg=PA83&lpg=PA83&ots=TVBAqCHPdl&focus=viewport&dq=clock_gettime+evaluation
// https://www.guyrutenberg.com/2007/09/22/profiling-code-using-clock_gettime/

#define N_RUNS 50
static void evaluate_operation(uint8_t op_code, uint32_t card_id, uint32_t value, char *send_buf, size_t send_buf_size, char *recv_buf, size_t recv_buf_size)
{
	TEEC_Result res = TEEC_SUCCESS;
	
	struct timespec begin, end;
	long total_time, elapsed;
	total_time = elapsed = 0;

	for(int i = 0; i < N_RUNS && res == TEEC_SUCCESS; i++)
	{
		clock_gettime(CLOCK_MONOTONIC, &begin);		

		// Create request according to terminal arguments
		if((res = create_request(op_code, i, value, send_buf, &send_buf_size)) != TEEC_SUCCESS)
		{
			printf("%s:%s:%d: Failed create_request, error = %x\n", __FILE__, __func__, __LINE__, res);
			exit(-1);
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = get_elapsed_time_nanos(begin, end);
		elapsed -= get_evaluation_time() * NANO_SEC_IN_MILLI_SECS;
		printf("%ld\n", elapsed);
	}
}

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
	uint32_t value = argc == 3 ? atoi(argv[2]) : 0;

	// Evaluate Purchase

	printf("PURCHASE Evaluation:\n");
	evaluate_operation(PURCHASE, card_id, 125, send_buf, send_buf_size, recv_buf, recv_buf_size);
	memset(send_buf, 0, send_buf_size);
	memset(recv_buf, 0, recv_buf_size);
/*
	// Evaluate Validate
	printf("VALIDATE Evaluation:\n");
	evaluate_operation(VALIDATE, card_id, value, send_buf, send_buf_size, recv_buf, recv_buf_size);
	memset(send_buf, 0, send_buf_size);
	memset(recv_buf, 0, recv_buf_size);

	// Evaluate USE
	printf("USE Evaluation:\n");
	evaluate_operation(USE, card_id, value, send_buf, send_buf_size, recv_buf, recv_buf_size);
	memset(send_buf, 0, send_buf_size);
	memset(recv_buf, 0, recv_buf_size);

	// Evaluate RECHARGE
	printf("RECHARGE Evaluation:\n");
	evaluate_operation(RECHARGE, card_id, value, send_buf, send_buf_size, recv_buf, recv_buf_size);
	memset(send_buf, 0, send_buf_size);
	memset(recv_buf, 0, recv_buf_size);
*/

ERROR:

	if(send_buf) free(send_buf);
	if(recv_buf) free(recv_buf);

	snfc_initiator_close(&s);
	exit_thce_ta();

	return 0;
}
