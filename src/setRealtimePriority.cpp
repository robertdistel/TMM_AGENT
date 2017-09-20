/*
 * setRealtimePriority.cpp
 *
 *  Created on: 18 Sep. 2017
 *      Author: robert
 */


#include <pthread.h>
#include <sched.h>
#include <iostream>

void set_realtime_priority(int prio) {
	int ret;

	// We'll operate on the currently running thread.
	pthread_t this_thread = pthread_self();

	// struct sched_param is used to store the scheduling priority
	struct sched_param params;

	// We'll set the priority to the maximum.
	params.sched_priority = prio;//sched_get_priority_max(SCHED_FIFO);

	// We'll set the priority to the maximum.
	std::cerr << "Trying to set thread realtime prio = " << params.sched_priority << std::endl;

	// Attempt to set thread real-time priority to the SCHED_FIFO policy
	ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
	if (ret != 0) {
		// Print the error
		std::cerr << "Unsuccessful in setting thread realtime prio" << std::endl;
		return;
	}params.sched_priority = sched_get_priority_max(SCHED_FIFO);
	// Now verify the change in thread priority
	int policy = 0;
	ret = pthread_getschedparam(this_thread, &policy, &params);
	if (ret != 0) {
		std::cerr << "Couldn't retrieve real-time scheduling paramers" << std::endl;
		return;
	}

	// Check the correct policy was applied
	if(policy != SCHED_FIFO) {
		std::cerr << "Scheduling is NOT SCHED_FIFO!" << std::endl;
	} else {
		std::cerr << "SCHED_FIFO OK" << std::endl;
	}

	// Print thread scheduling priority
	std::cout << "Thread priority is " << params.sched_priority << std::endl;
}
