#include <hce.h>
#include <string.h>
#include <stdio.h>

void print_card(hce_card *card)
{
	printf("Card ID: %d\n", card->id);
	printf("Card Expiration Date: %d\n", card->expiration_date);
	printf("Card Value: %d\n", card->value);
}

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
