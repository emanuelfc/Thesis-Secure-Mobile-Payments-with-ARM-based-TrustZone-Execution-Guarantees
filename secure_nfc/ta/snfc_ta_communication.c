#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <snfc_ta_communication.h>
#include <snfc_ta_context.h>
#include <snfc_ta_utils.h>
#include <stdlib.h>
#include <string.h>

#define MAX_IV_SYNC_ATTEMPTS 10

static TEE_Result create_next_iv(char *iv, size_t iv_len, char *next_iv)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	size_t bigint_iv_len = TEE_BigIntSizeInU32(iv_len * 8);
	TEE_BigInt *bigint_iv = calloc(bigint_iv_len, sizeof(TEE_BigInt));

	if(bigint_iv)
	{
		TEE_BigIntInit(bigint_iv, bigint_iv_len);

		if((res = TEE_BigIntConvertFromOctetString(bigint_iv, iv, iv_len, 0)) == TEE_SUCCESS)
		{
			TEE_BigInt *bigint_inc = calloc(1, sizeof(TEE_BigInt));
			if(bigint_inc)
			{
				TEE_BigIntInit(bigint_inc, sizeof(TEE_BigInt));
				TEE_BigIntConvertFromS32(bigint_inc, 1);

				TEE_BigInt *bigint_next_iv = calloc(bigint_iv_len, sizeof(TEE_BigInt));
				if(bigint_next_iv)
				{
					TEE_BigIntInit(bigint_next_iv, bigint_iv_len);

					TEE_BigIntAdd(bigint_next_iv, bigint_iv, bigint_inc);

					res = TEE_BigIntConvertToOctetString(next_iv, &iv_len, bigint_next_iv);

					TEE_Free(bigint_next_iv);
				}
				
				free(bigint_inc);
			}
		}

		free(bigint_iv);
	}

	return res;
}

