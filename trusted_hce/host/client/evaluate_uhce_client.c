#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <hce.h>
#include <nfc.h>

#include <data_store_storage.h>
#include <data_store.h>

static nfc_device *pnd;
static nfc_context *context;
static nfc_target target;

static data_store *store;

static char* ui32toa(uint32_t value)
{
	size_t str_size = 33;	
	char *str = calloc(str_size, sizeof(char));
	snprintf(str, str_size, "%d", value);
	return str;
}

static int write_data_store_hce_card(hce_card *card)
{
	int res = 0;
	
	char *str_id = ui32toa(card->id);

	if((res = data_store_insert(store, str_id, CARD_ID_FIELD, (unsigned char*)&card->id, sizeof(card->id))))
	{
		if((res = data_store_insert(store, str_id, CARD_EXPIRATION_DATE_FIELD, (unsigned char*)&card->expiration_date, sizeof(card->expiration_date))))
		{
			res = data_store_insert(store, str_id, CARD_VALUE_FIELD, (unsigned char*)&card->value, sizeof(card->value));
		}
	}
	
	free(str_id);


	return res;
}

static int read_data_store_hce_card(uint32_t id, hce_card *card)
{
	int res = 0;

	char *str_id = ui32toa(id);

	card->id = id;

	data *d = NULL;

	if((d = data_store_get(store, str_id, CARD_EXPIRATION_DATE_FIELD)))
	{
		card->expiration_date = *(uint32_t*)d->value;	
		if((d = data_store_get(store, str_id, CARD_VALUE_FIELD)))
		{
			card->value = *(uint32_t*)d->value;
			res = 1;
		}
	}

	free(str_id);
	
	return res;
}

static uint32_t get_request_buffer_size(uint8_t op_code)
{
	uint32_t request_size = sizeof(uint8_t); // We must have an operation code

	switch(op_code)
	{
		case PURCHASE:
			request_size += sizeof(uint32_t); // For the value amount
			break;

		case RECHARGE:
			request_size += sizeof(uint32_t); // For the recharge value amount
		case USE:
		case VALIDATE:
			request_size += sizeof(hce_card); // For the requests which we need to send the card
			break;

		default:
			request_size = 0;
	}

	return request_size;
}

static int is_valid_request(uint8_t op_code)
{
	switch(op_code)
	{
		case PURCHASE:
		case VALIDATE:
		case USE:
		case RECHARGE:
			return 1;

		default:
			printf("%s:%s:%d: Invalid operation code, op_code = %d\n", __FILE__, __func__, __LINE__, op_code);
			return 0;
	}
}

static int create_request(uint8_t op_code, uint32_t card_id, uint32_t value, char *write_buf, size_t *write_buf_size)
{
	if(!is_valid_request(op_code)) return 0;

	*write_buf_size = get_request_buffer_size(op_code);

	int res = 0;

	// Write operation code
	memcpy(write_buf, &op_code, sizeof(op_code));
	write_buf += sizeof(op_code);

	hce_card card;

	switch(op_code)
	{
		case PURCHASE:
			memcpy(write_buf, &value, sizeof(value));
			write_buf += sizeof(value);
			res = 1;
			break;

		case VALIDATE:
		case USE:
		case RECHARGE:
			if(read_data_store_hce_card(card_id, &card))
			{
				write_buf += write_hce_card(&card, write_buf);
				memcpy(write_buf, &value, sizeof(value));
				res = 1;
			}
			break;

		default:
			res = 0;
			printf("%s:%s:%d: Invalid operation code, op_code = %d\n", __FILE__, __func__, __LINE__, op_code);
	}

	return res;
}

static int process_response(uint8_t op_code, char *response, size_t response_size)
{
	int res = 0;	
		
	uint8_t response_code = *(uint8_t*)response;
	response += sizeof(response_code);

	if(response_code)
	{
		hce_card card;
		char *card_id_str = NULL;

		switch(op_code)
		{
			case VALIDATE:
				res = 1;
				break;

			case PURCHASE:
				card = read_hce_card(response);
				res = write_data_store_hce_card(&card);
				break;

			case USE:
			case RECHARGE:
				card = read_hce_card(response);
				card_id_str = ui32toa(card.id);
				res = data_store_insert(store, card_id_str, CARD_VALUE_FIELD, (unsigned char*)&card.value, sizeof(card.value));
				free(card_id_str);
				break;

			default:
				printf("%s:%s:%d: Invalid operation code, op_code = %d\n", __FILE__, __func__, __LINE__, op_code);
		}	
	}
	else printf("%s:%s:%d: Invalid response_code, response_code = %d\n", __FILE__, __func__, __LINE__, response_code);

	return res;
}

