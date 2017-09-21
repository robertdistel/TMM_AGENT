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


	TMM_MixerThread (Configuration* config_) ;


	void start_thread ();

	void stop_thread();



};




#endif /* TMM_MIXERTHREAD_H_ */
