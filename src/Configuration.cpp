/*
 * Configuration.cpp
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */

#include <strings.h>
#include <string>
#include "Configuration.h"
#include "lcd.h"
#include "AlsaReaderWriter.h"
#include "UdpReaderWriter.h"
#include "Mixer.h"
#include "Opus_ReaderWriter.h"
#include <boost/program_options.hpp>


const char * Configuration::DomainNames[]={
		"G:Plain Text",
		"W:Restricted",
		"R:Secret"
};

template <class T>
class NamesWrapper
{
public:
	static	const char * Names[];
	static  const char  TypeName[];
};



#define TYPENAME(T)   template <> const char  NamesWrapper<Configuration:: T>::TypeName[] = #T
#define ENUMNAMES(T) template <>  const char * NamesWrapper<Configuration:: T >::Names[]

TYPENAME(AnalogueInMode);
TYPENAME(AnalogueOutMode);
TYPENAME(SecureRadioMode);
TYPENAME(MixMode);
TYPENAME(ArbFlags);
TYPENAME(FreePinsOnBCM);

ENUMNAMES(AnalogueInMode) = {

		"none",
		"vox",
		"hw"
};

ENUMNAMES(AnalogueOutMode) = {

		"none",
		"hw"
};

ENUMNAMES(SecureRadioMode) = {

		"none",
		"no_handshake",
		"handshake"

};

ENUMNAMES(MixMode) =
{

		"mix",
		"arb"
};

ENUMNAMES(ArbFlags) = {
		"first_come",
		"prio",
		"power",
		"pri_power",
		"power_pri"
};

ENUMNAMES(FreePinsOnBCM) ={
		"none",			//0
		0,				//1
		0,
		0,
		"PIN_7",		//4
		"PIN_29",		//5
		"PIN_31",		//6
		0,
		0,
		0,
		0,
		"PIN_36",		//11
		"PIN_32",		//12
		0,
		"PIN_8",		//14
		"PIN_10",		//15
		0,
		"PIN_11",		//17
		0,
		0,
		0,				//20
		0,
		0,
		0,
		0,
		0,				//25
		0,
		0,
		0,
		0,
		0,				//30
		0,
		0,
		"PIN_13"		//33

};

uint8_t Configuration::noDomains(void ) { return sizeof(DomainNames)/sizeof(DomainNames[0]); }
const char* Configuration::getDomainName(void) const {return DomainNames[selected_domain];}

