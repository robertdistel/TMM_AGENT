/*
 * HardwareTimer.h
 *
 *  Created on: 15 Sep. 2017
 *      Author: robert
 */

#ifndef HARDWARETIMER_H_
#define HARDWARETIMER_H_
#include <memory>
class HardwareTimerImpl;

//allow the use of a precise hardware counter (ie. sample counter) with the rtc to extend the accuracy of the RTC
//use with the sample count from AD/DA converters for instance to deal with slight errors in sample rate
class HardwareTimer
{
private:
	std::shared_ptr<HardwareTimerImpl> timer;
public:
	HardwareTimer();

	float getTime(const uint64_t& counter, uint32_t* epoch=nullptr) const;

	void updateEstimator(const uint64_t& hw_counter, const struct timespec& rtc);

	void resetEstimator(uint32_t sample_rate);

};


#endif /* HARDWARETIMER_H_ */
