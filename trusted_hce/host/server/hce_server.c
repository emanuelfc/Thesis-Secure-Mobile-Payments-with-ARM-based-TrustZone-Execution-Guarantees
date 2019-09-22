#include <hce.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <snfc.h>
#include <nfc.h>

#define MONTH_SECONDS 2628003

static int validate(hce_card *card)
{
	time_t current_date = time(NULL);	
	return card->expiration_date >= current_date && card->value > 0;
}

static void recharge(hce_card *card, uint32_t value)
{
	card->value += value;
}

static void use(hce_card *card, uint32_t value)
{
	card->value -= value;
}

static hce_card create_hce_card(uint32_t id, uint32_t expiration_date, uint32_t value)
{
	hce_card card;

	card.id = id;
	card.expiration_date = expiration_date;
	card.value = value;

	return card;
}

static void process_purchase(char *buf, char *write_buf, size_t *write_buf_size)
{
	// Write response code	
	uint8_t response_code = 1;	
	memcpy(write_buf, &response_code, sizeof(response_code));
	write_buf += sizeof(response_code);
	*write_buf_size = sizeof(response_code);

	// Create and Write hce card
	uint32_t value = *(uint32_t*)buf;
	static int new_card_id = 0;
	hce_card card = create_hce_card(new_card_id++, time(NULL) + MONTH_SECONDS, value);
	printf("[i] Purchased card:\n");
	print_card(&card);
	write_hce_card(&card, write_buf);

	*write_buf_size += sizeof(card);
}

static void process_validate(hce_card *card, char *write_buf, size_t *write_buf_size)
{
	// Write validate response code	
	uint8_t valid = (uint8_t)validate(card);
	memcpy(write_buf, &valid, sizeof(valid));
	write_buf += sizeof(valid);
	*write_buf_size = sizeof(valid);
}

static void process_use(hce_card *card, uint32_t use_price, char *write_buf, size_t *write_buf_size)
{
	process_validate(card, write_buf, write_buf_size);
	write_buf += *write_buf_size;

	if(validate(card))
	{
		use(card, use_price);
		*write_buf_size += write_hce_card(card, write_buf);
	}
}

static void process_recharge(hce_card *card, uint32_t recharge_value, char *write_buf, size_t *write_buf_size)
{
	printf("[i] Recharge Value = %d\n", recharge_value);
	process_validate(card, write_buf, write_buf_size);
	write_buf += *write_buf_size;

	if(validate(card))
	{	
		recharge(card, recharge_value);
		*write_buf_size += write_hce_card(card, write_buf);
	}
}

static int op_requires_card(uint8_t op_code)
{
	switch(op_code)
	{
		case VALIDATE:
		case USE:
		case RECHARGE:
			return 1;

		case PURCHASE:
		default:
			return 0;
	}
}

static void process_invalid_request(char *write_buf, size_t *write_buf_size)
{
	int invalid_code = -1;
	memcpy(write_buf, &invalid_code, sizeof(invalid_code));
	*write_buf_size = sizeof(invalid_code);
}

static void process_request(char *buf, char *write_buf, size_t *write_buf_size)
{
	uint8_t op_code = (uint8_t)*buf;
	buf += 	sizeof(op_code);

	printf("[i] Processing request with code = %d\n", op_code);

	hce_card card;
	int requires_card = op_requires_card(op_code);
	if(requires_card)
	{
		printf("[i] Received card:\n");		
		card = read_hce_card(buf);
		buf += sizeof(hce_card);
		print_card(&card);
	}
	
	switch(op_code)
	{
		case VALIDATE:
			printf("[i] Validate request\n");
			process_validate(&card, write_buf, write_buf_size);
			break;
		
		case PURCHASE:
			printf("[i] Purchase request\n");
			process_purchase(buf, write_buf, write_buf_size);
			break;

		case USE:
			printf("[i] Use request\n");
			process_use(&card, 1, write_buf, write_buf_size);
			break;

		case RECHARGE:
			printf("[i] Recharge request\n");
			process_recharge(&card, *(uint32_t*)buf, write_buf, write_buf_size);
			break;

		default:
			printf("%s:%s:%d: Invalid operation code, code = %d\n", __FILE__, __func__, __LINE__, op_code);
			process_invalid_request(write_buf, write_buf_size);
	}

	if(requires_card)
	{
		printf("[i] Card after operation:\n");
		print_card(&card);
	}

	printf("[i] Request processed\n");
}