bool Configuration::initialise(DefaultConfigTypes defaultConfigType)
{
	//set up defaults
	plaintext_domain=plaintext_domain % Configuration::noDomains();
	cyphertext_domain = cyphertext_domain % Configuration::noDomains();
	if (plaintext_domain == cyphertext_domain)
	{
		std::cerr << "cannot set cyphertext and plaintext domains to the same domain" << std::endl;
		return false;
	}

	frame_size_in_ms = (frame_size_in_ms/5) * 5; //make sure its a multiple of 5ms
	frame_size_in_ms=(frame_size_in_ms>50)?50:frame_size_in_ms; //clip it
	frame_size_in_ms=(frame_size_in_ms<5)?5:frame_size_in_ms;
	system_delay_in_ms=(system_delay_in_ms<2*frame_size_in_ms)?2*frame_size_in_ms:system_delay_in_ms; //clip - we need at least two frame delays to play back
	system_delay_in_ms=system_delay_in_ms>500?500:system_delay_in_ms;
	compressor_bit_rate=8000*(frame_size_in_ms*compressor_bit_rate/ 8000  ) / frame_size_in_ms; //each frame must be an integral number of bytes
	switch (defaultConfigType)
	{
	case Operator:
	{
		analogueInMode=none_in;
		analogueOutMode=none_out;
		secureRadioMode=no_crypto;
		mixMode=mix;
		arbFlags=ArbFlags(0); //dont matter - will be ignored
		analogueInModePin=no_pin;
		analogueInMutePin=PIN_7; // pull high to mute
		analogueOutModePin=no_pin;
		cypherModeRequestPin=no_pin;
		cypherModeRequestPin=no_pin;
		break;
	}
	case PlaintextRadio:
	{
		analogueInMode=codan;
		analogueOutMode=ptt;
		secureRadioMode=no_crypto;
		mixMode=arb;
		arbFlags=FirstCome_FirstServed; //dont matter - will be ignored
		analogueInModePin=PIN_7;
		analogueInMutePin=no_pin;
		analogueOutModePin=PIN_31;
		cypherModeRequestPin=no_pin;
		cypherModeActivePin=no_pin;
		break;
	}
	case CryptoRadio:
	{
		analogueInMode=vox;
		analogueOutMode=ptt;
		secureRadioMode=CypherTextHandshake;
		mixMode=arb;
		arbFlags=FirstCome_FirstServed; //dont matter - will be ignored
		analogueInModePin=no_pin;
		analogueInMutePin=no_pin;
		analogueOutModePin=PIN_31;
		cypherModeRequestPin=PIN_7;
		cypherModeActivePin=PIN_29;
		break;
	}
	default: return false;
	}

	//now parse all options
	bool b =  parse(options[NamesWrapper<AnalogueInMode>::TypeName],analogueInMode) &&
			parse(options[NamesWrapper<AnalogueOutMode>::TypeName],analogueOutMode) &&
			parse(options[NamesWrapper<SecureRadioMode>::TypeName],secureRadioMode) &&
			parse(options[NamesWrapper<MixMode>::TypeName],mixMode) &&
			parse(options[NamesWrapper<ArbFlags>::TypeName],arbFlags) &&
			parse(options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("analogueInModePin")],analogueInModePin) &&
			parse(options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("analogueInMutePin")],analogueInMutePin) &&
			parse(options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("analogueOutModePin")],analogueOutModePin) &&
			parse(options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("cypherModeRequest")],cypherModeRequestPin) &&
			parse(options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("cypherModeActive")],cypherModeActivePin);

	if (!b) return false;



	audio_if = std::shared_ptr<AlsaReaderWriter>(new AlsaReaderWriter("plughw:CARD=audioinjectorpi,DEV=0","plughw:CARD=audioinjectorpi,DEV=0",this));
	pkt_if = std::shared_ptr<UdpReaderWriter>(new UdpReaderWriter("10.42.0.1:12345"));
	mixer = std::shared_ptr<Mixer>(new Mixer(system_delay_in_ms,48000));
	for(size_t k=0; k<256; k++)
		codecs[k]=std::shared_ptr<OpusCodec>(new OpusCodec(this));

	return true;

}


Configuration::Configuration()
{
	sample_rate=48000;  //we cannot have any other sample rate due to bugs in the ALSA driver
	timebase_locked=false;
}

template <class T>
bool Configuration::parse(const std::string& s, T& out)
{
	if (strcasecmp(s.c_str(),"default")==0)
	{
		return true; //dont change from default value
	}

	for (uint32_t v=(0); v!=sizeof(NamesWrapper<T>::Names)/sizeof(NamesWrapper<T>::Names[0]); v++)
	{
		if (NamesWrapper<T>::Names[v]!=0 && (strcasecmp(s.c_str(),NamesWrapper<T>::Names[v])==0))
		{
			out=T(v);
			return true;
		}
	}
	std::cerr << "Invalid value " << s << " specified for " << NamesWrapper<T>::TypeName << ". Using default instead." << std::endl;
	return false;
}

template <class T>
std::string Configuration::getOptions()
{
	std::string s("Options for ");
	s+=NamesWrapper<T>::TypeName;
	s+="\n";
	for (uint32_t v=0; v!=sizeof(NamesWrapper<T>::Names)/sizeof(NamesWrapper<T>::Names[0]); v++)
	{
		if (NamesWrapper<T>::Names[v]!=0 )
		{
			s += std::string(NamesWrapper<T>::Names[v]);
			s += std::string("\n");
		}
	}
	s+=std::string("Default is ") + NamesWrapper<T>::Names[0] + "\n";
	return s;
}

