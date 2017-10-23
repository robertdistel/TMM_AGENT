/*
 * Opus_ReaderWriter.cpp
 *
 *  Created on: 21 Sep. 2017
 *      Author: robert
 */

#include "Opus_ReaderWriter.h"
#include "opus.h"
#include "TMM_Frame.h"
#include <cstdint>
#include <assert.h>
#include "perfmon.h"
#include "Configuration.h"
#include <iostream>

const char* OpusErrorCodesToText(int errcode)
{
	switch (errcode)
	{
	case 	OPUS_OK:
		return "OPUS_OK. No error. ";

	case 	OPUS_BAD_ARG:
		return "OPUS_BAD_ARG. One or more invalid/out of range arguments. ";

	case 	OPUS_BUFFER_TOO_SMALL:
	 	return "OPUS_BUFFER_TOO_SMALL. Not enough bytes allocated in the buffer. ";

	case 	OPUS_INTERNAL_ERROR:
	 	return "OPUS_INTERNAL_ERROR. An internal error was detected. ";

	case 	OPUS_INVALID_PACKET:
	 	return "OPUS_INVALID_PACKET: The compressed data passed is corrupted. ";

	case 	OPUS_UNIMPLEMENTED:
	 	return "OPUS_UNIMPLEMENTED: Invalid/unsupported request number. ";

	case 	OPUS_INVALID_STATE:
	 	return "OPUS_INVALID_STATE: An encoder or decoder structure is invalid or already freed. ";

	case 	OPUS_ALLOC_FAIL:
		return "OPUS_ALLOC_FAIL: Memory allocation has failed.";
	}
	return "unknown error code";
}

TMM_Frame  OpusCodec::compress (const TMM_Frame& frame)
{
	assert(	frame.linear()==true);
	assert(	frame.plaintext()==true);

	TMM_Frame output_frame(frame);
	int err;
	if (!encoderCtx)
	{
		encoderCtx= opus_encoder_create (config->getSampleRate(), 1, OPUS_APPLICATION_VOIP, &err);
		assert(err== OPUS_OK);
		opus_encoder_ctl(encoderCtx,OPUS_RESET_STATE);
		opus_encoder_ctl(encoderCtx,OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND)	);
		opus_encoder_ctl(encoderCtx,OPUS_SET_BITRATE(config->getBitRate())	); 	//target bit rate after compression - including FEC overhead
		opus_encoder_ctl(encoderCtx,OPUS_SET_COMPLEXITY(1)); 	//simple complexity
		opus_encoder_ctl(encoderCtx,OPUS_SET_DTX(config->analogueInMode==Configuration::vox)); 			//discontinuous transmission enabled
		opus_encoder_ctl(encoderCtx,OPUS_SET_LSB_DEPTH	(15)); //ignore the least bit for the purpose of silence
		opus_encoder_ctl(encoderCtx,OPUS_SET_INBAND_FEC	(1));	//turn on Forward Error Correction
		opus_encoder_ctl(encoderCtx,OPUS_SET_PACKET_LOSS_PERC	(10)); //set aside 10% of the frame for packet loss concealment

	}
	MON("OpusCodec::compress ");
	int sz=opus_encode(encoderCtx,(const int16_t*)frame.data(),frame.data_sz()/2,output_frame.data(),TMM_Frame::maxDataSize );
	assert(sz>=0);
	if (sz<=2)
	{
		sz=0; //two or less indicates that we have detected silence. dont send anything - set sz to 0
	}
	output_frame.data_sz(sz);
	output_frame.linear(false);
	tx_sequence_number++;
	output_frame.stream_ctr(tx_sequence_number);
	output_frame.stream_idx(channel);
	return output_frame;
}

TMM_Frame  OpusCodec::expand (const TMM_Frame& frame)
{
	assert(	frame.linear()==false);
	assert(	frame.plaintext()==true);

	TMM_Frame output_frame(frame);
	int err;
	if (!decoderCtx)
	{
		decoderCtx= opus_decoder_create (config->getSampleRate(), 1, &err);
		assert(err== OPUS_OK);
		opus_decoder_ctl(decoderCtx,OPUS_RESET_STATE);
	}
	MON("OpusCodec::expand");
	//stream_ctr is a monotonically increasing unsigned 32 bit number, rolling over every 12,000 hours or so (ie - never)
	if(frame.stream_ctr() == rx_sequence_number )
	{
		//next one in order
		int sz=opus_decode(decoderCtx,frame.data(),frame.data_sz(),(int16_t*)output_frame.data(),TMM_Frame::maxDataSize/2,0 );
		assert(sz>=0);
		output_frame.data_sz(sz*2);
		rx_sequence_number=frame.stream_ctr()+1;
	}
	else if(frame.stream_ctr() > rx_sequence_number )
	{
		//we missed some packets
		//we decode two packets, using the PLC data to regenerate the previous frame

		int sz1=opus_decode(decoderCtx,frame.data(),frame.data_sz(),(int16_t*)output_frame.data(),config->getFrameSizeInSamples(), 1 ); //get no more than one frame of concealment
		assert(sz1>=0);
		int sz2=opus_decode(decoderCtx,frame.data(),frame.data_sz(),&((int16_t*)output_frame.data())[sz1],TMM_Frame::maxDataSize/2-sz1,0 );
		if (sz2<0)
			std::cerr << "opus Decoder Error " << OpusErrorCodesToText(sz2) << std::endl;
		assert(sz2>=0);
		output_frame.data_sz((sz1+sz2)*2);
		int32_t new_time = frame.time() - config->getSampleRate()/(frame.data_sz() / 2); //shift the start time forward by one frame - we have used FEC to make the missing frame
		if (new_time<0)
			new_time+=config->getSampleRate();

		output_frame.time(new_time);
		rx_sequence_number=frame.stream_ctr()+1;



	}
	else
	{
		//this one is out of order .. just ignore it and swallow it up
		output_frame.data_sz(0); //zero size
	}


	//if we are missing a packet (or its arrived out of order)
	//we must decide
	//if its just out of order - drop it and generate an empty frame - there is nothing we need to do
	//if we have just dropped the previous packet, and we have not yet played out the previous packet - we can hide it by concatenating this frame and the previous one together
	//if we havent played for a big long while, then we dont care

	output_frame.linear(true);
	return output_frame;

}


OpusCodec::OpusCodec (const Configuration* config_):encoderCtx(nullptr),decoderCtx(nullptr),config(config_)
{
	missing_packet=false;
	tx_sequence_number=0;
	rx_sequence_number=0;
	last_rx_at=0;
	channel=0;
}

OpusCodec::~OpusCodec ()
{
	if(encoderCtx)
		opus_encoder_destroy (encoderCtx);

	if(decoderCtx)
		opus_decoder_destroy (decoderCtx);

}

