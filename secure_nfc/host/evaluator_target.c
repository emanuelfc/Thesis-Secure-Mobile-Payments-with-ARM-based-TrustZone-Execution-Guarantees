#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <snfc.h>
#include <snfc_ta.h>

#include <tee_client_api.h>

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

#ifdef SNFC
	snfc_socket s;
	s.nt = nt;
#else
	nfc_device *pnd;
	nfc_context *context;
#endif

	int res = 0;

	size_t buf_size = 256;
	char *buf = calloc(buf_size, sizeof(char));

#ifdef SNFC
	
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
		res = -1;
		while(res < 0)
		{
#ifdef SNFC
			res = snfc_target_start(&s, 0);
#else
			res = nfc_target_init(pnd, &nt, (uint8_t*)buf, buf_size, 0);
#endif
			if(res < 0)
			{
#ifdef SNFC
				printf("%s:%s:%d: Failed snfc_target_start, error = %x\n", __FILE__, __func__, __LINE__, res);
#else
				printf("%s:%s:%d: Failed nfc_target_init, error = %x\n", __FILE__, __func__, __LINE__, res);
#endif
			}
		}
		printf("[i] Target initiated and started\n");

		while(true)
		{
#ifdef SNFC		
			if((res = snfc_target_receive(&s, buf, buf_size, 0)) < 0)
			{
				printf("%s:%s:%d: Failed snfc_target_receive, error = %x\n", __FILE__, __func__, __LINE__, res);
				break;
			}
			if((res = snfc_target_send(&s, buf, res, 0)) < 0)
			{
				printf("%s:%s:%d: Failed snfc_target_send, error = %x\n", __FILE__, __func__, __LINE__, res);
				break;
			}
#else
			if((res = nfc_target_receive_bytes(pnd, (uint8_t*)buf, buf_size, 0)) < 0)
			{
				printf("%s:%s:%d: Failed nfc_target_receive_bytes, error = %x\n", __FILE__, __func__, __LINE__, res);
				break;
			}
			if((res = nfc_target_send_bytes(pnd, (uint8_t*)buf, res, 0)) < 0)
			{
				printf("%s:%s:%d: Failed nfc_target_send_bytes, error = %x\n", __FILE__, __func__, __LINE__, res);
				break;
			}
#endif
		}
	}

ERROR:

	free(buf);

#ifdef SNFC
	snfc_target_close(&s);
#else
	nfc_abort_command(pnd);	
	nfc_close(pnd);
	nfc_exit(context);
#endif

	return 0;
}
