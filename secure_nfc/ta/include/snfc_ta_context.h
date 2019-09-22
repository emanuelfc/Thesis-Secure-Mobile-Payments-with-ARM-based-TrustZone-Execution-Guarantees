#pragma once

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <snfc_associations.h>

typedef struct security_context
{
	security_association sa;		// Security association of the context
	key_exchange_association kea;		// Key exchange association
	TEE_ObjectHandle ecdh_key;		// ECDH key for the key exchange phase
	TEE_OperationHandle encrypt_handle;	// Cipher encrypt operation
	TEE_OperationHandle decrypt_handle;	// Cipher decrypt operation
	char *iv;
	TEE_OperationHandle mac_handle; 	// Digest operation
	size_t seq_n;

	TEE_ObjectHandle peer_pub_key;
	
} security_context;
