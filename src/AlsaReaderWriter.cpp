/*
 * AlsaSocketReaderWriter.cpp
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */



#include "AlsaReaderWriter.h"
#include "TMM_Frame.h"
#include <iostream>
#include <algorithm>
#include "HardwareTimer.h"
#include "perfmon.h"
#include <map>


#define INIT(FUNC,...) 																			\
		if ((err = FUNC (__VA_ARGS__)) < 0)														\
		{																						\
			fprintf (stderr, "%s failed with snd_errno (%s)\n", #FUNC, snd_strerror (err));		\
			return err;																				\
		}


int AlsaReaderWriter::configure_alsa_audio (snd_pcm_t *& device, const char* name, enum _snd_pcm_stream mode)
{
	static bool is_device_initialised=false;
	if (!is_device_initialised){
		is_device_initialised=true;
		auto ph=popen("alsactl --file /usr/share/doc/audioInjector/asound.state.MIC.thru.test restore","r");
		pclose(ph);  //wait for aslactl to doits stuff

	}
	snd_pcm_hw_params_t *hw_params;

	int err;
	unsigned int tmp;
	/* allocate memory for hardware parameter structure on the stack*/
	snd_pcm_hw_params_alloca(&hw_params);








	INIT(snd_pcm_open ,&device, name, mode, 0);

	/* fill structure from current audio parameters */
	INIT(snd_pcm_hw_params_any , device, hw_params);
	/* set access type, sample rate, sample format, channels */
	INIT(snd_pcm_hw_params_set_access , device, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	//16 bit format little endian
	INIT(snd_pcm_hw_params_set_format ,device, hw_params, SND_PCM_FORMAT_S16_LE);

	//set the sample rate
	tmp = sample_rate;
	INIT(snd_pcm_hw_params_set_rate_near , device, hw_params, &tmp, 0);

	if (tmp != sample_rate)
	{
		fprintf (stderr, "Could not set requested sample rate, asked for %d got %d\n", sample_rate, tmp);
		sample_rate = tmp;
	}
	//and number of channels
	INIT(snd_pcm_hw_params_set_channels , device, hw_params, channels);

	if (mode == SND_PCM_STREAM_PLAYBACK)
	{
		auto period(target_latency /2 + 5 * ms);
		auto buffer(target_latency + 5 * ms);

		//set roughly how long we want our minimum latency will be - the minimum time between samples becoming available and the sound system unblocking
		INIT(snd_pcm_hw_params_set_period_time_near , device, hw_params, &period, 0);

		//set how long the buffer is - ie. what the maximum latency if we fill it right up , our ignore incoming data before it over/under flows.
		INIT(snd_pcm_hw_params_set_buffer_time_near ,device, hw_params, &buffer, 0);
	}
	else
	{
		auto period(target_latency / 2 + 5 * ms);
		auto buffer(target_latency + 5 * ms);

		//set roughly how long we want our minimum latency will be - the minimum time between samples becoming available and the sound system unblocking
		INIT(snd_pcm_hw_params_set_period_time_near , device, hw_params, &period, 0);

		//set how long the buffer is - ie. what the maximum latency if we fill it right up , our ignore incoming data before it over/under flows.
		INIT(snd_pcm_hw_params_set_buffer_time_near ,device, hw_params, &buffer, 0);

	}


	INIT(snd_pcm_hw_params ,device, hw_params);

	//	auto bits_per_sample = snd_pcm_format_physical_width(SND_PCM_FORMAT_S16_LE);
	//	auto bits_per_frame = bits_per_sample * channels;
	//	auto bytes_per_frame = bits_per_frame / 8;
	//std::cout << "Bytes per frame are :" << bytes_per_frame << std::endl;


#if 0

	/* get the current hwparams */
	INIT(snd_pcm_hw_params_current,device, hw_params);

	if (snd_pcm_hw_params_supports_audio_wallclock_ts(hw_params))
		printf("Playback relies on audio wallclock timestamps\n");
	else
		printf("Playback relies on audio sample counter timestamps\n");

	snd_pcm_sw_params_alloca(&sw_params);

	/* get the current swparams */
	INIT(snd_pcm_sw_params_current,device, sw_params);


	/* enable tstamp */
	INIT( snd_pcm_sw_params_set_tstamp_mode,device, sw_params, SND_PCM_TSTAMP_ENABLE);

	/* write the sw parameters */
	INIT( snd_pcm_sw_params, device, sw_params);
#endif

	return err;
}


AlsaReaderWriter::AlsaReaderWriter () :
    												  capture_device_name (), playback_device_name (),
													  capture_device_handle (nullptr), playback_device_handle (nullptr),
													  sample_rate (0), target_latency (0),
													  underrun_count (0), overrun_count (0),
													  read_sample_count(0), log_f(nullptr)
{
}

AlsaReaderWriter::AlsaReaderWriter (std::string capture_device_name_,std::string playback_device_name_,unsigned int sample_rate_,unsigned int target_latency_) :
    												  capture_device_name (capture_device_name_), playback_device_name (playback_device_name_),
													  capture_device_handle (nullptr), playback_device_handle (nullptr),
													  sample_rate (sample_rate_), target_latency (target_latency_ * ms),
													  underrun_count (0), overrun_count (0),
													  read_sample_count(0), log_f(nullptr)
{
	//slow it down
	//max_time_to_xflow = max_time_to_xflow*10;
	//latency = latency * 10;
}



AlsaReaderWriter::~AlsaReaderWriter ()
{
	if (capture_device_handle)
	{
		snd_pcm_drop (capture_device_handle);
		snd_pcm_close (capture_device_handle);
	}
	if (playback_device_handle)
	{
		snd_pcm_drop (playback_device_handle);
		snd_pcm_close (playback_device_handle);
	}
}

uint16_t AlsaReaderWriter::getRX_Timestamp(void) const
{
	return uint16_t((static_cast<int32_t>(captureTimer.getTime(read_sample_count)*sample_rate) ) % sample_rate);
} //this is when a packet was received - it is the time at the start of the RX queue

class test_log
{
public:
	snd_pcm_sframes_t					available_frames;
	uint64_t							read_sample_count;
	test_log():available_frames(0),read_sample_count(0){}
};

uint64_t LL[100];

uint16_t AlsaReaderWriter::getTX_Timestamp(void) const
{
	//	static uint32_t ctr(0);

	snd_pcm_sframes_t available_frames(0);
	if (capture_device_handle)
		available_frames=snd_pcm_avail(capture_device_handle);
	//	LL[ctr]=available_frames+read_sample_count ;
	//	ctr++;
	//	if (ctr>=128)
	//		ctr=0;

	return uint16_t((static_cast<int32_t>(captureTimer.getTime(read_sample_count+available_frames)*sample_rate) ) % sample_rate);
} //this is the time when a packet will be inserted into the TX queue - ie. the time RIGHT NOW

float AlsaReaderWriter::getTX_TimestampRaw(void) const
{
	//	static uint32_t ctr(0);

	snd_pcm_sframes_t available_frames(0);
	if (capture_device_handle)
		available_frames=snd_pcm_avail(capture_device_handle);
	//	LL[ctr]=available_frames+read_sample_count ;
	//	ctr++;
	//	if (ctr>=128)
	//		ctr=0;

	return ((fmod(captureTimer.getTime(read_sample_count+available_frames)*sample_rate,sample_rate)));
} //this is the time when a packet will be inserted into the TX queue - ie. the time RIGHT NOW

inline int snd_pcm_readi_wrapper(_snd_pcm * handle, void * data, unsigned long int frames)
{
	MON("snd_pcm_readi");
	return snd_pcm_readi (handle, data , frames);
}




TMM_Frame&  AlsaReaderWriter::Read (TMM_Frame& tmm_frame)
{
	if (!capture_device_handle)
	{
		if (configure_alsa_audio (capture_device_handle,capture_device_name.c_str (), SND_PCM_STREAM_CAPTURE) != 0)
		{
			exit (1);
		}

		snd_pcm_prepare (capture_device_handle);
		captureTimer.resetEstimator(sample_rate);
	}

	MON("AlsaReaderWriter::Read");
	unsigned int frames = std::min(snd_pcm_bytes_to_frames (capture_device_handle, TMM_Frame::maxDataSize)-1,snd_pcm_sframes_t((target_latency*sample_rate)/sec));

	read_sample_count+=frames;
	static unsigned int ticker(0);
	if (ticker > sample_rate  ) //update once per second
	{
		ticker = 0;
		struct timespec now;
		clock_gettime(CLOCK_REALTIME,&now);
		auto available_frames=snd_pcm_avail(capture_device_handle);
		if (available_frames >= 0)
			captureTimer.updateEstimator(read_sample_count + available_frames,now);
	}
	ticker+=frames;

	//the problem - the timestamp returned is not sufficiently precise, although ntp ensures that it is infact accurate. we can only trust it to a 10ms granularity.
	//we need to interpolate this up using the number of samples written out to improve things.
	//every time we read we increment the samples read counter. when we have read samples%10000==0 samples we look at the real time and take the difference between the last time. This gives us the real time gained per 10000 samples
	tmm_frame.time( getRX_Timestamp() ); //set timestamp - in ms / wrapped at 13 bits

	//	static uint32_t ticker(0);
	//	if(ticker%100 == 0)
	//		std::cout << (static_cast<int64_t>(getTime(read_sample_count)*1000) ) << std::endl;
	//	ticker++;


	int inframes;
	while ((inframes = snd_pcm_readi_wrapper (capture_device_handle, tmm_frame.data() , frames)) < 0)
	{
		if (inframes == -EAGAIN)
			continue;
		else if (inframes == -EPIPE)
		{
			//this is an over run - we flooded the device by not reading quickly enough
			std::cerr << "Input buffer overrun\n";
			//restarting = 1;
			snd_pcm_prepare (capture_device_handle);
			captureTimer.resetEstimator(sample_rate);
			overrun_count++;
		}
		else
		{
			std::cerr << "Unknown read error : " << snd_strerror (inframes) << std::endl;
		}
	}
	if (static_cast<snd_pcm_uframes_t> (inframes) != frames)
		std::cerr << "Short read from capture device: " << inframes << ", expecting " << frames << std::endl;
	tmm_frame.data_sz(snd_pcm_frames_to_bytes (capture_device_handle, inframes));
	tmm_frame.plaintext(true);
	tmm_frame.linear(true);
	tmm_frame.set_pwr();
	tmm_frame.gain(0);
	//	tmm_frame.Dump();



	return tmm_frame;

}

const TMM_Frame&  AlsaReaderWriter::Write (const TMM_Frame& tmm_frame)
{
	assert(tmm_frame.plaintext()==true);
	assert(tmm_frame.linear()==true);

	if (!playback_device_handle)
	{
		if (configure_alsa_audio (playback_device_handle,playback_device_name.c_str (), SND_PCM_STREAM_PLAYBACK) != 0)
		{
			exit (1);
		}

		snd_pcm_prepare (playback_device_handle);
	}

	MON("AlsaReaderWriter::Write");

	unsigned int frames = snd_pcm_bytes_to_frames (playback_device_handle, tmm_frame.data_sz());


	int outframes;
	while ((outframes = snd_pcm_writei (playback_device_handle, tmm_frame.data(), frames)) < 0)
	{
		if (outframes == -EAGAIN)
		{
			continue;
		}
		else if (outframes == -EPIPE)
		{
			//this is an under run - we starved the device
			//			std::cerr << "Output buffer underrun\n";
			//restarting = 1;
			snd_pcm_prepare (playback_device_handle);
			underrun_count++;
		}
		else
		{
			std::cerr << "Unknown write error : " << snd_strerror (outframes) << std::endl;
		}
	}
	//		if (static_cast<size_t> (outframes) != frames)
	//			std::cerr << "Short write to playback device: " << outframes << ", expecting, " << frames << std::endl;

	return tmm_frame;
}


