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
	init_tds_ta();

	hce_card card1 =
	{
		.id = 123,
		.expiration_date = 999,
		.value = 5
	};

	/*char *buf = calloc(sizeof(hce_card)+1, sizeof(char));
	uint8_t response = 1;
	memcpy(buf, &response, sizeof(response));
	buf += sizeof(response);
	write_hce_card(&card1, buf);*/

	//process_response(1, buf, sizeof(hce_card)+1);

	size_t size = sizeof(uint32_t);
	uint32_t id = 0;
	uint32_t exp = 0;
	uint32_t value = 0;

	tds_insert(ui32toa(card1.id), CARD_ID_FIELD, &card1.id, sizeof(card1.id));
	tds_insert(ui32toa(card1.id), CARD_EXPIRATION_DATE_FIELD, &card1.expiration_date, sizeof(card1.expiration_date));
	tds_insert(ui32toa(card1.id), CARD_VALUE_FIELD, &card1.value, sizeof(card1.value));

	tds_get(ui32toa(card1.id), CARD_ID_FIELD, &id, &size);
	printf("id = %d\n", id);

	tds_get(ui32toa(card1.id), CARD_EXPIRATION_DATE_FIELD, &exp, &size);
	printf("exp = %d\n", exp);

	tds_get(ui32toa(card1.id), CARD_VALUE_FIELD, &value, &size);
	printf("value = %d\n", value);

	exit_tds_ta();

	init_tds_ta();

	id = 0;
	tds_get(ui32toa(card1.id), CARD_ID_FIELD, &id, &size);
	printf("id = %d\n", id);

	exp = 0;
	tds_get(ui32toa(card1.id), CARD_EXPIRATION_DATE_FIELD, &exp, &size);
	printf("exp = %d\n", exp);

	value = 0;
	tds_get(ui32toa(card1.id), CARD_VALUE_FIELD, &value, &size);
	printf("value = %d\n", value);

	exit_tds_ta();
	
	return 0;
}