static int execute_operation(uint8_t op_code, uint32_t card_id, uint32_t value, char *send_buf, size_t send_buf_size, char *recv_buf, size_t recv_buf_size)
{
	int res = 0;

	// Create request according to terminal arguments
	if((res = create_request(op_code, card_id, value, send_buf, &send_buf_size)))
	{
		// Send request and receive response
		if((res = nfc_initiator_transceive_bytes(pnd, (uint8_t*)send_buf, send_buf_size, (uint8_t*)recv_buf, recv_buf_size, 0)) > 0)
		{	
			if(!(res = process_response(op_code, recv_buf, res)))
			{
				printf("%s:%s:%d: Failed process_response, error = %x\n", __FILE__, __func__, __LINE__, res);
			}
		}
		else printf("%s:%s:%d: Failed nfc_initiator_transceive_bytes, error = %x\n", __FILE__, __func__, __LINE__, res);
	}
	else printf("%s:%s:%d: Failed create_request, error = %x\n", __FILE__, __func__, __LINE__, res);

	return res;
}

const int NANO_SEC_IN_SECS = 1000000000;
static long secs_to_nano(long secs)
{
	return secs * NANO_SEC_IN_SECS;
}

static long get_elapsed_time_nanos(struct timespec begin, struct timespec end)
{
	return secs_to_nano(end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec);
}

#define N_RUNS 50
static void evaluate_operation(uint8_t op_code, uint32_t card_id, uint32_t value, char *send_buf, size_t send_buf_size, char *recv_buf, size_t recv_buf_size)
{
	int res = 1;
	
	struct timespec begin, end;
	long total_time, elapsed;
	total_time = elapsed = 0;

	// Purchase evaluation
	for(int i = 0; i < N_RUNS && res; i++)
	{
		elapsed = 0;		
		clock_gettime(CLOCK_MONOTONIC, &begin);
		//res = execute_operation(op_code, i, value, send_buf, send_buf_size, recv_buf, recv_buf_size);

		// Create request according to terminal arguments
		if((res = create_request(op_code, i, value, send_buf, &send_buf_size)))
		{
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = get_elapsed_time_nanos(begin, end);
			// Send request and receive response
			if((res = nfc_initiator_transceive_bytes(pnd, (uint8_t*)send_buf, send_buf_size, (uint8_t*)recv_buf, recv_buf_size, 0)) > 0)
			{
				clock_gettime(CLOCK_MONOTONIC, &begin);
				if(!(res = process_response(op_code, recv_buf, res)))
				{
					printf("%s:%s:%d: Failed process_response, error = %x\n", __FILE__, __func__, __LINE__, res);
				}
			}
			else printf("%s:%s:%d: Failed nfc_initiator_transceive_bytes, error = %x\n", __FILE__, __func__, __LINE__, res);
		}
		else printf("%s:%s:%d: Failed create_request, error = %x\n", __FILE__, __func__, __LINE__, res);

		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed += get_elapsed_time_nanos(begin, end);
		printf("%ld\n", elapsed);
		total_time += elapsed;
	}
	printf("Total time = %ld\n", total_time);
}

int main(int argc, char **argv)
{
	uint8_t op_code = atoi(argv[1]);
	uint32_t card_id = atoi(argv[2]);
	uint32_t value = atoi(argv[3]);

	// Initiate NFC
	nfc_init(&context);
	if(!context)
	{
		printf("%s:%d: Failed nfc_init\n", __func__, __LINE__);
		return -1;
	}

	pnd = nfc_open(context, NULL);
	if(!pnd)
	{
		printf("%s:%d: Failed nfc_open\n", __func__, __LINE__);
		nfc_exit(context);
		return -1;
	}

	// Start initiator

	int res = 0;

	if((res = nfc_initiator_init(pnd)) < 0)
	{
		printf("%s:%d: Failed nfc_initiator_init, error = %x\n", __func__, __LINE__, res);
		goto ERROR;
	}

	if((res = nfc_initiator_select_dep_target(pnd, NDM_PASSIVE, NBR_212, NULL, &target, 0)) < 0)
	{
		printf("%s:%d: Failed nfc_initiator_select_dep_target, error = %x\n", __func__, __LINE__, res);
		goto ERROR;
	}

	// HCE

	size_t recv_buf_size = 256;
	char *recv_buf = calloc(recv_buf_size, sizeof(char));

	size_t send_buf_size = 256;
	char *send_buf = calloc(send_buf_size, sizeof(char));

	remove("data_store");
	store = load_data_store();

	// Evaluate Purchase
	printf("PURCHASE Evaluation:\n");
	evaluate_operation(PURCHASE, card_id, 125, send_buf, send_buf_size, recv_buf, recv_buf_size);
	memset(send_buf, 0, send_buf_size);
	memset(recv_buf, 0, recv_buf_size);

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

ERROR:

	nfc_initiator_deselect_target(pnd);
	nfc_close(pnd);
	nfc_exit(context);

	save_data_store(store);

	return 0;
}
