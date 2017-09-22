/*
 * TMM_InputThread.cpp
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */

#include "TMM_InputThread.h"
#include <thread>
#include "Configuration.h"
#include "UdpReaderWriter.h"
#include "AlsaReaderWriter.h"
#include "TMM_Frame.h"
#include "Mixer.h"
#include "setRealtimePriority.h"
#include "OpenSSL_ReaderWriter.h"
#include "Opus_ReaderWriter.h"
#include "perfmon.h"




class TMM_InputThread::Context
{
public:


	Context (Configuration* config_) :
		halt_thread(false), ctx (),config(config_)
	{
	}


	bool halt_thread;
	std::thread ctx;
	Configuration* config;
	static void do_thread (std::shared_ptr<Context> pctx);
};




void TMM_InputThread::Context::do_thread (std::shared_ptr<Context> pctx)
{
	MON_THREAD("TMM_InputThread");
	set_realtime_priority();
	auto mix(pctx->config->getMixer());
	auto pkt(pctx->config->getPktIf());
	auto  codec(pctx->config->getCodec(0));
	Crypto crypto;
//	auto snd(pctx->config->getAudioIf());


	TMM_Frame tmm_frame;
	do {
		{
			MON("TMM_InputThread:MainLoop");
		mix->Write(codec->expand(crypto.decrypt(pkt->Read(tmm_frame))));
//		mix->Write((pkt->Read(tmm_frame)));
		}
	} while(!pctx->halt_thread );
}





TMM_InputThread::TMM_InputThread (Configuration* config_) :
							  pcontext (new Context (config_))
{
}


void TMM_InputThread::start_thread ()
{
	pcontext->ctx = std::thread (Context::do_thread, pcontext);
}

void TMM_InputThread::stop_thread ()
{
	pcontext->halt_thread=true;
	pcontext->ctx.join ();
}


