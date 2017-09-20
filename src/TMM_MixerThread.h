/*
 * TMM_MixerThread.h
 *
 *  Created on: 15 Sep. 2017
 *      Author: robert
 */

#ifndef TMM_MIXERTHREAD_H_
#define TMM_MIXERTHREAD_H_

#include <memory>
class Configuration;

class TMM_MixerThread
{
private:

	class Context;
	std::shared_ptr<Context> pcontext;



public:

	TMM_MixerThread () ;


	TMM_MixerThread (Configuration* config_) ;


	void start_thread ();

	void join (void);

	static bool halt_threads;


};




#endif /* TMM_MIXERTHREAD_H_ */
