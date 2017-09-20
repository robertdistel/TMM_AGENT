/*
 * OpenSSL_Wrapper.h
 *
 *  Created on: 4 Sep. 2017
 *      Author: robert
 */

#ifndef OPENSSL_WRAPPER_H_
#define OPENSSL_WRAPPER_H_
#include <pthread.h>

#define MUTEX_TYPE            pthread_mutex_t

class OpenSSL
{
private:
  /* This array will store all of the mutexes available to OpenSSL. */
  static MUTEX_TYPE *mutex_buf;

  static void locking_function (int mode, int n, const char * file, int line);
  static unsigned long  id_function (void);
public:
  OpenSSL ();

  ~OpenSSL ();
};





#endif /* OPENSSL_WRAPPER_H_ */