int main(int argc, char **argv)
{
	nfc_target nt =
	{
		.nm =
		{
			.nmt = NMT_DEP,
			.nbr = NBR_UNDEFINED
		},
		.nti =
		{
			.ndi =
			{
				.abtNFCID3 = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff, 0x00, 0x00 },
				.szGB = 4,
				.abtGB = { 0x12, 0x34, 0x56, 0x78 },
				.ndm = NDM_UNDEFINED,
				// These bytes are not used by nfc_target_init: the chip will provide them automatically to the initiator
				.btDID = 0x00,
				.btBS = 0x00,
				.btBR = 0x00,
				.btTO = 0x00,
				.btPP = 0x01,
			},
		},
	};

#ifdef TRUSTED_HCE_SERVER
	snfc_socket s;
	s.nt = nt;
#else
	nfc_device *pnd;
	nfc_context *context;
#endif

	int res = 0;

	size_t recv_buf_cap = 1028;
	size_t recv_buf_size = recv_buf_cap;
	char *recv_buf = calloc(recv_buf_size, sizeof(char));

	size_t send_buf_size = 1028;
	char *send_buf = calloc(send_buf_size, sizeof(char));

#ifdef TRUSTED_HCE_SERVER
	
	if((res = snfc_target_init(&s)) != NFC_SUCCESS)
	{
		printf("%s:%s:%d Failed snfc_target_init, error = %x\n", __FILE__, __func__, __LINE__, res);
		return -1;
	}
	printf("[i] SNFC Target initiated\n");
#else
	nfc_init(&context);
	if(!context)
	{
		printf("%s:%s:%d Failed nfc_init\n", __FILE__, __func__, __LINE__);
		return -1;
	}
	printf("[i] NFC Context opened\n");

	pnd = nfc_open(context, NULL);
	if(!pnd)
	{
		printf("%s:%s:%d: Failed nfc_open\n", __FILE__, __func__, __LINE__);
		nfc_exit(context);
		return -1;
	}
	printf("NFC device: %s opened\n", nfc_device_get_name(pnd));
#endif


	while(1)
	{
		// Start connection with client
		do
		{
#ifdef TRUSTED_HCE_SERVER
			res = snfc_target_start(&s, 0);
#else
			res = nfc_target_init(pnd, &nt, (uint8_t*)recv_buf, recv_buf_cap, 0);
#endif			
		}
		while(res == 0xffffffff); // Minor bug fix since libnfc has a bug which only has a 5secs timeout
		if(res < 0)
		{
#ifdef TRUSTED_HCE_SERVER
			printf("%s:%s:%d: Failed snfc_target_start, error = %x\n", __FILE__, __func__, __LINE__, res);
#else
			printf("%s:%s:%d: Failed nfc_target_init, error = %x\n", __FILE__, __func__, __LINE__, res);
#endif			
			goto ERROR;
		}
		printf("[i] Target initiated and started\n");

		do
		{
			// Wait for client request

#ifdef TRUSTED_HCE_SERVER
			if((res = snfc_target_receive(&s, recv_buf, recv_buf_cap, 0)) < 0)
			{
				printf("%s:%d: Failed snfc_target_receive, error = %x\n", __FILE__, __func__, __LINE__, res);
				goto ERROR;
			}
#else
			if((res = nfc_target_receive_bytes(pnd, (uint8_t*)recv_buf, recv_buf_cap, 0)) < 0)
			{
				printf("%s:%s:%d: Failed nfc_target_receive_bytes, error = %x\n", __FILE__, __func__, __LINE__, res);
				goto ERROR;
			}
#endif

			// Process client request

			process_request(recv_buf, send_buf, &send_buf_size);

			// Send client response

#ifdef TRUSTED_HCE_SERVER
			if((res = snfc_target_send(&s, send_buf, send_buf_size, -1)) != send_buf_size)
			{
				printf("%s:%s:%d: Failed snfc_target_send, error = %x\n", __FILE__, __func__, __LINE__, res);
				goto ERROR;
			}
#else
			if((res = nfc_target_send_bytes(pnd, (uint8_t*)send_buf, send_buf_size, -1)) != send_buf_size)
			{
				printf("%s:%s:%d: Failed nfc_target_send_bytes, error = %x\n", __FILE__, __func__, __LINE__, res);
				goto ERROR;
			}
#endif
		}
		while(res >= 0);
	}

ERROR:

	free(recv_buf);
	free(send_buf);

#ifdef TRUSTED_HCE_SERVER
	snfc_target_close(&s);
#else
	nfc_abort_command(pnd);	
	nfc_close(pnd);
	nfc_exit(context);
#endif

	return 0;
}
