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
#include "direct_gpio.h"



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
	while(!pctx->config->timebase_locked)
		sleep(1);

	set_realtime_priority(5);
	auto  pkt(pctx->config->getPktIf());
	auto  snd(pctx->config->getAudioIf());
	auto  codec(pctx->config->getCodec(0));
	Pin analogueInMode(pctx->config->analogueInModePin,Pin::readonly);
	Pin analogueInMute(pctx->config->analogueInMutePin,Pin::readonly);
	Pin cyphertextActive(pctx->config->cypherModeActivePin,Pin::readonly);
	Crypto crypto;


	TMM_Frame tmm_frame;
	do {
		{
			MON("TMM_OutputThread:MainLoop");

			if(pctx->config->secureRadioMode==Configuration::no_crypto)
				tmm_frame.domain_ID(pctx->config->selected_domain);
			else
			{
				if (cyphertextActive)
				{
					tmm_frame.domain_ID(pctx->config->cyphertext_domain);
				}
				else
				{
					tmm_frame.domain_ID(pctx->config->plaintext_domain);
				}
			}

			tmm_frame=snd->Read(tmm_frame);
			if (analogueInMute)
				memset(tmm_frame.data(),0,tmm_frame.data_sz());	//if we have the mute enabled - memset the frame to zeros

			tmm_frame=codec->compress(tmm_frame);

			if(
					tmm_frame.data_sz()==0 ||	//vox mode is enabled and we detected a zero length frame or
					((pctx->config->analogueInMode == Configuration::codan) && !analogueInMode) //codan mode is enabled and there is no codan present
			)
				continue;

			pkt->Write(crypto.encrypt(tmm_frame));
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




