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
#include <boost/program_options.hpp>
#include <cstdint>


namespace po = boost::program_options;




const char DefaultConfigTypesHelp[] =
		"Default Configurations : \n"
		"0)  Operator Configuration. Mute/HSD  = pin 7\n"
		"1)  Radio With No Crypto. PTT = pin 31. CODAN = pin 7 \n"
		"2)  Crypto Radio. PTT = pin 31, CODAN = vox, Cypher Text Enable = pin 35, Cypher Text Active set to pin 7 \n";


int main(int argc, char** argv)
{
	Configuration config;
	uint16_t defaultConfigType=0;

	po::options_description description ("MyTool Usage");
	description.add_options ()
						("base-type,t",po::value<uint16_t> (&defaultConfigType), DefaultConfigTypesHelp)
						("help,h", "Display this help message")
						("version,v", "Display the version number");
	config.add_options(description);
	po::variables_map vm;
	po::store (po::command_line_parser (argc, argv).options (description).run (), vm);
	po::notify (vm);




	if (vm.count ("help"))
	{
		std::cout << description;
		return -1;

	}

	if (vm.count ("version"))
	{
		std::cout << "TMM_Agent Version 1.0" << std::endl;
		return -1;
	}

	if (!config.initialise(static_cast<Configuration::DefaultConfigTypes>(defaultConfigType)))
	{
		std::cout << description;
		return -1;

	}



	OpenSSL OpenSSL_instance;
	TMM_DisplayThread display_thread(&config);
	TMM_InputThread input(&config);
	TMM_OutputThread output(&config);
	TMM_MixerThread mixer(&config);
	output.start_thread();
	display_thread.start_thread();
	input.start_thread();
	mixer.start_thread();
	while(!config.shutdown)
	{
		sleep (1);
	}
	input.stop_thread();
	output.stop_thread();
	mixer.stop_thread();
	display_thread.stop_thread();


}
