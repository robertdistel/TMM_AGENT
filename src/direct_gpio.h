/*
 * direct_gpio.h
 *
 *  Created on: 2 Oct. 2017
 *      Author: robert
 */

#ifndef DIRECT_GPIO_H_
#define DIRECT_GPIO_H_

class Pin
{
public:
	enum RW_Flag {readonly, writeonly} ;
	operator bool() const { return value(); }
	Pin& operator=(bool rhs) { return value(rhs); }

	bool value() const;
	Pin& value(bool b);

	int BCM_PinNo();

	Pin(int BCM_PinNo, RW_Flag flag);
	~Pin();

private:
	int pinValueSysFsFd; 	//file descriptor in sysfs space of the "value" file - used for reading/writing the pin state
	RW_Flag rw_flag;
	int bcm_PinNo;
	void Initialise( void ) const;
	bool cachedValue;
	int  sub_poll_count;
};



#endif /* DIRECT_GPIO_H_ */
