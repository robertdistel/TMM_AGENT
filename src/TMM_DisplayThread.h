/*
 * TMM_DisplayThread.h
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */

#ifndef TMM_DISPLAYTHREAD_H_
#define TMM_DISPLAYTHREAD_H_
#include <memory>

class Configuration;

class TMM_DisplayThread
{
private:

  class Context;
  std::shared_ptr<Context> pcontext;

public:


  TMM_DisplayThread () ;


  TMM_DisplayThread (Configuration* config_) ;


  void start_thread ();

  void  join (void);

  static bool halt_threads;

};




#endif /* TMM_DISPLAYTHREAD_H_ */
