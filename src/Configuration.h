/*
 * Configuration.h
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_
#include <atomic>
#include <cstdint>
#include <memory>

class AlsaReaderWriter;
class UdpReaderWriter;
class Mixer;
class OpusCodec;

class Configuration
{
public:
	std::atomic<uint8_t> selected_domain;
	std::atomic<bool>    shutdown;
	static uint8_t noDomains(void);
	const char* getDomainName() const;
	std::shared_ptr<AlsaReaderWriter> 	getAudioIf(){return audio_if;}
	std::shared_ptr<UdpReaderWriter>  	getPktIf() {return pkt_if;}
	std::shared_ptr<Mixer>				getMixer() {return mixer;}
	std::shared_ptr<OpusCodec>			getCodec(uint8_t idx) {return codecs[idx];}
	Configuration();

private:
	static const char* DomainNames[];
	std::shared_ptr<AlsaReaderWriter>	audio_if;
	std::shared_ptr<UdpReaderWriter>	pkt_if;
	std::shared_ptr<Mixer>				mixer;
	std::shared_ptr<OpusCodec>			codecs[256];

};




#endif /* CONFIGURATION_H_ */
