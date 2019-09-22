#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <data_store.h>
#include <data_store_storage.h>

static data_store *store;

typedef struct hce_card
{
	uint32_t id;
	uint32_t expiration_date;
	uint32_t value;

} hce_card;

static char* ui32toa(uint32_t value)
{
	char *str = calloc(33, sizeof(char));
	sprintf(str, "%d", value);
	return str;
}


static hce_card read_hce_card(uint32_t id)
{
	size_t unused = 0;

	hce_card card;

	char *card_id_str = ui32toa(id);

	data *d;

	d = data_store_get(store, card_id_str, "id");
	card.id = *(int*)d->value;

	d = data_store_get(store, card_id_str, "expiration_date");
	card.expiration_date = *(int*)d->value;

	d = data_store_get(store, card_id_str, "value");
	card.value = *(int*)d->value;

	free(card_id_str);

	return card;
}

static void write_hce_card(hce_card *card)
{
	char *card_id_str = ui32toa(card->id);

	data_store_insert(store, card_id_str, "id", &card->id, sizeof(card->id));
	data_store_insert(store, card_id_str, "expiration_date", &card->expiration_date, sizeof(card->expiration_date));
	data_store_insert(store, card_id_str, "value", &card->value, sizeof(card->value));

	free(card_id_str);
}

int main(int argc, char **argv)
{
	store = create_data_store();

	// Insert card1
	hce_card card1 =
	{
		.id = 1,
		.expiration_date = 123,
		.value = 5
	};
	write_hce_card(&card1);

	// Insert card2
	hce_card card2 =
	{
		.id = 2,
		.expiration_date = 456,
		.value = 10
	};
	write_hce_card(&card2);

	// Insert card3
	hce_card card3 =
	{
		.id = 3,
		.expiration_date = 678,
		.value = 15
	};
	write_hce_card(&card3);

	// Save
	save_data_store(store);

	// Load
	store = load_data_store();

	// Check card1
	hce_card c = read_hce_card(card1.id);
	printf("card1 id = %d : %d\n", card1.id, c.id);
	printf("card1 expiration_date = %d : %d\n", card1.expiration_date, c.expiration_date);
	printf("card1 value = %d : %d\n", card1.value, c.value);
	assert(card1.id == c.id);
	assert(card1.expiration_date == c.expiration_date);
	assert(card1.value == c.value);

	// Check card2
	c = read_hce_card(card2.id);
	assert(card2.id == c.id);
	assert(card2.expiration_date == c.expiration_date);
	assert(card2.value == c.value);

	// Check card3
	c = read_hce_card(card3.id);
	assert(card3.id == c.id);
	assert(card3.expiration_date == c.expiration_date);
	assert(card3.value == c.value);

	char *card_id_str = ui32toa(card1.id);
	assert(data_store_delete_field(store, card_id_str, "value"));
	assert(data_store_delete_field(store, card_id_str, "value") != true);
	free(card_id_str);

	card_id_str = ui32toa(card2.id);
	assert(data_store_delete_field(store, card_id_str, "value"));
	assert(data_store_delete_field(store, card_id_str, "value") != true);
	free(card_id_str);

	card_id_str = ui32toa(card3.id);
	assert(data_store_delete_field(store, card_id_str, "value"));
	assert(data_store_delete_field(store, card_id_str, "value") != true);
	free(card_id_str);
}
