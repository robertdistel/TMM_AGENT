/*
 * AlsaSocketReaderWriter.h
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */

#ifndef ALSAREADERWRITER_H_
#define ALSAREADERWRITER_H_

#include <string>
#include <alsa/asoundlib.h>
#include "HardwareTimer.h"

class TMM_Frame;

class AlsaReaderWriter
{
private:
  const static unsigned int channels = 1;
  const static unsigned int bits = 16;
  const static unsigned int ms = 1000; //1000 us per ms
  const static unsigned int sec = 1000 * ms; //1000 ms per s
  HardwareTimer captureTimer;
  std::string capture_device_name;
  std::string playback_device_name;
  snd_pcm_t* capture_device_handle;
  snd_pcm_t* playback_device_handle;

  unsigned int sample_rate;

  unsigned int target_latency; //in us - this is how frequently we get interrupts to poll the buffers - this sets the length of time between when data is received and when the subsystem can 'notice' it

  unsigned int underrun_count;
  unsigned int overrun_count;
  uint64_t read_sample_count;
  FILE* log_f;

  int  configure_alsa_audio (snd_pcm_t *& device, const char* name, enum _snd_pcm_stream mode);

public:
  AlsaReaderWriter () ;

  AlsaReaderWriter (std::string capture_device_name,std::string playback_device_name,unsigned int sample_rate_ ,unsigned int target_latency_) ;


  ~AlsaReaderWriter ();
  TMM_Frame&  Read (TMM_Frame& frame);
  const TMM_Frame&  Write (const TMM_Frame& frame);

  uint16_t getRX_Timestamp(void) const ; //this is when a packet was received - it is the time at the start of the RX queue
  uint16_t getTX_Timestamp(void) const ; //this is the time when a packet will be inserted into the TX queue - ie. the time RIGHT NOW
  float getTX_TimestampRaw(void) const ; //this is the time when a packet will be inserted into the TX queue - ie. the time RIGHT NOW
  uint16_t getTX_BufferLevel(void) const;
  uint16_t getLatencyInSamples(void) const { return (target_latency * sample_rate) / sec ; }
  uint16_t getLatencyInMs(void) const { return (target_latency / ms ) ; }

};




#endif /* ALSAREADERWRITER_H_ */
