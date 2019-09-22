#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tee_client_api.h>
#include <tds_ta_entry.h>
#include <trusted_hce_ta_entry.h>

#define STRING(str) #str
#define CARD_ID_FIELD STRING(id)
#define CARD_EXPIRATION_DATE_FIELD STRING(expiration_date)
#define CARD_VALUE_FIELD STRING(value)

typedef struct hce_card
{
	uint32_t id;
	uint32_t expiration_date;
	uint32_t value;

} hce_card;

hce_card read_hce_card(char *buf)
{
	hce_card card;	

	card.id = *(uint32_t*)buf;
	buf += sizeof(card.id);

	card.expiration_date = *(uint32_t*)buf;
	buf += sizeof(card.expiration_date);

	card.value = *(uint32_t*)buf;

	return card;
}

size_t write_hce_card(hce_card *card, char *buf)
{
	memcpy(buf, &card->id, sizeof(card->id));
	buf += sizeof(card->id);

	memcpy(buf, &card->expiration_date, sizeof(card->expiration_date));
	buf += sizeof(card->expiration_date);

	memcpy(buf, &card->value, sizeof(card->value));
	buf += sizeof(card->value);

	return sizeof(hce_card);
}

static char* ui32toa(uint32_t value)
{
	size_t str_size = 33;	
	char *str = calloc(str_size, sizeof(char));
	snprintf(str, str_size, "%d", value);
	return str;
}

int main(int agrc, char* argv[])
{
	init_thce_ta();

	hce_card card1 =
	{
		.id = 123,
		.expiration_date = 999,
		.value = 5
	};

	size_t buf_size = sizeof(hce_card)+1;
	char *buf = calloc(buf_size, sizeof(char));

	printf("create_request PURCHASE 123 buffer:\n");
	create_request(1, 123, buf, &buf_size);
	for(int i = 0; i < buf_size; i++)
	{
		printf("%02x ", buf[i]);
	}
	printf("\n");

	memset(buf, 0, sizeof(hce_card)+1);

	uint8_t response = 1;
	memcpy(buf, &response, sizeof(response));
	buf += sizeof(response);
	write_hce_card(&card1, buf);

	process_response(1, buf-sizeof(response), buf_size);

	memset(buf, 0, sizeof(hce_card)+1);

	printf("create_request USE 123 buffer:\n");
	create_request(2, 123, buf, &buf_size);
	for(int i = 0; i < buf_size; i++)
	{
		printf("%02x ", buf[i]);
	}
	printf("\n");
	
	buf += sizeof(uint8_t);
	hce_card read_card = read_hce_card(buf);
	printf("read_card id = %d\n", read_card.id);
	printf("read_card exp = %d\n", read_card.expiration_date);
	printf("read_card value = %d\n", read_card.value);

	exit_thce_ta();
	
	return 0;
}
