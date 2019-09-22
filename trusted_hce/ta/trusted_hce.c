#include <trusted_hce.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <hce.h>
#include <snfc_ta_to_ta_entry.h>
#include <tds_ta_to_ta_entry.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static char* ui32toa(uint32_t value)
{
	size_t str_size = 33;	
	char *str = calloc(str_size, sizeof(char));
	snprintf(str, str_size, "%d", value);
	return str;
}

static int read_tds_hce_card(uint32_t id, hce_card *card)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	size_t size = 0;

	char *card_id_str = ui32toa(id);

	card->id = id;
		
	size = sizeof(card->expiration_date);
	if((res = tds_get(card_id_str, CARD_EXPIRATION_DATE_FIELD, (char*)&card->expiration_date, &size)) == TEE_SUCCESS)
	{	
		size = sizeof(card->value);			
		res = tds_get(card_id_str, CARD_VALUE_FIELD, (char*)&card->value, &size);
		if(res != TEE_SUCCESS) DMSG("%s:%s:%d: Failed tds_get card value, error = %x\n", __FILE__, __func__, __LINE__, res);
	}
	else DMSG("%s:%s:%d: Failed tds_get card expiration_date, error = %x\n", __FILE__, __func__, __LINE__, res);

	TEE_Free(card_id_str);

	return res;
}

static TEE_Result write_tds_hce_card(hce_card *card)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	char *card_id_str = ui32toa(card->id);

	if((res = tds_insert(card_id_str, CARD_ID_FIELD, &card->id, sizeof(card->id))) == TEE_SUCCESS)
	{
		if((res = tds_insert(card_id_str, CARD_EXPIRATION_DATE_FIELD, &card->expiration_date, sizeof(card->expiration_date))) == TEE_SUCCESS)
		{
			res = tds_insert(card_id_str, CARD_VALUE_FIELD, &card->value, sizeof(card->value));
			if(res != TEE_SUCCESS) DMSG("%s:%s:%d: Failed tds_insert card value, error = %x\n", __FILE__, __func__, __LINE__, res);
		}
		else DMSG("%s:%s:%d: Failed tds_insert card expiration_date, error = %x\n", __FILE__, __func__, __LINE__, res);
	}
	else DMSG("%s:%s:%d: Failed tds_insert card id, error = %x\n", __FILE__, __func__, __LINE__, res);

	TEE_Free(card_id_str);

	return res;
}

/*

This is strange... but hear me out...
Somehow OPTEE doesnt allow us to use the function read_hce_card which reads an hce card from a buffer
we have tested and it works... , the function works in qemu and works in other places of the code such as the server... we have copy-pasted the function into this file and it works.... literally copy pasting the code and changing nothing else....
It seems to not work if the function is in another file...
This function is a quick and dirty fix for a rather strange and unusual bug?...

*/
static void read_hce_card_ptr(char *buf, hce_card *card)
{
	card->id = *(uint32_t*)buf;
	buf += sizeof(card->id);

	card->expiration_date = *(uint32_t*)buf;
	buf += sizeof(card->expiration_date);

	card->value = *(uint32_t*)buf;
}

TEE_Result process_response(uint8_t op_code, char *response, size_t response_size)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	size_t buf_size = response_size;
	char *buf = calloc(response_size, sizeof(char));
	char *buf_begin = buf;

	if(buf)
	{	
		if((res = process_snfc_packet(response, response_size, buf, &buf_size)) == TEE_SUCCESS)
		{	
			uint8_t response_code = *(uint8_t*)buf;
			buf += sizeof(response_code);

			if(response_code)
			{
				hce_card card;
				char *card_id_str = NULL;

				switch(op_code)
				{
					case VALIDATE:
						res = TEE_SUCCESS;
						break;

					case PURCHASE:
						// Please read the comment above the read_hce_card_ptr function
						read_hce_card_ptr(buf, &card);
						res = write_tds_hce_card(&card);
						break;

					case USE:
					case RECHARGE:
						// Please read the comment above the read_hce_card_ptr function
						read_hce_card_ptr(buf, &card);
						card_id_str = ui32toa(card.id);
						res = tds_insert(card_id_str, CARD_VALUE_FIELD, &card.value, sizeof(card.value));
						TEE_Free(card_id_str);
						break;

					default:
						res = TEE_ERROR_GENERIC;
						DMSG("%s:%s:%d: Invalid operation code, op_code = %d\n", __FILE__, __func__, __LINE__, op_code);
				}	
			}
			else DMSG("%s:%s:%d: Invalid response_code, response_code = %d\n", __FILE__, __func__, __LINE__, response_code);

			buf -= sizeof(response_code);
		}
		else DMSG("%s:%s:%d: Failed process_snfc_packet, error = %d\n", __FILE__, __func__, __LINE__, res);
	
		TEE_Free(buf);
	}

	return res;
}

static uint32_t get_request_buffer_size(uint8_t op_code)
{
	uint32_t request_size = sizeof(uint8_t); // We must have an operation code

	switch(op_code)
	{
		case PURCHASE:
			request_size += sizeof(uint32_t); // For the extra arg
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
			DMSG("%s:%s:%d: Invalid operation code, op_code = %d\n", __FILE__, __func__, __LINE__, op_code);
			return 0;
	}
}

TEE_Result create_request(uint8_t op_code, uint32_t card_id, uint32_t value, char *write_buf, size_t *write_buf_size)
{
	if(!is_valid_request(op_code)) return TEE_ERROR_BAD_PARAMETERS;	

	TEE_Result res = TEE_ERROR_GENERIC;

	size_t request_size = get_request_buffer_size(op_code);
	char *request_buf = calloc(request_size, sizeof(char));
	char *request_buf_begin = request_buf;

	if(request_buf)
	{
		// Write operation code
		memcpy(request_buf, &op_code, sizeof(op_code));
		request_buf += sizeof(op_code);

		hce_card card;

		switch(op_code)
		{
			case PURCHASE:
				memcpy(request_buf, &value, sizeof(value));
				request_buf += sizeof(value);
				res = TEE_SUCCESS;
				break;

			case VALIDATE:
			case USE:
			case RECHARGE:
				if((res = read_tds_hce_card(card_id, &card)) == TEE_SUCCESS)
				{
					request_buf += write_hce_card(&card, request_buf);
					if(op_code == RECHARGE) memcpy(request_buf, &value, sizeof(value));
					res = TEE_SUCCESS;
				}
				break;

			default:
				res = TEE_ERROR_BAD_PARAMETERS;
				DMSG("%s:%s:%d: Invalid operation code, op_code = %d\n", __FILE__, __func__, __LINE__, op_code);
		}

		if(res == TEE_SUCCESS)
		{			
			res = create_snfc_packet(request_buf_begin, request_size, write_buf, write_buf_size);
		}

		TEE_Free(request_buf_begin);
	}

	return res;
}
