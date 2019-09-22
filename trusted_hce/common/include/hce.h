#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct hce_card
{
	uint32_t id;
	uint32_t expiration_date;
	uint32_t value;

} hce_card;

#define VALIDATE 0
#define PURCHASE 1
#define USE	 2
#define RECHARGE 3

#define STRING(str) #str
#define CARD_ID_FIELD STRING(id)
#define CARD_EXPIRATION_DATE_FIELD STRING(expiration_date)
#define CARD_VALUE_FIELD STRING(value)

void print_card(hce_card *card);

hce_card read_hce_card(char *buf);
size_t write_hce_card(hce_card *card, char *buf);
