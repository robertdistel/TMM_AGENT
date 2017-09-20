/*
 * TMM_Frame.cpp
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */

#include "TMM_Frame.h"
#include "AlsaReaderWriter.h"
#include "UdpReaderWriter.h"
#include "Mixer.h"
#include <iostream>



uint16_t	TMM_Frame::packet_sz() const {return pFrame->header.packet_sz;}
uint8_t		TMM_Frame::domain_ID() const {return pFrame->header.domain_ID;}
uint16_t   	TMM_Frame::time() const {return pFrame->header.time;}
TMM_Frame&	TMM_Frame::packet_sz(uint16_t sz){ pFrame->header.packet_sz = sz; return *this;}
TMM_Frame&	TMM_Frame::domain_ID(uint8_t id){pFrame->header.domain_ID = id; return *this;}
TMM_Frame&  TMM_Frame::time(uint16_t t){ pFrame->header.time=t; return *this;}
uint8_t*	TMM_Frame::data(){return pFrame->data;}
const uint8_t*	TMM_Frame::data() const {return &pFrame->data[0];}
uint8_t*	TMM_Frame::frame(){return (uint8_t*)&pFrame->header;}
const uint8_t*	TMM_Frame::frame() const {return (uint8_t*)&pFrame->header;}
bool		TMM_Frame::linear() const {return pFrame->header.linear;}
bool		TMM_Frame::plaintext() const {return !pFrame->header.encrypted;}
uint8_t*	TMM_Frame::IV() { return pFrame->header.iv;}
const uint8_t*	TMM_Frame::IV() const { return pFrame->header.iv;}
TMM_Frame&	TMM_Frame::linear(bool b) { pFrame->header.linear=b; return *this;}
TMM_Frame&	TMM_Frame::plaintext(bool b) { pFrame->header.encrypted=!b; return *this;}


#define DUMP(V) std::cout << #V <<" "<< V ()<< std::endl;
void TMM_Frame::Dump(void)
{
	DUMP(packet_sz);
	DUMP(data_sz);
	DUMP(domain_ID);
	DUMP(time);
	DUMP(linear);
	DUMP(plaintext);
}


TMM_Frame::TMM_Frame():pFrame(new frame_t)
{

}

