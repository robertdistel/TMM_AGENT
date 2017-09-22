/*
 * OpenSSL_ReaderWriter.cpp
 *
 *  Created on: 4 Sep. 2017
 *      Author: robert
 */

#include "OpenSSL_ReaderWriter.h"
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <iostream>
#include <cstring>
#include "TMM_Frame.h"
#include <assert.h>
#include "perfmon.h"




static uint64_t key[3][4] = {
		{0,0,0,1},
		{0,0,0,2},
		{0,0,0,3}
};


TMM_Frame  Crypto::encrypt(const TMM_Frame& ptext) //encrypt a block
{
	MON("Crypto::encrypt");
	assert(ptext.plaintext()==true);
	assert(ptext.domain_ID()>=0 && ptext.domain_ID()<3);
	evp_cipher_ctx_st * ctx = EVP_CIPHER_CTX_new ();
	uint8_t* KEY=(uint8_t*)key[ptext.domain_ID()];


	int rc = EVP_EncryptInit_ex (ctx, EVP_aes_256_cbc (), NULL, KEY, iv);
	if (rc != 1)
	{
		std::cerr << ("EVP_EncryptInit_ex failed") << std::endl;
		exit (-1);
	}

	rc = EVP_CIPHER_CTX_set_padding (ctx, 1);
	if (rc != 1)
	{
		std::cerr << ("EVP_CIPHER_CTX_set_padding failed") << std::endl;
		exit (-1);
	}

	TMM_Frame ctext(ptext);
	// Cipher text expands upto BLOCK_SIZE
	int out_len1 = (int) TMM_Frame::maxDataSize;

	rc = EVP_EncryptUpdate (ctx, ctext.data (), &out_len1,  ptext.data (), (int) ptext.data_sz ());
	if (rc != 1)
	{
		std::cerr << "EVP_EncryptUpdate failed" << std::endl;
		exit (-1);
	}

	int out_len2 = (int) TMM_Frame::maxDataSize - out_len1;
	rc = EVP_EncryptFinal_ex (ctx, ctext.data () + out_len1, &out_len2);
	if (rc != 1)
	{
		std::cerr << "EVP_EncryptFinal_ex failed" << std::endl;
		exit (-1);
	}

	// Set cipher text size now that we know it
	ctext.data_sz (out_len1 + out_len2);
	ctext.IV(iv);
	//and set the iv to use next as the start of this cypher text
	//memcpy (iv, ctext.data (), BLOCK_SIZE);
	(*iv)++;

	EVP_CIPHER_CTX_free (ctx);
	ctext.plaintext(false);

	return ctext;
}

Crypto::Crypto ()
{

	EVP_add_cipher (EVP_aes_256_cbc ());
	memset(iv,0,BLOCK_SIZE);

}

Crypto::~Crypto ()
{
}


TMM_Frame  Crypto::decrypt (const TMM_Frame& ctext) //decrypt a block
{
	MON("Crypto::decrypt");
	assert(ctext.plaintext()==false);

	auto ctx = EVP_CIPHER_CTX_new ();

	uint8_t* KEY=(uint8_t*)key[ctext.domain_ID()];

	int rc = EVP_DecryptInit_ex (ctx, EVP_aes_256_cbc (), NULL, KEY, ctext.IV());


	if (rc != 1)
	{
		std::cerr << "EVP_DecryptInit_ex failed" << std::endl;
		exit (-1);
	}

	rc = EVP_CIPHER_CTX_set_padding (ctx, 1);
	if (rc != 1)
	{
		std::cerr << ("EVP_CIPHER_CTX_set_padding failed") << std::endl;
		exit (-1);
	}

	// Recovered text contracts upto BLOCK_SIZE
	TMM_Frame rtext(ctext);
	int out_len1 = (int) TMM_Frame::maxDataSize;

	rc = EVP_DecryptUpdate (ctx,  rtext.data (), &out_len1,  ctext.data (),  ctext.data_sz ());
	if (rc != 1)
	{
		std::cerr << "EVP_DecryptUpdate failed" << std::endl;
		exit (-1);
	}

	int out_len2 = (int) TMM_Frame::maxDataSize - out_len1;
	rc = EVP_DecryptFinal_ex (ctx, rtext.data () + out_len1, &out_len2);
	if (rc != 1)
	{
		std::cerr << "EVP_DecryptFinal_ex failed" << std::endl;
		ERR_print_errors_fp(stderr);
		exit (-1);
	}

	// Set recovered text size now that we know it
	rtext.data_sz (out_len1 + out_len2);

	//memcpy (iv, ctext->data (), BLOCK_SIZE);
	rtext.plaintext(true);

	return rtext;
}