static TEE_Result sync_iv(security_context *ctx, char next_iv_check)
{
	TEE_Result res = TEE_ERROR_BAD_STATE;

	size_t iv_len = ctx->sa.block_size / 8;
	
	for(int i = 0; i < MAX_IV_SYNC_ATTEMPTS && ctx->iv[iv_len - 1] != next_iv_check; i++)
	{
		DMSG("IV syncing attempt %d\n", i);
		res = create_next_iv(ctx->iv, iv_len, ctx->iv);
	}

	if(ctx->iv[iv_len - 1] == next_iv_check)
	{
		res = TEE_SUCCESS;
	}

	if(res != TEE_SUCCESS) DMSG("%s:%s:%d: Failed sync_iv, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);

	return res;
}

static TEE_Result perform_authenticated_cipher_operation(TEE_OperationMode mode, TEE_OperationHandle cipher_op, char *iv, size_t iv_size, char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size)
{
	TEE_Result res;
	size_t tag_len = iv_size;
	if((res = TEE_AEInit(cipher_op, iv, iv_size, iv_size * 8, 0, buf_size)) == TEE_SUCCESS)
	{
		if(mode == TEE_MODE_ENCRYPT)
		{
			res = TEE_AEEncryptFinal(cipher_op, buf, buf_size, write_buf + tag_len, write_buf_size, write_buf, &tag_len);
			*write_buf_size += tag_len;
			if(res != TEE_SUCCESS) DMSG("%s:%s:%d: Failed TEE_AEEncryptFinal, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
		}
		else // mode == TEE_MODE_DECRYPT
		{
			res = TEE_AEDecryptFinal(cipher_op, buf + tag_len, buf_size - tag_len, write_buf, write_buf_size, buf, tag_len);
			if(res != TEE_SUCCESS) DMSG("%s:%s:%d: Failed TEE_AEDecryptFinal, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
		}
	}
	else DMSG("%s:%s:%d: Failed TEE_AEInit, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
	
	return res;
}

static TEE_Result perform_cipher_operation(TEE_OperationHandle cipher_op, char *iv, size_t iv_size, char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size)
{
	TEE_Result res;
	TEE_CipherInit(cipher_op, iv, iv_size);
	res = TEE_CipherDoFinal(cipher_op, buf, buf_size, write_buf, write_buf_size);
	if(res != TEE_SUCCESS) DMSG("%s:%s:%d: Failed TEE_CipherDoFinal, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
	return res;
}

static size_t write_snfc_packet(snfc_packet c, security_context *ctx, char *buf, size_t payload_size)
{
	char *buf_start = buf;	
	
	buf = write_buffer(&c.seq_n, buf, sizeof(c.seq_n));
	buf = write_buffer(&c.iv_check_value, buf, sizeof(c.iv_check_value));
	buf = write_buffer(c.payload, buf, payload_size);
	buf = write_buffer(c.icv, buf, ctx->sa.mac_len / 8);

	return (buf - buf_start);
}

static snfc_packet new_snfc_packet(security_context *ctx, char *payload, char *icv, char iv_check_value)
{
	snfc_packet c;

	c.seq_n = ctx->seq_n;
	c.iv_check_value = iv_check_value;
	c.payload = payload;
	c.icv = icv;

	return c;
}

TEE_Result create_integrity_check_value(security_context *ctx, snfc_packet c, size_t payload_size, char *icv, size_t *icv_len)
{
	TEE_Result res = TEE_ERROR_GENERIC;	
	TEE_MACInit(ctx->mac_handle, NULL, 0);
	TEE_MACUpdate(ctx->mac_handle, &c.seq_n, sizeof(c.seq_n));
	TEE_MACUpdate(ctx->mac_handle, &c.iv_check_value, sizeof(c.seq_n));
	res = TEE_MACComputeFinal(ctx->mac_handle, c.payload, payload_size, icv, icv_len);
	if(res != TEE_SUCCESS) DMSG("%s:%s:%d: Failed TEE_MACComputeFinal, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);

	return res;
}

TEE_Result create_snfc_packet(security_context *ctx, char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size)
{	
	TEE_Result res = TEE_ERROR_GENERIC;

	snfc_packet c;

	size_t payload_size = buf_size + (ctx->sa.block_size - (buf_size % ctx->sa.block_size));
	size_t icv_len = ctx->sa.mac_len / 8;
	size_t iv_len = ctx->sa.block_size / 8;

	size_t packet_size = sizeof(c.seq_n) + sizeof(c.iv_check_value) + payload_size + icv_len;

	if(*write_buf_size < packet_size)
	{
		res = TEE_ERROR_SHORT_BUFFER;
		DMSG("%s:%s:%d: Failed create_snfc_packet, write_buf is too short - require %d bytes but got %d bytes\n", __FILE__, packet_size, *write_buf_size);
		return res;
	}

	TEE_MemFill(write_buf, 0, packet_size);

	char *payload = write_buf + sizeof(c.seq_n) + sizeof(c.iv_check_value);

	if(ctx->sa.cipher_alg == TEE_ALG_AES_GCM)
	{
		res = perform_authenticated_cipher_operation(TEE_MODE_ENCRYPT, ctx->encrypt_handle, ctx->iv, iv_len, buf, buf_size, payload, &payload_size);
	}
	else res = perform_cipher_operation(ctx->encrypt_handle, ctx->iv, iv_len, buf, buf_size, payload, &payload_size);
	
	if(res == TEE_SUCCESS)
	{			
		c = new_snfc_packet(ctx, payload, NULL, ctx->iv[iv_len-1]);
		c.icv = calloc(icv_len, sizeof(char));

		if(c.icv && ((res = create_integrity_check_value(ctx, c, payload_size, c.icv, &icv_len)) == TEE_SUCCESS))
		{
			*write_buf_size = write_snfc_packet(c, ctx, write_buf, payload_size);

			ctx->seq_n++;

			res = create_next_iv(ctx->iv, iv_len, ctx->iv);
		}
		else DMSG("%s:%s:%d: Failed icv allocaiton or create_integrity_check_value, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
	}
	else DMSG("%s:%s:%d: Failed perform_cipher_operation, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
	
	return res;
}

/*TEE_Result create_snfc_packet(security_context *ctx, char *buf, size_t buf_size, char *write_buf, size_t *write_buf_size)
{	
	TEE_Result res = TEE_ERROR_GENERIC;

	size_t payload_size = buf_size + (ctx->sa.block_size - (buf_size % ctx->sa.block_size));
	char *payload = calloc(payload_size, sizeof(char));
	size_t iv_len = ctx->sa.block_size / 8;

	if(payload)
	{		
		if(ctx->sa.cipher_alg == TEE_ALG_AES_GCM)
		{
			res = perform_authenticated_cipher_operation(TEE_MODE_ENCRYPT, ctx->encrypt_handle, ctx->iv, iv_len, buf, buf_size, payload, &payload_size);
		}
		else res = perform_cipher_operation(ctx->encrypt_handle, ctx->iv, iv_len, buf, buf_size, payload, &payload_size);
		
		if(res == TEE_SUCCESS)
		{			
			snfc_packet c = new_snfc_packet(ctx, payload, NULL, ctx->iv[iv_len-1]);
			size_t icv_len = ctx->sa.mac_len / 8;
			c.icv = calloc(icv_len, sizeof(char));

			if(c.icv && ((res = create_integrity_check_value(ctx, c, payload_size, c.icv, &icv_len)) == TEE_SUCCESS))
			{
				*write_buf_size = write_snfc_packet(c, ctx, write_buf, payload_size);

				ctx->seq_n++;

				res = create_next_iv(ctx->iv, iv_len, ctx->iv);
			}
			else DMSG("%s:%s:%d: Failed icv allocaiton or create_integrity_check_value, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
		}
		else DMSG("%s:%s:%d: Failed perform_cipher_operation, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);

		TEE_Free(payload);
	}
	else
	{
		res = TEE_ERROR_OUT_OF_MEMORY;
		DMSG("%s:%s:%d: Failed payload allocation, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
	}
	
	return res;
}*/

static snfc_packet read_snfc_packet(security_context *ctx, char *packet, size_t packet_size)
{
	snfc_packet c;

	c.icv = packet + (packet_size - ctx->sa.mac_len / 8);

	c.seq_n = *(typeof(c.seq_n)*)packet;
	packet += sizeof(c.seq_n);

	c.iv_check_value = *(typeof(c.iv_check_value)*)packet;
	packet += sizeof(c.iv_check_value);
	
	c.payload = packet;

	return c;
}

static TEE_Result verify_integrity_check_value(security_context *ctx, snfc_packet c, size_t payload_size)
{
	TEE_Result res = TEE_ERROR_GENERIC;	
	TEE_MACInit(ctx->mac_handle, NULL, 0);
	TEE_MACUpdate(ctx->mac_handle, &c.seq_n, sizeof(c.seq_n));
	TEE_MACUpdate(ctx->mac_handle, &c.iv_check_value, sizeof(c.seq_n));
	res = TEE_MACCompareFinal(ctx->mac_handle, c.payload, payload_size, c.icv, ctx->sa.mac_len / 8);
	if(res != TEE_SUCCESS) DMSG("%s:%s:%d: Failed TEE_MACCompareFinal, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
	return res;
}

TEE_Result process_snfc_packet(security_context *ctx, char *packet, size_t packet_size, char *write_buf, size_t *write_buf_size)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	snfc_packet c = read_snfc_packet(ctx, packet, packet_size);
	size_t payload_size = c.icv - c.payload;
	
	// Check sequence number
	if(c.seq_n != ctx->seq_n)
	{
		DMSG("Failed seq_n check. got = %d, expected = %d\n", c.seq_n, ctx->seq_n);
		return TEE_ERROR_BAD_STATE;
	}

	ctx->seq_n++;
	
	// Verify the check value
	if((res = verify_integrity_check_value(ctx, c, payload_size)) == TEE_SUCCESS)
	{			
		if((res = sync_iv(ctx, c.iv_check_value)) == TEE_SUCCESS)
		{
			// Decrypt payload
			if(ctx->sa.cipher_alg == TEE_ALG_AES_GCM)
			{
				res = perform_authenticated_cipher_operation(TEE_MODE_DECRYPT, ctx->decrypt_handle, ctx->iv, (ctx->sa.block_size / 8), c.payload, payload_size, write_buf, write_buf_size);
			}
			else res = perform_cipher_operation(ctx->decrypt_handle, ctx->iv, ctx->sa.block_size / 8, c.payload, payload_size, write_buf, write_buf_size);
			if(res == TEE_SUCCESS) res = create_next_iv(ctx->iv, ctx->sa.block_size / 8, ctx->iv);
			else DMSG("%s:%s:%d: Failed perform_x_cipher_operation, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
		}
		else DMSG("%s:%s:%d: Failed sync_iv, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
	}
	else DMSG("%s:%s:%d: Failed verify_integrity_check_value, error: hex = %x, dec = %d\n", __FILE__, __func__, __LINE__, res, res);
	
	return res;
}

