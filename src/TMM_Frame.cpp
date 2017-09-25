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
#include <math.h>



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
TMM_Frame&	TMM_Frame::IV(const uint8_t* iv_) { memcpy(pFrame->header.iv,iv_, 16/*sizeof(pFrame->header.iv)*/); return *this;}
const uint8_t*	TMM_Frame::IV() const { return pFrame->header.iv;}
TMM_Frame&	TMM_Frame::linear(bool b) { pFrame->header.linear=b; return *this;}
TMM_Frame&	TMM_Frame::plaintext(bool b) { pFrame->header.encrypted=!b; return *this;}
TMM_Frame&  TMM_Frame::stream_ctr(uint32_t ctr) {pFrame->header.stream_ctr=ctr; return *this;}
uint32_t 	TMM_Frame::stream_ctr(void) const {return pFrame->header.stream_ctr;}
TMM_Frame&  TMM_Frame::stream_idx(uint8_t idx) {pFrame->header.stream_idx=idx; return *this;}
uint8_t 	TMM_Frame::stream_idx(void) const {return pFrame->header.stream_idx;}
TMM_Frame&  TMM_Frame::pwr(int8_t pwr) {pFrame->header.pwr=pwr; return *this;}
int8_t		TMM_Frame::pwr() const {return pFrame->header.pwr;}
TMM_Frame&  TMM_Frame::gain(int8_t gain) {pFrame->header.gain=gain; return *this;}
int8_t		TMM_Frame::gain() const {return pFrame->header.gain;}


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

TMM_Frame::TMM_Frame(const TMM_Frame & rhs):pFrame(new frame_t)
{
	//need to do a deep copy
	*pFrame=*(rhs.pFrame); //copy the contents of the from not just the reference
}

std::vector<int32_t> init_gain_LUT(void )
{
	std::vector<int32_t> m(97);
	for (int k=0; k<97; k++)
	{
		int v=uint32_t(16384*pow10(double(k)/20));  //given a power gain in dB this is the multiplication required - note the factor of 20
		m[k]=v;
	}
	return m;
}


std::vector<int32_t> gain_LUT(init_gain_LUT());



TMM_Frame&  TMM_Frame::set_gain()
{
	assert(plaintext()==true);
	assert(linear()==true);

	int32_t g=gain();
	if (g>0)
	{
		g=gain_LUT[g];
		for(size_t k=0; k<data_sz()/2; k++)
			((int16_t *)data())[k]=g*((int16_t *)data())[k]/16384;
	}
	else
	{
		g=gain_LUT[-g];
		for(size_t k=0; k<data_sz()/2; k++)
			((int16_t *)data())[k]=(int32_t)16384*((int16_t *)data())[k]/g;

	}
	gain(0);
	return *this;
}

std::map<int32_t,int32_t> init_log_LUT(void )
						{
	std::map<int32_t,int32_t> m;
	for (int k=0; k<97; k++)
	{
		int v=uint32_t(pow10(double(k)/10));
		m[v]=k;
	}
	return m;
						}


const std::map<int32_t,int32_t> log_LUT(init_log_LUT());

uint32_t dB(uint32_t p)
{
	auto v=log_LUT.lower_bound(p);
	return v->second;
}

TMM_Frame&  TMM_Frame::set_pwr() //measure the frames power (if linear and non-encrypted) and set the pwr field
{
	assert(plaintext()==true);
	assert(linear()==true);
	int32_t 	sum(0);
	int32_t 	sum_2(0);
	int32_t 	x;
	size_t		n(data_sz()/2);

	for(size_t k=0; k<n; k++)
	{
		x=((int16_t*)data())[k];
		sum+=x;
		sum_2+=x*x;
	}

	x=(sum_2-(sum*sum)/n)/n;
	pwr(dB(x)-72); //sets full scale square wave (14bits ) as +6dB power point
	//and we are now somewhere between -84 and 6 dB
	//	static int counter(0);
	//	counter++;
	//	if (counter%50==0)
	//		std::cout<< x<<":" << dB(x) <<std::endl;
	return *this ;
}


