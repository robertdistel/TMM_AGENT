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
#include <map>
#include "HardwareTimer.h"

namespace boost {
namespace program_options
{
class options_description;
}
}

namespace po = boost::program_options;

class AlsaReaderWriter;
class UdpReaderWriter;
class Mixer;
class OpusCodec;

class Configuration
{
public:
	enum DefaultConfigTypes {Operator, PlaintextRadio, CryptoRadio};

	enum AnalogueInMode {none_in, vox, codan};  	//how to deal with audio from microphone - do we enable it all the time, use voxing or use a hw input to gate it

	enum AnalogueOutMode  {none_out, ptt};					//how to deal with audio to the speakers, do we output a pin when audio is active

	enum SecureRadioMode {no_crypto,NoCypherTextHandshake, CypherTextHandshake}; //is there a secure radio - does it drive a handshake when put in to CT mode, and do we have to wait for the handshake

	enum MixMode { mix,arb};						//do we add multiple audio streams or select a single stream

	enum ArbFlags {FirstCome_FirstServed, Priority, Power, PriorityThenPower, PowerThenPriority}; //how to perform arbitration - priority or power or combinations

	enum FreePinsOnBCM { no_pin=0,PIN_8=14, PIN_10=15, PIN_32=12, PIN_36=11, PIN_7=4, PIN_11=17, PIN_29=5, PIN_31=6, PIN_13=33 };
	//these are the unused pins that are free on the 40pin header on the raspi2/3 and pi0. PIN_x refers to the xth pin on the header. The numerical value refers to the
	//mapping in the BCM35xx internal registers, and the values used by the device drivers. ie. PIN_7 on the header is attached to io device 4 in the sysctl file system.
	//this allows us to talk about mappings on the header - not in the internal registers


	po::options_description& add_options(po::options_description& opts);

	bool initialise(DefaultConfigTypes defaultConfigType);   //called after all option parsing set up and option strings loaded - goes and initialises the conf structure with the parsed option outputs
							//returns true on success


	AnalogueInMode						analogueInMode;
	FreePinsOnBCM						analogueInModePin;		//optional - pin to enable audio at microphone - pull high to enable audio
	FreePinsOnBCM						analogueInMutePin;		//optional - pin to mute audio at mic - pull high to mute

	AnalogueOutMode						analogueOutMode;
	FreePinsOnBCM						analogueOutModePin;		//optional - pin to assert when audio is being sent to speaker

	SecureRadioMode						secureRadioMode;		//indicates how a crypto radio is to be used
	uint8_t								cyphertext_domain;
	uint8_t								plaintext_domain;
	FreePinsOnBCM						cypherModeRequestPin;		//optional - assert when transmitting audio that is in the cyphertext_domain
	FreePinsOnBCM						cypherModeActivePin;		//optional - pin to indicate that the radio is in cyphertext mode, received audio is in the cyphertext domain

	MixMode								mixMode;				//how multiple inbound audio streams are to be handled - either mixed together or a single stream selected
	ArbFlags							arbFlags;				//rule to be applied for arbitrating streams



	std::atomic<uint8_t> 				selected_domain;
	std::atomic<bool>    				shutdown;
	std::atomic<bool>					timebase_locked;


	uint8_t noDomains(void);
	const char* getDomainName() const;

	std::shared_ptr<AlsaReaderWriter> 	getAudioIf(){return audio_if;}
	std::shared_ptr<UdpReaderWriter>  	getPktIf() {return pkt_if;}
	std::shared_ptr<Mixer>				getMixer() {return mixer;}
	std::shared_ptr<OpusCodec>			getCodec(uint8_t idx) {return codecs[idx];}
	uint32_t							getSampleRate() const {return sample_rate;}
	uint32_t							getBitRate() const { return compressor_bit_rate;}
	uint32_t							getFrameSize() const { return frame_size_in_ms;}
	uint32_t							getFrameSizeInSamples() const { return (frame_size_in_ms * sample_rate) / 1000; }


	Configuration();
	HardwareTimer 						captureTimer;

private:
	uint16_t							frame_size_in_ms;
	uint16_t							system_delay_in_ms;
	uint32_t							compressor_bit_rate;
	uint32_t							sample_rate;

	std::map<std::string,std::string> options; //command line options
	//method to turn string identifying an enum into the enum itself
	template <class T>
	bool parse(const std::string& s, T& v);

	template <class T>
	std::string getOptions();

	static const char* DomainNames[];
	std::shared_ptr<AlsaReaderWriter>	audio_if;
	std::shared_ptr<UdpReaderWriter>	pkt_if;
	std::shared_ptr<Mixer>				mixer;
	std::shared_ptr<OpusCodec>			codecs[256];


};




#endif /* CONFIGURATION_H_ */
