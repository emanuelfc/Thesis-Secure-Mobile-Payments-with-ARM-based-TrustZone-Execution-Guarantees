#include <nfc.h>
#include <snfc.h>
#include <snfc_ta_entry.h>
#include <tee_client_api.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define DEFAULT_BUFFER_SIZE 1024

// Common functions

int snfc_security_context_close()
{
	return security_context_close();
}

static int snfc_init(snfc_socket *s)
{
	TEEC_Result res = init_snfc_ta();
	if(res != TEEC_SUCCESS)
	{
		printf("%s:%s:%d: Failed init_snfc_ta, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
		return res;
	}
	
	nfc_init(&s->context);
	if(s->context)
	{	
		s->device = nfc_open(s->context, NULL);
		if(s->device)
		{		
			return NFC_SUCCESS;
		}
		else printf("%s:%s:%d: Failed nfc_open\n", __FILE__, __func__, __LINE__);

		nfc_exit(s->context);
	}
	else printf("%s:%s:%d: Failed nfc_init\n", __FILE__, __func__, __LINE__);
	
	return NFC_ESOFT;
}

static void snfc_cleanup(snfc_socket *s)
{
	nfc_close(s->device);
	nfc_exit(s->context);
	exit_snfc_ta();
}

// Initiator functions

int snfc_initiator_init(snfc_socket *s)
{	
	int res = TEEC_ERROR_GENERIC;	

	if((res = snfc_init(s)) == NFC_SUCCESS)
	{
		//nfc_initiator_init(s->device) >= 0 = GOOD	
		res = nfc_initiator_init(s->device);
		if(res < 0) printf("%s:%s:%d: Failed nfc_initiator_init, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
	}
	else printf("%s:%s:%d: Failed snfc_init, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
	
	return res;
}

static int initiator_handshake(snfc_socket *s, uint8_t spi, uint8_t kei, uint8_t flags)
{
	int res = TEEC_ERROR_GENERIC;
	
	size_t send_buf_cap = DEFAULT_BUFFER_SIZE;
	size_t send_buf_size = send_buf_cap;
	char *send_buf = calloc(send_buf_size, sizeof(char));

	size_t recv_buf_size = DEFAULT_BUFFER_SIZE;
	char *recv_buf = calloc(recv_buf_size, sizeof(char));

	if(send_buf && recv_buf)
	{
		if((res = set_security_parameters(spi, kei)) == TEEC_SUCCESS)
		{	
			if((res = create_handshake_request(spi, kei, flags, send_buf, &send_buf_size)) == TEEC_SUCCESS)
			{		
				if((res = nfc_initiator_transceive_bytes(s->device, (uint8_t*)send_buf, send_buf_size, (uint8_t*)recv_buf, recv_buf_size, 0)) > 0)
				{
					recv_buf_size = res;
					send_buf_size = send_buf_cap;
					res = process_handshake_reply(flags, recv_buf, recv_buf_size, send_buf, &send_buf_size);
					if(res != TEEC_SUCCESS) printf("%s:%s:%d: Failed process_handshake_reply, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);

					// Authentication is enabled
					if(flags && SNFC_FLAGS_AUTHENTICATION)
					{
						res = nfc_initiator_transceive_bytes(s->device, (uint8_t*)send_buf, send_buf_size, (uint8_t*)recv_buf, recv_buf_size, 2000);
						if(res == NFC_ETIMEOUT || res > 0) res = NFC_SUCCESS;
					}
				}
				else printf("%s:%s:%d: Failed nfc_initiator_transceive_bytes, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
			}
			else printf("%s:%s:%d: Failed create_handshake_request_message, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
		}
		else printf("%s:%s:%d: Failed set_communication_parameters, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);

		free(send_buf);
		free(recv_buf);
	}
	
	return res;
}

int snfc_initiator_start(snfc_socket *s, uint8_t spi, uint8_t kei, uint16_t flags, int timeout)
{
	int res = NFC_ESOFT;

	// Select a target and request active or passive mode for D.E.P. (Data Exchange Protocol)
	if((res = nfc_initiator_select_dep_target(s->device, NDM_PASSIVE, NBR_212, NULL, &s->nt, timeout)) > 0)
	{
		res = initiator_handshake(s, spi, kei, flags);
		if(res != NFC_SUCCESS) printf("%s:%s:%d: Failed initiator_handshake, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
	}
	else printf("%s:%s:%d: Failed nfc_initiator_select_dep_target, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
	
	return res;
}

void snfc_initiator_close(snfc_socket *s)
{
	if(nfc_initiator_deselect_target(s->device) < 0)
	{
		printf("Initiator could not deselect target");
	}

	snfc_cleanup(s);
}

int snfc_initiator_transceive_raw_bytes(snfc_socket *s, char *send_buf, size_t send_buf_size, char *recv_buf, size_t recv_buf_size, int timeout)
{
	return 	nfc_initiator_transceive_bytes(s->device, (uint8_t*)send_buf, send_buf_size, (uint8_t*)recv_buf, recv_buf_size, timeout);
}

int snfc_initiator_transceive(snfc_socket *s, char *send_buf, size_t send_buf_size, char *recv_buf, size_t recv_buf_size, int timeout)
{
	int res = TEEC_ERROR_GENERIC;

	size_t send_packet_size = send_buf_size + DEFAULT_BUFFER_SIZE;
	char *send_packet = calloc(send_packet_size, sizeof(char));

	if(send_packet)
	{
		if((res = create_snfc_packet(send_buf, send_buf_size, send_packet, &send_packet_size)) == TEEC_SUCCESS)
		{
			size_t recv_packet_size = DEFAULT_BUFFER_SIZE;
			char *recv_packet = calloc(recv_packet_size, sizeof(char));

			if(recv_packet)
			{
				if((res = snfc_initiator_transceive_raw_bytes(s, send_packet, send_packet_size, recv_packet, recv_packet_size, timeout)) > 0)
				{		
					res = process_snfc_packet(recv_packet, res, recv_buf, &recv_buf_size);
					if(res == TEEC_SUCCESS) res = recv_buf_size;
					else printf("%s:%s:%d Failed process_snfc_packet, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
				}
				else printf("%s:%s:%d: Failed nfc_initiator_transceive_bytes, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);

				free(recv_packet);
			}
			else printf("%s:%s:%d: Failed recv_packet allocation, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
		}
		else printf("%s:%s:%d: Failed create_communication_packet, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);

		free(send_packet);
	}
	

	return res;
}

// Target functions

int snfc_target_init(snfc_socket *s)
{
	return snfc_init(s);
}

static int target_handshake(snfc_socket *s)
{
	int res = TEEC_ERROR_GENERIC;
	
	size_t send_buf_size = DEFAULT_BUFFER_SIZE;
	char *send_buf = calloc(send_buf_size, sizeof(char));

	size_t recv_buf_cap = DEFAULT_BUFFER_SIZE;
	size_t recv_buf_size = recv_buf_cap;
	char *recv_buf = calloc(recv_buf_cap, sizeof(char));

	if(send_buf && recv_buf)
	{	
		// Wait for initiator handshake request
		if((res = nfc_target_receive_bytes(s->device, (uint8_t*)recv_buf, recv_buf_size, 0)) > 0)
		{
			uint8_t flags = (uint8_t)(recv_buf[2]);
			if((res = process_handshake_request(recv_buf, recv_buf_size, send_buf, &send_buf_size)) == TEEC_SUCCESS)
			{		
				if((res = nfc_target_send_bytes(s->device, (uint8_t*)send_buf, send_buf_size, 0)) == send_buf_size)
				{
					if(flags && SNFC_FLAGS_AUTHENTICATION)
					{			
						recv_buf_size = recv_buf_cap;
						sleep(1);					
						if((res = nfc_target_receive_bytes(s->device, (uint8_t*)recv_buf, recv_buf_size, 0)) > 0)
						{
							recv_buf_size = res;

							/* 
								Sending back a response is not needed!!

								This is strange but without sending back a packet 
								(doesnt matter the data or amount)
								both the initiator and target cannot communicate again with one another.
								
								I have tested by trying to send data from the initiator to the target
								and from the target to the initiator and all it does is return either
								a timeout error or a transmission error.

								After many hours of testing and debugging, i've put this line below and it finally started working, but it's theoretically not needed. Even the architecture and protocol specification does not require sending back a response.
								I attribute this weird bug/something to the environment itself, as i've encountered other similar strange bugs, such as the read_hce_card bug, or timeouts not even working in the libNFC library...
								Speaking of timeouts not working, nfc_initiator_transceive_bytes with a timeout of -1 is not the default timeout as specified in the ddocumentation, instead it blocks indefinitely. But -1 is the default timeout for all the other functions... And by the way... the 0 timeout, which is supposed to be the blocks indefinitely timeout, DOES NOT WORK!!!!
							
							*/
							nfc_target_send_bytes(s->device, (uint8_t*)recv_buf, 10, 2000);

							res = process_authentication_proof(recv_buf, recv_buf_size);
							if(res != TEEC_SUCCESS)	printf("%s:%s:%d: Failed process_authentication_proof, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
							res = NFC_SUCCESS;
						}
						else printf("%s:%s:%d: Failed nfc_target_receive_bytes (receive authentication proof), error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
					}
					else res = TEEC_SUCCESS;
				}
				else printf("%s:%s:%d: Failed nfc_target_send_bytes (send handshake reply), error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
			}
			else printf("%s:%s:%d: Failed process_handshake_request, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
		}

		free(send_buf);
		free(recv_buf);
	}
	
	return res;
}

int snfc_target_start(snfc_socket *s, int timeout)
{
	int res = TEEC_ERROR_GENERIC;

	size_t buf_size = DEFAULT_BUFFER_SIZE;
	char *buf = calloc(buf_size, sizeof(char));

	if(buf)
	{	
		if((res = nfc_target_init(s->device, &s->nt, (uint8_t*)buf, buf_size, timeout)) > 0)
		{			
			res = target_handshake(s);
			if(res != TEEC_SUCCESS && res != 0xffffffff) printf("%s:%s:%d: Failed target_handshake, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res); // 0xffffffff is an USB error from libUSB that happens when it doesnt contain anything in the buffer, this fix makes it so it doesnt spam the console
		}
		else if(res != 0xffffffff) printf("%s:%s:%d: Failed nfc_target_init, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res); // 0xffffffff is an USB error from libUSB that happens when it doesnt contain anything in the buffer, this fix makes it so it doesnt spam the console

		free(buf);
	}

	return res;
}

void snfc_target_close(snfc_socket *s)
{
	snfc_cleanup(s);
}

int snfc_target_send_raw_bytes(snfc_socket *s, char *payload, size_t payload_size, int timeout)
{	
	return nfc_target_send_bytes(s->device, (uint8_t*)payload, payload_size, timeout);
}

int snfc_target_send(snfc_socket *s, char *send_buf, size_t send_buf_size, int timeout)
{	
	int res = TEEC_ERROR_GENERIC;
	
	size_t packet_size = send_buf_size + DEFAULT_BUFFER_SIZE;
	char *packet = calloc(packet_size, sizeof(char));

	if(packet)
	{
		if((res = create_snfc_packet(send_buf, send_buf_size, packet, &packet_size)) == TEEC_SUCCESS)
		{	
			res = snfc_target_send_raw_bytes(s, packet, packet_size, timeout);
			if(res != packet_size) printf("%s:%s:%d: Failed nfc_target_send_bytes, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
			else res = send_buf_size;
		}
		else printf("%s:%s:%d: Failed create_communication_packet, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
		
		free(packet);
	}
	
	return res;
}

int snfc_target_receive_raw_bytes(snfc_socket *s, char *buf, size_t buf_size, int timeout)
{
	int recv_bytes = nfc_target_receive_bytes(s->device, (uint8_t*)buf, buf_size, timeout);
	if(recv_bytes <= 0) printf("%s:%s:%d: Failed nfc_target_receive_bytes, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, recv_bytes, recv_bytes);
	return recv_bytes;
}

int snfc_target_receive(snfc_socket *s, char *buf, size_t buf_size, int timeout)
{
	int recv_bytes = NFC_ESOFT;	

	// Allocate enough memory to hold a full SNFC packet
	// It includes header + payload
	size_t packet_size = DEFAULT_BUFFER_SIZE;
	char *packet = calloc(packet_size, sizeof(char));

	if(packet)
	{
		recv_bytes = snfc_target_receive_raw_bytes(s, packet, packet_size, timeout);
	
		if(recv_bytes > 0)
		{
			// Parse communication header
			int teec_res = process_snfc_packet(packet, recv_bytes, buf, &recv_bytes);	
			if(teec_res != TEEC_SUCCESS)
			{
				recv_bytes = teec_res;
				printf("%s:%s:%d: Failed process_snfc_packet, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, teec_res, teec_res);
			}
		}
		else printf("%s:%s:%d: Failed snfc_target_receive_raw_bytes, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, recv_bytes, recv_bytes);

		free(packet);
	}
	
	return recv_bytes;
}
