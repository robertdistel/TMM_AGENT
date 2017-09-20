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

bool TMM_InputThread::halt_threads = false;


class TMM_InputThread::Context
{
public:
	Context () :config(nullptr)
{
}

	Context (Configuration* config_) :
		ctx (),config(config_)
	{
	}


	std::thread ctx;
	Configuration* config;
	static void do_thread (std::shared_ptr<Context> pctx);
};




void TMM_InputThread::Context::do_thread (std::shared_ptr<Context> pctx)
{
	set_realtime_priority();
	auto mix(pctx->config->getMixer());
	auto pkt(pctx->config->getPktIf());
	Crypto crypto;
//	auto snd(pctx->config->getAudioIf());


	TMM_Frame tmm_frame;
	do {
//		mix->Write(crypto.decrypt(pkt->Read(tmm_frame)));
		mix->Write((pkt->Read(tmm_frame)));
	} while(!TMM_InputThread::halt_threads );
}



TMM_InputThread::TMM_InputThread () :
							  pcontext (new Context)
{
}


TMM_InputThread::TMM_InputThread (Configuration* config_) :
							  pcontext (new Context (config_))
{
}


void TMM_InputThread::start_thread ()
{
	pcontext->ctx = std::thread (Context::do_thread, pcontext);
}


void TMM_InputThread::join (void)
{
	return pcontext->ctx.join ();
}

