#pragma once

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

uint32_t alg_to_type(uint32_t alg);

void set_flag(uint16_t value, int pos, uint16_t *flags);
int get_flag(int pos, uint16_t flags);

void* write_buffer(void *src, void *dst, const size_t num);
size_t write_tee_object_ref_attribute(TEE_ObjectHandle obj, uint32_t attribute, char *buf);

TEE_Result create_digest_signature(int signature_alg, TEE_ObjectHandle key, size_t key_size, char *digest, size_t digest_len, char *signature, size_t *signature_len);
TEE_Result verify_digest_signature(int signature_alg, TEE_ObjectHandle key, size_t key_size, char *digest, size_t digest_len, char *signature, size_t signature_len);

TEE_Result create_digest(int digest_alg, char *msg, size_t msg_len, char* digest, size_t *digest_len);

TEE_Result read_ecc_public_key(char *buf, int key_type, size_t key_size, int ecc_curve, TEE_ObjectHandle *ecc_key);

TEE_Result get_certificate_keypair(TEE_ObjectHandle *keypair, char *pub_key, size_t key_size, char *priv_key, int curve);

void print_ta_hex(char *buf, size_t buf_size);
