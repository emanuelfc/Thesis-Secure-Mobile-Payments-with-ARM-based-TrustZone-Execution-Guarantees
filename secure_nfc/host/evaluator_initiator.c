#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>

#include <tee_client_api.h>
#include <snfc_ta.h>
#include <snfc.h>
#include <nfc.h>

#ifdef SNFC

static snfc_socket s;

#else

static nfc_device *pnd;
static nfc_context *context;
static nfc_target target;

#endif

const int NANO_SEC_IN_SECS = 1000000000;
static long secs_to_nano(long secs)
{
	return secs * NANO_SEC_IN_SECS;
}

static long get_elapsed_time_nanos(struct timespec begin, struct timespec end)
{
	return secs_to_nano(end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec);
}

#define N_RUNS 10
int main(int agrc, char* argv[])
{
	int res = 0;

#ifdef SNFC
	uint8_t spi = atoi(argv[1]);
	uint8_t kei = atoi(argv[2]);
	uint8_t flags = 0;

	// Initiate SNFC
	if((res = snfc_initiator_init(&s)) != NFC_SUCCESS)
	{
		printf("%s:%d: Failed snfc_initiator_init\n", __func__, __LINE__);
		return -1;
	}

	struct timespec begin, end;
	for(int i = 0; i < N_RUNS; i++)
	{
		long elapsed = 0;		
		clock_gettime(CLOCK_MONOTONIC, &begin);		
		// Start initiator SNFC
		if((res = snfc_initiator_start(&s, spi, kei, flags, 0)) < 0)
		{
			printf("%s:%d: Failed snfc_initiator_start, error = %x\n", __func__, __LINE__, res);
			exit(-1);
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		elapsed = get_elapsed_time_nanos(begin, end);
		printf("%ld\n", elapsed);
		sleep(2);
	}
#else
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
#endif

ERROR:

#ifdef SNFC
	snfc_initiator_close(&s);
#else
	nfc_initiator_deselect_target(pnd);
	nfc_close(pnd);
	nfc_exit(context);
#endif

	return 0;
}