po::options_description& Configuration::add_options(po::options_description& opts)
{
	//set up defaults
	options[NamesWrapper<AnalogueInMode>::TypeName]=std::string("default");
	options[NamesWrapper<AnalogueOutMode>::TypeName]=std::string("default");
	options[NamesWrapper<SecureRadioMode>::TypeName]=std::string("default");
	options[NamesWrapper<MixMode>::TypeName]=std::string("default");
	options[NamesWrapper<ArbFlags>::TypeName]=std::string("default");
	options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("analogueInModePin")]=std::string("default");
	options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("analogueInMutePin")]=std::string("default");
	options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("analogueOutModePin")]=std::string("default");
	options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("cypherModeRequest")]=std::string("default");
	options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("cypherModeActive")]=std::string("default");
	cyphertext_domain=1;
	plaintext_domain=0;
	frame_size_in_ms=5;
	system_delay_in_ms=50;
	compressor_bit_rate=32000;

	opts.add_options ()
										("analog-input-mode", po::value<std::string> (&options[NamesWrapper<AnalogueInMode>::TypeName]), getOptions<AnalogueInMode>().c_str())
										("analog-output-mode",po::value<std::string> (&options[NamesWrapper<AnalogueOutMode>::TypeName]),getOptions<AnalogueOutMode>().c_str())
										("secure-radio-mode",po::value<std::string> (&options[NamesWrapper<SecureRadioMode>::TypeName]),getOptions<SecureRadioMode>().c_str())
										("mix-mode",po::value<std::string> (&options[NamesWrapper<MixMode>::TypeName]),getOptions<MixMode>().c_str())
										("arb-mode",po::value<std::string> (&options[NamesWrapper<ArbFlags>::TypeName]),getOptions<ArbFlags>().c_str())
										("analog-input-enable-pin",po::value<std::string> (&options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("analogueInModePin")]),(std::string("Analogue input enable pin. ")+getOptions<FreePinsOnBCM>()).c_str())
										("analog-input-mute-pin",po::value<std::string> (&options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("analogueInMutePin")]),(std::string("Analogue input mute pin. ")+getOptions<FreePinsOnBCM>()).c_str())
										("analog-output-active-pin",po::value<std::string> (&options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("analogueOutModePin")]),(std::string("Analogue output active pin. ")+getOptions<FreePinsOnBCM>()).c_str())
										("cypher-mode-request-pin",po::value<std::string> (&options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("cypherModeRequest")]),(std::string("Cypher mode request pin. ")+getOptions<FreePinsOnBCM>()).c_str())
										("cypher-mode-active-pin",po::value<std::string> (&options[NamesWrapper<FreePinsOnBCM>::TypeName+std::string("cypherModeActive")]),(std::string("Cypher mode active pin. ")+getOptions<FreePinsOnBCM>()).c_str())
										("cypher-text-domain",po::value<std::uint8_t> (&cyphertext_domain),("Cypher text domain for this radio - 0-7. Default is 1\n"))
										("plain-text-domain",po::value<std::uint8_t> (&plaintext_domain),("Plain text domain for this radio - 0-7. Default is 0\n"))
										("frame-size-in-ms",po::value<std::uint16_t> (&frame_size_in_ms),("Frame size in ms. Minimum and default is 5ms. Max is 50 ms. Must be in increments of 5ms\n"))
										("plain-text-domain",po::value<std::uint8_t> (&plaintext_domain),("System delay in ms. Sets size of delay compensation buffer. Default is 50, maximum is 500ms \n"))
										("compressor-bit-rate",po::value<std::uint32_t> (&compressor_bit_rate),(
												"Target bit rate for the Opus compressor. Opus is compressing fullband (48kbps 24kHz b/w) audio.\n"
												"It supports 10% packet loss concealment overhead concealing the loss of a single packet, \n"
												"with 16 bit linear audio frames. Larger frame sizes improve compression quality. \n"
												"Default is 32(kbps). Acceptable values [8-64]\n"
												"Note that frame overheads will considerably increase this number, particularly with small frames\n"));

	return opts;
}


