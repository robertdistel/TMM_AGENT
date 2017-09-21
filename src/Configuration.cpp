/*
 * Configuration.cpp
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */

#include "Configuration.h"
#include "lcd.h"
#include "AlsaReaderWriter.h"
#include "UdpReaderWriter.h"
#include "Mixer.h"


const char * Configuration::DomainNames[]={
		"G:Plain Text",
		"W:Restricted",
		"R:Secret"
};


uint8_t Configuration::noDomains(void ) { return sizeof(DomainNames)/sizeof(DomainNames[0]); }
const char* Configuration::getDomainName(void) const {return DomainNames[selected_domain];}

Configuration::Configuration():selected_domain(0),audio_if(new AlsaReaderWriter("plughw:CARD=audioinjectorpi,DEV=0","plughw:CARD=audioinjectorpi,DEV=0",48000,5)),
		pkt_if(new UdpReaderWriter("10.42.0.1:12345")),
		mixer(new Mixer(50,48000))
{

}




