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

TMM_Frame  OpusCodec::compress (const TMM_Frame& frame)
{
	assert(	frame.linear()==true);
	assert(	frame.plaintext()==true);

	TMM_Frame output_frame(frame);
	int err;
	if (!encoderCtx)
	{
		encoderCtx= opus_encoder_create (sample_rate, 1, OPUS_APPLICATION_VOIP, &err);
		assert(err== OPUS_OK);
		opus_encoder_ctl(encoderCtx,OPUS_RESET_STATE);
		opus_encoder_ctl(encoderCtx,OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND)	);
		opus_encoder_ctl(encoderCtx,OPUS_SET_BITRATE(32000)	); 	//allow 64kbps after compression
		opus_encoder_ctl(encoderCtx,OPUS_SET_COMPLEXITY(0)); 	//simple complexity
		opus_encoder_ctl(encoderCtx,OPUS_SET_DTX(1)); 			//discontinuous transmission enabled
		opus_encoder_ctl(encoderCtx,OPUS_SET_LSB_DEPTH	(15)); //ignore the least bit for the purpose of silence
	}
	MON("OpusCodec::compress ");
	int sz=opus_encode(encoderCtx,(const int16_t*)frame.data(),frame.data_sz()/2,output_frame.data(),TMM_Frame::maxDataSize );
	assert(sz>=0);
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
		decoderCtx= opus_decoder_create (sample_rate, 1, &err);
		assert(err== OPUS_OK);
		opus_decoder_ctl(decoderCtx,OPUS_RESET_STATE);
	}
	MON("OpusCodec::expand");
	int sz=opus_decode(decoderCtx,frame.data(),frame.data_sz(),(int16_t*)output_frame.data(),TMM_Frame::maxDataSize/2,0 );
	assert(sz>=0);
	output_frame.data_sz(sz*2);
	output_frame.linear(true);
	if(rx_sequence_number!=output_frame.stream_ctr())
		missing_packet=true;
	else
		missing_packet=false;
	rx_sequence_number=output_frame.stream_ctr()+1;
	return output_frame;

}


OpusCodec::OpusCodec (uint32_t sample_rate_):encoderCtx(nullptr),decoderCtx(nullptr),sample_rate(sample_rate_)
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

