/*
 * direct_gpio.cpp
 *
 *  Created on: 2 Oct. 2017
 *      Author: robert
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include "direct_gpio.h"
#include <assert.h>
#include <iostream>
#include "perfmon.h"
#include <time.h>
const char IN_STR[]="in";
const char OUT_STR[]="out";

//magic strings used by /sys/class/gpio driver
constexpr const char * direction(Pin::RW_Flag flag)
{
	return flag==Pin::readonly?IN_STR:OUT_STR;
}

constexpr size_t sizeof_direction(Pin::RW_Flag flag)
{
	return flag==Pin::readonly?sizeof(IN_STR):sizeof(OUT_STR);
}

void Pin::Initialise(void) const
{
	//all this sys/class stuff is shitty and buggy because the underlying implementation relies on udev rules running
	//asynchronously under the synchronous API - thus you cannot rely on anything being set up after a successful operation
	//instead you must keep polling and waiting for the udev rule to run in another process
	//it just makes me throw up in my mouth its so crappy - and this is the ONLY way of accessing the pins without root privileges

	//grab exclusive contrl of the pin for this process
	char buffer[1024];
	const size_t BUFFER_MAX(sizeof(buffer)-1);
	int fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		exit(-1);
	}
	size_t bytes_written = snprintf(buffer, BUFFER_MAX, "%d", bcm_PinNo);
	write(fd, buffer, bytes_written);
	std::cerr << "bcm_PinNo:"<<bcm_PinNo<<std::endl;
	close(fd);



	//set the pin IO direction - we need to spin and wait for the subdirectory to appear after the udev rule runs to create it
	uint32_t counter(0);
	timespec t={0,500000000} ;//500 ms

	snprintf(buffer, BUFFER_MAX, "/sys/class/gpio/gpio%d/direction", bcm_PinNo);
	while(-1==(fd = open(buffer, O_WRONLY)))
	{
		nanosleep(&t,0);

		if (counter >10) //2.5 seconds
			exit(-1);
		counter++;
	}

	if (-1 == write(fd, direction(rw_flag),sizeof_direction(rw_flag)) )
	{
		fprintf(stderr, "Failed to set direction!\n");
		exit(-1);
	}
	std::cerr << "direction(rw_flag):"<<direction(rw_flag)<<std::endl;

	close(fd);

	snprintf(buffer, BUFFER_MAX, "/sys/class/gpio/gpio%d/value", bcm_PinNo);
	if (rw_flag==readonly)
		fd = open(buffer, O_RDONLY);
	else
		fd = open(buffer, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for reading/writing!\n");
		exit(-1);
	}

	//this is semantically const - although some bits are changed - we only initialise on first access - this is effectively a cache
	const_cast<Pin*>(this)->pinValueSysFsFd=fd;	//file descriptor to use for accessing pin

}

Pin::Pin(int BCM_PinNo, RW_Flag flag)
{
	pinValueSysFsFd=0; 	//file descriptor in sysfs space of the "value" file - used for reading/writing the pin state
	rw_flag=flag;
	bcm_PinNo=BCM_PinNo;
	cachedValue=false;
	sub_poll_count=0;
}

bool Pin::value() const
{
	char value_str[3];


	if(rw_flag==writeonly || bcm_PinNo==0) //there is no real pin or we are write only
	{
		return cachedValue; //we are write only - return the last value written
	}


	//readonly - actually read the interface - we have to set it up first if necessary
	if(!pinValueSysFsFd)
		Initialise();
	MON("Pin::value()");
	//we sub sample - only one in every 10 reads is a "real" read - we typically are spinning on every packet in and out. there is a packet every 5 ms on average
	if(sub_poll_count==0)
	{

		int rc;
		if (-1 == lseek(pinValueSysFsFd,0,SEEK_SET))
		{
			fprintf(stderr, "Failed to seek to start of file!\n");
			exit(-1);

		}
		if (-1 == (rc=read(pinValueSysFsFd, value_str, 3))) {
			fprintf(stderr, "Failed to read value!\n");
			exit(-1);
		}
		const_cast<Pin*>(this)->sub_poll_count=1;

		if (atoi(value_str)!=const_cast<Pin*>(this)->cachedValue)
//		std::cerr << "rc" << rc << std::endl;
		std::cerr << "pin value now" << value_str << std::endl;

		const_cast<Pin*>(this)->cachedValue=atoi(value_str);

	}
	const_cast<Pin*>(this)->sub_poll_count--;
	return cachedValue;
}

Pin& Pin::value(bool b)
{
	const static char value_str[3]="01";

	assert(rw_flag==writeonly);
	if(bcm_PinNo==0)
	{
		cachedValue=b; //if we have no real pin defined - just store it
		return *this;
	}

	if(!pinValueSysFsFd)
	{
		Initialise();
		cachedValue=!b;	//set ourselves up so that we always perform the first update
	}
	MON("Pin::value(bool b)");
	if (b!=cachedValue) //writing is relatively expensive - so we only write when we change the actual value
	{
		if (-1 == write(pinValueSysFsFd, b?&value_str[1]:&value_str[0], 1)) {
			fprintf(stderr, "Failed to set value!\n");
			exit(-1);
		}
		cachedValue=b;
	}

	return *this;
}


Pin::~Pin()
{
	if (pinValueSysFsFd)	//we have successfully opened a pin - we need to clean up
	{
		close(pinValueSysFsFd);		//close the pin
		char buffer[1024];
		const size_t BUFFER_MAX(sizeof(buffer)-1);
		int fd = open("/sys/class/gpio/unexport", O_WRONLY);	//and hand control of the pin back to the kernel
		if (-1 == fd) {
			fprintf(stderr, "Failed to open unexport for writing!\n");
			exit(-1);
		}
		size_t bytes_written = snprintf(buffer, BUFFER_MAX, "%d", bcm_PinNo);
		write(fd, buffer, bytes_written);
		close(fd);

	}
}
