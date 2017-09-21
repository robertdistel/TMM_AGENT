/*
 * TMM_InputThread.h
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */

#ifndef TMM_INPUTTHREAD_H_
#define TMM_INPUTTHREAD_H_
#include <memory>

class Configuration;

class TMM_InputThread
{
private:

  class Context;
  std::shared_ptr<Context> pcontext;

public:



  TMM_InputThread (Configuration* config_) ;


  void start_thread ();
  void stop_thread();


};



#endif /* TMM_INPUTTHREAD_H_ */
