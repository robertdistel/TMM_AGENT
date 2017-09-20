/*
 * OpenSSL_ReaderWriter.h
 *
 *  Created on: 4 Sep. 2017
 *      Author: robert
 */

#ifndef OPENSSL_READERWRITER_H_
#define OPENSSL_READERWRITER_H_

#include <string>
#include <memory>
class TMM_Frame;



static const unsigned int BLOCK_SIZE = 16;





class Crypto
{

public:
  TMM_Frame  encrypt (const TMM_Frame& frame);
  TMM_Frame  decrypt (const TMM_Frame& frame);
  Crypto ();
  ~Crypto ();
private:
  uint8_t iv[BLOCK_SIZE];

};





#endif /* OPENSSL_READERWRITER_H_ */
