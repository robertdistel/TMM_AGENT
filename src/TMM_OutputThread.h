/*
 * TMM_OutputThread.h
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */

#ifndef TMM_OUTPUTTHREAD_H_
#define TMM_OUTPUTTHREAD_H_
#include <memory>

class Configuration;

class TMM_OutputThread
{
private:

  class Context;
  std::shared_ptr<Context> pcontext;

public:





  TMM_OutputThread (Configuration* config_) ;


  void start_thread ();

  void stop_thread ();


};



#endif /* TMM_OUTPUTTHREAD_H_ */
