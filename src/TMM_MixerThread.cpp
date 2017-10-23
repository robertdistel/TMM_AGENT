#include <thread>
#include <atomic>
#include "Configuration.h"
#include "TMM_Frame.h"
#include <cstring>
#include <cstdint>
#include "Mixer.h"
#include "TMM_MixerThread.h"
#include "AlsaReaderWriter.h"
#include <iostream>
#include "setRealtimePriority.h"
#include "perfmon.h"
#include "direct_gpio.h"




class TMM_MixerThread::Context
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
int32_t diff(struct timespec& now,struct timespec& prev)
{
	auto d = (now.tv_sec - prev.tv_sec) * 1000;
	d+=(now.tv_nsec - prev.tv_nsec)/1000000;
	return d;
}
void TMM_MixerThread::Context::do_thread(std::shared_ptr<Context> pctx)
{
	MON_THREAD("TMM_MixerThread");
	while(!pctx->config->timebase_locked)
		sleep(1);

	set_realtime_priority();
	auto mix(pctx->config->getMixer());
	auto snd(pctx->config->getAudioIf());
	TMM_Frame tmm_frame;

	tmm_frame.data_sz(snd->getLatencyInSamples()*2); // in bytes
	do {
		MON("TMM_MixerThread::main Loop");
		tmm_frame.time(snd->getTX_Timestamp());

		snd->Write(mix->Read(tmm_frame));
	} while(!pctx->halt_thread );

}



TMM_MixerThread::TMM_MixerThread (Configuration* config_) :
											  pcontext (new Context (config_))
{
}


void TMM_MixerThread::start_thread ()
{
	pcontext->ctx = std::thread (Context::do_thread, pcontext);
}

void TMM_MixerThread::stop_thread ()
{
	pcontext->halt_thread=true;
	return pcontext->ctx.join ();
}




