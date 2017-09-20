/*
 * OpenSSL_Wrapper.cpp
 *
 *  Created on: 4 Sep. 2017
 *      Author: robert
 */
#include "OpenSSL_Wrapper.h"
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#define MUTEX_TYPE            pthread_mutex_t
#define MUTEX_SETUP(x)        pthread_mutex_init(&(x), NULL)
#define MUTEX_CLEANUP(x)      pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x)         pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x)       pthread_mutex_unlock(&(x))
#define THREAD_ID             pthread_self()


void OpenSSL::locking_function (int mode, int n, const char * file, int line)
{
  if (mode & CRYPTO_LOCK)
    MUTEX_LOCK(mutex_buf[n]);
  else
    MUTEX_UNLOCK(mutex_buf[n]);
}

unsigned long OpenSSL::id_function (void)
{
  return ((unsigned long) THREAD_ID);
}


OpenSSL::OpenSSL ()
{
  /* Load the human readable error strings for libcrypto */
  ERR_load_crypto_strings();

  /* Load all digest and cipher algorithms */
  OpenSSL_add_all_algorithms();

  /* Load config file, and other important initialisation */
  //		OPENSSL_config(NULL);
  //initalise openssl threading callbacks
  int i;

  mutex_buf = static_cast<pthread_mutex_t*> (malloc (
      CRYPTO_num_locks() * sizeof(MUTEX_TYPE)));
  if (!mutex_buf)
    {
      //	std::cerr << "allocating mutex buffer for openssl failed" << std::endl;
      exit (-1);
    }
  for (i = 0; i < CRYPTO_num_locks( ); i++)
    MUTEX_SETUP(mutex_buf[i]);CRYPTO_set_id_callback(id_function);CRYPTO_set_locking_callback(locking_function);

}

OpenSSL::~OpenSSL ()
{
  int i;
  if (mutex_buf)
    {
      CRYPTO_set_id_callback(NULL);CRYPTO_set_locking_callback(NULL);
      for (i = 0; i < CRYPTO_num_locks( ); i++)
	MUTEX_CLEANUP(mutex_buf[i]);
      free (mutex_buf);
      mutex_buf = NULL;
    }

  /* Removes all digests and ciphers */
  EVP_cleanup();

  /* if you omit the next, a small leak may be left when you make use of the BIO (low level API) for e.g. base64 transformations */
  CRYPTO_cleanup_all_ex_data();

  /* Remove error strings */
  ERR_free_strings();
}


/* This array will store all of the mutexes available to OpenSSL. */
MUTEX_TYPE *OpenSSL::mutex_buf = nullptr;





