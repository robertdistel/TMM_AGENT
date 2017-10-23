/*
 * Opus.h
 *
 *  Created on: 21 Sep. 2017
 *      Author: robert
 */

#ifndef OPUS_READERWRITER_H_
#define OPUS_READERWRITER_H_

#include <cstdint>

class TMM_Frame;
class OpusEncoder;
class OpusDecoder;
class Configuration;



class OpusCodec
{

public:
  TMM_Frame  compress (const TMM_Frame& frame);
  TMM_Frame  expand (const TMM_Frame& frame);
  OpusCodec (const Configuration* config_);
  ~OpusCodec ();
private:
  OpusEncoder* encoderCtx;
  OpusDecoder* decoderCtx;
  const Configuration* 	config;

  uint32_t 	tx_sequence_number;
  uint32_t 	rx_sequence_number;
  uint64_t 	last_rx_at;
  uint8_t	channel;
  bool		missing_packet;

};




#endif /* OPUS_READERWRITER_H_ */
