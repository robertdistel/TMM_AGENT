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



bool TMM_MixerThread::halt_threads = false;

class TMM_MixerThread::Context
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
int32_t diff(struct timespec& now,struct timespec& prev)
{
	auto d = (now.tv_sec - prev.tv_sec) * 1000;
	d+=(now.tv_nsec - prev.tv_nsec)/1000000;
	return d;
}
void TMM_MixerThread::Context::do_thread(std::shared_ptr<Context> pctx)
{
	set_realtime_priority();
	auto mix(pctx->config->getMixer());
	auto snd(pctx->config->getAudioIf());
	TMM_Frame tmm_frame;
	std::cout << "UNLOCKED";

	tmm_frame.data_sz(snd->getLatencyInSamples()*2); // in bytes
	do {
#if 0
		static int ticker(0);
		if (ticker % 50 ==0)
		{
//			prev=now;
//			clock_gettime(CLOCK_REALTIME,&now);
//			std::cout << " : " << tmm_frame.data_sz();
//			std::cout << " : " << diff(now,prev)*48;
//			std::cout << " : " << mix->last_read;
//			std::cout << " : " << now.tv_nsec*3/62500 ;
//			std::cout << " : " << snd->getTX_Timestamp() ;
			//std::cout << " : " << snd->getTX_TimestampRaw() ;
			std::cout << std::endl;
		}
		ticker++;
#endif
		tmm_frame.time(snd->getTX_Timestamp());
		snd->Write(mix->Read(tmm_frame));
	} while(!TMM_MixerThread::halt_threads );

}

TMM_MixerThread::TMM_MixerThread () :
							  pcontext (new Context)
{
}


TMM_MixerThread::TMM_MixerThread (Configuration* config_) :
							  pcontext (new Context (config_))
{
}


void TMM_MixerThread::start_thread ()
{
	pcontext->ctx = std::thread (Context::do_thread, pcontext);
}


void TMM_MixerThread::join (void)
{
	return pcontext->ctx.join ();
}


