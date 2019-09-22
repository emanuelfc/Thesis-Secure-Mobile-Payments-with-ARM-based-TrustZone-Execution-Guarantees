#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <hce.h>
#include <nfc.h>

#include <data_store_storage.h>
#include <data_store.h>

static hce_card dummy_card =
{
	.id = 0,
	.expiration_date = 999999999,
	.value = 999
};

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

static void write_data_store_hce_card(hce_card *card)
{
	char *str_id = ui32toa(card->id);

	data_store_insert(store, str_id, CARD_ID_FIELD, (unsigned char*)&card->id, sizeof(card->id));
	data_store_insert(store, str_id, CARD_EXPIRATION_DATE_FIELD, (unsigned char*)&card->expiration_date, sizeof(card->expiration_date));
	data_store_insert(store, str_id, CARD_VALUE_FIELD, (unsigned char*)&card->value, sizeof(card->value));
	
	free(str_id);
}

static hce_card read_data_store_hce_card(uint32_t id)
{
	hce_card card;

	char *str_id = calloc(10, 1);
	sprintf(str_id, "%d", id);

	card.id = id;
	card.expiration_date = *(uint32_t*)(data_store_get(store, str_id, CARD_EXPIRATION_DATE_FIELD)->value);
	card.value = *(uint32_t*)(data_store_get(store, str_id, CARD_VALUE_FIELD)->value);

	free(str_id);
	
	return card;
}

/*static void create_request(int argc, char **argv, char *write_buf, size_t *write_buf_size)
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
			//card = read_data_store_hce_card(extra_arg);
			card = dummy_card;
			*write_buf_size += write_hce_card(&card, write_buf);
			break;

		default:
			printf("%s:%d: Invalid operation code, code = %d\n", __func__, __LINE__, op_code);
	}
}*/

static bool create_request(uint8_t op_code, uint32_t card_id, uint32_t value, char *write_buf, size_t *write_buf_size)
{
	bool res = false;

	if(!is_valid_request(op_code)) return res;	

	*write_buf_size = get_request_buffer_size(op_code);

	// Write operation code
	memcpy(write_buf, &op_code, sizeof(op_code));
	write_buf += sizeof(op_code);

	hce_card card;

	switch(op_code)
	{
		case PURCHASE:
			memcpy(write_buf, &value, sizeof(value));
			write_buf += sizeof(value);
			res = true;
			break;

		case VALIDATE:
		case USE:
		case RECHARGE:
			card = read_data_store_hce_card(card_id, &card);
			write_buf += write_hce_card(&card, write_buf);
			if(op_code == RECHARGE) memcpy(write_buf, &value, sizeof(value));
			res = true;
			break;

		default:
			res = false;
			printf("%s:%s:%d: Invalid operation code, op_code = %d\n", __FILE__, __func__, __LINE__, op_code);
	}

	return res;
}

static bool process_response(uint8_t op_code, char *response, size_t response_size)
{
	bool res = false;

	uint8_t response_code = *(uint8_t*)response;
	response += sizeof(response_code);

	if(response_code)
	{
		hce_card card;
		char *card_id_str = NULL;

		switch(op_code)
		{
			case VALIDATE:
				res = true;
				break;

			case PURCHASE:
				card = read_hce_card(response);
				write_data_store_hce_card(&card);
				res = true;
				break;

			case USE:
			case RECHARGE:
				card = read_hce_card(response);
				card_id_str = ui32toa(card.id);
				data_store_insert(store, str_id, CARD_VALUE_FIELD, (unsigned char*)&card->value, sizeof(card->value));
				free(card_id_str);
				res = true;
				break;

			default:
				res = false;
				printf("%s:%s:%d: Invalid operation code, op_code = %d\n", __FILE__, __func__, __LINE__, op_code);
		}	
	}
	else printf("%s:%s:%d: Invalid response_code, response_code = %d\n", __FILE__, __func__, __LINE__, response_code);

	return res;
}

int main(int argc, char **argv)
{
	store = load_data_store();

	int res = 0;

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
	//printf("NFC device: %s opened\n", nfc_device_get_name(pnd));

	// Start initiator

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

	size_t recv_buf_size = 1028;
	char *recv_buf = calloc(recv_buf_size, sizeof(char));

	size_t send_buf_size = 1028;
	char *send_buf = calloc(send_buf_size, sizeof(char));

	uint8_t op_code = atoi(argv[1]);
	uint32_t card_id = atoi(argv[2]);
	uint32_t value = argc == 4 ? atoi(argv[3]) : 0;


	// Create request according to terminal arguments
	create_request(op_code, card_id, value, send_buf, &send_buf_size);

	// Send request and receive response
	if((res = nfc_initiator_transceive_bytes(pnd, (uint8_t*)send_buf, send_buf_size, (uint8_t*)recv_buf, recv_buf_size, 0)) < 0)
	{
		printf("%s:%d: Failed nfc_initiator_transceive_bytes, error = %x\n", __func__, __LINE__, res);
		goto ERROR;
	}
	
	//recv_buf_size = res;

	process_response(op_code, recv_buf, res);

	nfc_initiator_deselect_target(pnd);

ERROR:

	nfc_close(pnd);
	nfc_exit(context);

	return 0;
}
