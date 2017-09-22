/*
 * TMM_OutputThread.cpp
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */

#include "TMM_OutputThread.h"
#include <thread>
#include "Configuration.h"
#include "UdpReaderWriter.h"
#include "AlsaReaderWriter.h"
#include "TMM_Frame.h"
#include "setRealtimePriority.h"
#include "OpenSSL_ReaderWriter.h"
#include "perfmon.h"
#include "Opus_ReaderWriter.h"



class TMM_OutputThread::Context
{
public:

	Context (Configuration* config_) :
		halt_thread(false),ctx (),config(config_)
{
}


	bool halt_thread;
	std::thread ctx;
	Configuration* config;
	static void do_thread (std::shared_ptr<Context> pctx);
};




void TMM_OutputThread::Context::do_thread (std::shared_ptr<Context> pctx)
{
	MON_THREAD("TMM_OutputThread");
	set_realtime_priority(5);
	auto  pkt(pctx->config->getPktIf());
	auto  snd(pctx->config->getAudioIf());
	auto  codec(pctx->config->getCodec(0));
	Crypto crypto;


	TMM_Frame tmm_frame;
	do {
		{
			MON("TMM_OutputThread:MainLoop");

			tmm_frame.domain_ID(pctx->config->selected_domain);
			pkt->Write(crypto.encrypt(codec->compress(snd->Read(tmm_frame))));
		}
	} while(!pctx->halt_thread);
}






TMM_OutputThread::TMM_OutputThread (Configuration* config_) :
													  pcontext (new Context (config_))
{
}


void TMM_OutputThread::start_thread ()
{
	pcontext->ctx = std::thread (Context::do_thread, pcontext);
}

void TMM_OutputThread::stop_thread ()
{
	pcontext->halt_thread=true;
	return pcontext->ctx.join ();
}




