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

bool TMM_OutputThread::halt_threads = false;


class TMM_OutputThread::Context
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




void TMM_OutputThread::Context::do_thread (std::shared_ptr<Context> pctx)
{
	set_realtime_priority(5);
	auto  pkt(pctx->config->getPktIf());
	auto  snd(pctx->config->getAudioIf());
	Crypto crypto;

	TMM_Frame tmm_frame;
	do {
		tmm_frame.domain_ID(pctx->config->selected_domain);
//		pkt->Write(crypto.encrypt(snd->Read(tmm_frame)));
		pkt->Write((snd->Read(tmm_frame)));
		(crypto.encrypt(snd->Read(tmm_frame.data_sz(16))));
	} while(!TMM_OutputThread::halt_threads );
}



TMM_OutputThread::TMM_OutputThread () :
							  pcontext (new Context)
{
}


TMM_OutputThread::TMM_OutputThread (Configuration* config_) :
							  pcontext (new Context (config_))
{
}


void TMM_OutputThread::start_thread ()
{
	pcontext->ctx = std::thread (Context::do_thread, pcontext);
}


void TMM_OutputThread::join (void)
{
	return pcontext->ctx.join ();
}


