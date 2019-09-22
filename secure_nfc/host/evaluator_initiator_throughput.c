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

#define N_RUNS 50
void measure_throughput(size_t bits_per_packet)
{
	int res = 0;

	size_t buf_size = bits_per_packet / 8;	
	char *buf = calloc(buf_size, sizeof(char));

	struct timespec begin, end;
	long total_time, elapsed;
	total_time = elapsed = 0;

	double total_bytes_tx = 0;

	for(int i = 0; i < N_RUNS; i++)
	{
		clock_gettime(CLOCK_MONOTONIC, &begin);
#ifdef SNFC
		res = snfc_initiator_transceive(&s, buf, buf_size, buf, buf_size, 0);
#else
		res = nfc_initiator_transceive_bytes(pnd, (uint8_t*)buf, buf_size, (uint8_t*)buf, buf_size, 0);
#endif
		clock_gettime(CLOCK_MONOTONIC, &end);

		if(res != buf_size)
		{
			printf("%s:%d: Failed transceive, error = %x\n", __func__, __LINE__, res);
			exit(-1);
		}

		elapsed = get_elapsed_time_nanos(begin, end);
		printf("%ld\n", elapsed);
		total_time += elapsed;
		total_bytes_tx += buf_size * 2;
	}
	printf("Runs = %d\n", N_RUNS);
	printf("Total bytes transmitted = %lf\n", total_bytes_tx);
	printf("Throughput (bps) = %lf\n", (total_bytes_tx * 8) / (total_time / NANO_SEC_IN_SECS));
	free(buf);
}

int main(int agrc, char* argv[])
{
	int res = 0;

#ifdef SNFC
	uint8_t spi = atoi(argv[1]);
	uint8_t kei = 0;
	uint8_t flags = 0;

	// Initiate SNFC
	if((res = snfc_initiator_init(&s)) != NFC_SUCCESS)
	{
		printf("%s:%d: Failed snfc_initiator_init\n", __func__, __LINE__);
		return -1;
	}

	// Start initiator SNFC
	if((res = snfc_initiator_start(&s, spi, kei, flags, 0)) < 0)
	{
		printf("%s:%d: Failed snfc_initiator_start, error = %x\n", __func__, __LINE__, res);
		goto ERROR;
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

	int nfc_bitrate_list[] = {8, 16, 32, 40, 64, 104, 128, 136, 256, 512, 1024, 1600, 2000};	// For NFC
	int snfc_bitrate_list[] = {8, 16, 32, 40, 64, 104, 128, 136, 256, 512, 1024, 1600};		// For SNFC

	int *bitrate_list;
	int bitrate_list_size = 0;

#ifdef SNFC
	printf("SPI = %d\n", spi);
	
	bitrate_list = snfc_bitrate_list;
	bitrate_list_size = sizeof(snfc_bitrate_list)/sizeof(int);
#else
	bitrate_list = nfc_bitrate_list;
	bitrate_list_size = sizeof(nfc_bitrate_list)/sizeof(int);
#endif

	printf("Bitrate = %d\n", 1600);
	measure_throughput(1600);

	/*
	for(int i = 0; i < bitrate_list_size; i++)
	{
		printf("Bitrate = %d\n", bitrate_list[i]);
		measure_throughput(bitrate_list[i]);
	}
	*/

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
