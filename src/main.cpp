/*
 * main.cpp
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */
#include "Configuration.h"
#include "TMM_DisplayThread.h"
#include "TMM_InputThread.h"
#include "TMM_OutputThread.h"
#include "TMM_MixerThread.h"
#include "OpenSSL_Wrapper.h"
#include <iostream>
#include <unistd.h>





int main(int argc, char** argv)
{
	  Configuration config;
	  OpenSSL OpenSSL_instance;
	  TMM_DisplayThread display_thread(&config);
	  TMM_InputThread input(&config);
	  TMM_OutputThread output(&config);
	  TMM_MixerThread mixer(&config);
	  output.start_thread();
	  display_thread.start_thread();
	  input.start_thread();
	  mixer.start_thread();
	  for(uint32_t k=0; k<30; k++)
	    {
	  	    sleep (1);
	  	    std::cout << k << std::endl;

	    }
	  input.stop_thread();
	  output.stop_thread();
	  mixer.stop_thread();
	  display_thread.stop_thread();


}
