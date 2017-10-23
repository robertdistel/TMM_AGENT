/*
 * Mixer.cpp
 *
 *  Created on: 15 Sep. 2017
 *      Author: robert
 */

#include "Mixer.h"

#include "TMM_Frame.h"
#include <iostream>
#include <assert.h>
#include "perfmon.h"





Mixer::Mixer(uint16_t target_latency_,uint32_t sample_rate):target_latency(target_latency_),samples_per_ms(sample_rate/1000)
{
	buff_sz=sample_rate; //one second buffer
	sample_buffer=std::vector<uint16_t>(buff_sz,0);
	last_read=0;
	phase=0;
	rx_last_active=0;
}




const TMM_Frame&  Mixer::Write (const TMM_Frame& tmm_frame)
{
	MON("Mixer::Write");
	assert(tmm_frame.linear()==true);
	assert(tmm_frame.plaintext()==true);
	rx_last_active=5;

	uint16_t len(tmm_frame.data_sz()/2); //number of samples
	//we write it in - adding the desired latency on (latency*samples_per_ms)
	uint32_t k((target_latency)*samples_per_ms + tmm_frame.time());

	int32_t delta = uint32_t(k) -(last_read);
	if (delta < -int32_t(buff_sz/2))
		delta += buff_sz;

	if (delta > int32_t(buff_sz/2))
		delta -=buff_sz;

	if (delta <0)
		return tmm_frame; //it would end up writing into the area we are reading from

	//	static uint32_t ticker(0);
	//	if (ticker% 2 ==0)
	//		std::cout << "W@" << k/samples_per_ms << "." << ((k%samples_per_ms)*10)/samples_per_ms << std::endl;
	//	ticker++;
	for(uint16_t j(0); j<len; j++,k++)
	{
		while(k>=buff_sz)
			k=k-buff_sz;
		sample_buffer[k]+=((uint16_t*)tmm_frame.data())[j];
	}
	return tmm_frame;
}

//the timestamp to read and the duration is set in the frame
TMM_Frame&  Mixer::Read (TMM_Frame& tmm_frame)
{
	MON("Mixer::Read");

	uint16_t len(tmm_frame.data_sz()/2);
	uint32_t k(last_read); //we set k to continue from where we left off.

	if(rx_last_active!=0)
		rx_last_active--;
	for(uint16_t j(0); j<len; j++,k++) //and read in the new stuff
	{
		while(k>=buff_sz)
			k=k-buff_sz;
		((uint16_t*)tmm_frame.data())[j]=sample_buffer[k];
		sample_buffer[k]=0;
	}

	//in theory last_read and tmm_frame.time() are the same - all clocks are correct and in phase
	//but this is never true
	//if last read < .time() < last_read+len/2 then where we think we are and where we are reading from are "close enough"
	//if .time() < last_read then we are running slow.  we need to drop out a sample by skipping last read forward by one
	//if .time() > last_read+len/2 then we are running fast. we need to insert an extra sample in by duplicating a sample, we pop last read back by one

	int32_t delta = uint32_t(tmm_frame.time()) -last_read;
	if (delta < -int32_t(buff_sz/2))
		delta += buff_sz;

	if (delta > int32_t(buff_sz/2))
		delta -=buff_sz;

	phase = (1023*phase + delta) / 1024;


#if 0

	static int ticker(0);
	if (ticker % 100 ==0)
	{
		std::cout <<" phase "<<  phase;
		//std::cout << " : " << snd->getTX_TimestampRaw() ;
		std::cout << std::endl;
	}
	ticker++;

#endif

	last_read=k;
	if ((phase > 48) || (phase < 0))
	{
		if ((phase > 1000) || (phase < -1000))
		{
			last_read = tmm_frame.time() + len; // big step to the right time
//			std::cout << "big step\n";
		}
		else
		{
			if (phase > 48)
			{
//				std::cout << "adjust +1\n";
				sample_buffer[last_read]=0;
				last_read+=1;
			}
			if (phase < 0)
			{
//				std::cout << "adjust -1\n";
				sample_buffer[(buff_sz+last_read-1) % buff_sz]=sample_buffer[last_read];
				last_read-=1;
			}
			last_read=(last_read+buff_sz) % buff_sz;
		}

	}


	tmm_frame.linear(true);
	tmm_frame.plaintext(true);

	return tmm_frame;
}

