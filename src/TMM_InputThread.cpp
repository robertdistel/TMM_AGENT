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
#include "direct_gpio.h"




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
	while(!pctx->config->timebase_locked)
		sleep(1);
	set_realtime_priority();
	auto mix(pctx->config->getMixer());
	auto pkt(pctx->config->getPktIf());
	auto tmr(pctx->config->captureTimer);
	Crypto crypto;
	Pin cypherModeRequest(pctx->config->cypherModeRequestPin,Pin::writeonly);
	Pin cypherModeActive(pctx->config->cypherModeActivePin,Pin::readonly);
	Pin PTT(pctx->config->analogueOutModePin,Pin::writeonly);
	uint8_t			active_source_idx(0);
	float			active_source_timestamp(0); //seconds precision
	int32_t			active_source_pwr(-127);


	do {
		{
			MON("TMM_InputThread:MainLoop");
			TMM_Frame tmm_frame;
			tmm_frame = pkt->Read(tmm_frame);

			//if we are a radio we need to do some PTT arbitration first
			//we dont want to process anything that is not the current source
			//if this is the ACTIVE source or we are not a radio proceed

			bool isCyphertext(tmm_frame.domain_ID()==pctx->config->cyphertext_domain);
			bool isPlaintext(tmm_frame.domain_ID()==pctx->config->plaintext_domain);

			if(tmm_frame.packet_sz()==0 )	//timeout - there has been no activity for a while (500ms nominally)
				//so we turn the ptt off
			{
				if ( pctx->config->analogueOutMode==Configuration::ptt)
				{
					PTT=false;	//only change PTT state if we have ptt mode enabled
				}
				continue;
			}

			//do we need to do any arbitration of input audio - if we are receiving from the active source or we are just mixing - we need to store the inbound audio
			if(tmm_frame.stream_idx()==active_source_idx || pctx->config->mixMode == Configuration::mix)
			{
				//we only do any of these checks if check_cyphertext_enabled is set - ie. we are in radio mode
				//we set the CypherTextEnable pin - to tell the radio we want to send in crypto if we are not receiving packets in the plaintext domain
				//and we check to see if the radio has driven back the cyphertext active pin - showing the radio has the crypto enabled


				if (pctx->config->secureRadioMode != Configuration::no_crypto)
					cypherModeRequest=isCyphertext; //we have a secure radio - drive the CTE pin

				if(pctx->config->secureRadioMode != Configuration::no_crypto && 					//we have an external crypto
						(
								!( isCyphertext || isPlaintext) || //its not in a domain the radio is handling or
								(
										pctx->config->secureRadioMode == Configuration::CypherTextHandshake  && //  we need handshaking
										(	cypherModeRequest != cypherModeActive )	//the state we are asserting must match what we are detecting

								)
						)
				)
				{
					if ( pctx->config->analogueOutMode==Configuration::ptt)
					{
						PTT=false;	//only change PTT state if we have ptt mode enabled
					}
					//stop keying if the active source is in the wrong domain - or otherwise not ready yet
					continue;   //we do nothing else
				}
				//we dont decrypt unless the radio has confirmed its secure
				//update the metrics about the active source
				active_source_pwr=(active_source_pwr*1023 + tmm_frame.pwr())/1024; //update power estimator
			}
			else
			{
				//not the active source and we are a radio - need to decide if we should flick over
				//if the new frame has a much greater power - we instantly flick over
				//or if the old active source has not transmitted for at least 100ms
				//or if the new frame power exceeds the active source by 3dB during the first 200 frames (2 seconds)
				if (tmm_frame.pwr() - active_source_pwr >  30
						|| tmr.getTime()-active_source_timestamp>.1) //
				{
					active_source_pwr=tmm_frame.pwr();
					active_source_idx=tmm_frame.stream_idx();

					cypherModeRequest=isCyphertext;
					if(pctx->config->secureRadioMode != Configuration::no_crypto && 					//we have an external crypto
							(!( isCyphertext || isPlaintext) || //its not in a domain the radio is handling or
									( pctx->config->secureRadioMode == Configuration::CypherTextHandshake  && //  we need handshaking
											(	cypherModeRequest != cypherModeActive )	//the state we are asserting must match what we are detecting

									)
							)
					)
					{
						if ( pctx->config->analogueOutMode==Configuration::ptt)
						{
							PTT=false;	//only change PTT state if we have ptt mode enabled
						}
							//stop keying if the active source is in the wrong domain - or otherwise not ready yet
						continue;   //we do nothing else
					}
					//we dont decrypt unless the radio has confirmed its secure

				}
				else
				{
					continue;
				}
			}

			if ( pctx->config->analogueOutMode==Configuration::ptt)
			{
				PTT=true;	//only change PTT state if we have ptt mode enabled
			}

			active_source_timestamp=tmr.getTime();
			tmm_frame= crypto.decrypt(tmm_frame);
			auto  codec(pctx->config->getCodec(tmm_frame.stream_idx()));
			mix->Write(codec->expand(tmm_frame).set_gain());

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


