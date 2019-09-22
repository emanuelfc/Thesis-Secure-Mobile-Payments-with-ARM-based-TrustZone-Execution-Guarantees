#pragma once

#include <nfc.h>
#include <stddef.h>
#include <stdint.h>

// Flags
#define SNFC_FLAGS_AUTHENTICATION 1

typedef struct snfc_socket
{
	nfc_context *context;	
	nfc_device *device;
	nfc_target nt;
	
} snfc_socket;

// Common functions

int snfc_security_context_close();

// Initiator functions

int snfc_initiator_init(snfc_socket *s);
int snfc_initiator_start(snfc_socket *s, uint8_t spi, uint8_t kei, uint16_t flags, int timeout);
void snfc_initiator_close(snfc_socket *s);

int snfc_initiator_transceive_raw_bytes(snfc_socket *s, char *send_buf, size_t send_buf_size, char *recv_buf, size_t recv_buf_size, int timeout);
int snfc_initiator_transceive(snfc_socket *s, char *send_buf, size_t send_buf_size, char *recv_buf, size_t recv_buf_size, int timeout);

// Target functions

int snfc_target_init(snfc_socket *s);
int snfc_target_start(snfc_socket *s, int timeout);
void snfc_target_close(snfc_socket *s);

int snfc_target_send_raw_bytes(snfc_socket *s, char *payload, size_t payload_size, int timeout);
int snfc_target_send(snfc_socket *s, char *buf, size_t buf_size, int timeout);

int snfc_target_receive_raw_bytes(snfc_socket *s, char *buf, size_t buf_size, int timeout);
int snfc_target_receive(snfc_socket *s, char *buf, size_t buf_size, int timeout);
