/*
 * raspi_high_precision_timer.cpp
 *
 *  Created on: 13 Sep. 2017
 *      Author: robert
 */

#include <unistd.h>
#include <time.h>
#include <cstdint>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "Dense"
#include <iostream>
#include <iomanip>
#include "HardwareTimer.h"




class HardwareTimerImpl
{
private:

	//variables used for the kalman filter
	Eigen::Matrix2d 	P;	//variance of parameters
	Eigen::Vector2d 	X;  //the parameters
	double				R;
	Eigen::Matrix2d		Q;	//variation in state - it doesnt change very much - only as the underlying h/w drifts

	double 				eps;

	//variables used to deal with hw/counter wrapping
	uint64_t			last_hw_count;
	uint64_t			unwrapped_base;
	uint64_t			wrap;
	uint32_t			epoch;







public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
	HardwareTimerImpl(uint32_t nominal_tick_rate) ;
	~HardwareTimerImpl();

	uint32_t getEpoch() const { return epoch;}
	void setWrap(uint64_t wrap_){wrap=wrap_;}						//(re)set the wrap of the hw/counter - set to 0 for no wrap
	void reset(uint32_t nominal_tick_rate);										//reset the filter to unlocked - the hw/counter has glitched so we need to start trying to get a lock
	double getTime(const uint64_t& hw_counter) ;			//now use the hw-counter to get the real time precisely


	void updateEstimator(const uint64_t& hw_counter, const struct timespec& rtc); 	//we maintain a high precision estimator using the system rtc and a hardware counter
	//the counter wraps every wrap ticks - note that rtc is in 32.32 fixed point form (seconds)
};

#if 0
#define TIMER_OFFSET (4)

#define PI_ZERO 0
#define PI_2 2
#define PI_3 3
#define PI_CMM3 3

#define BOARD_TYPE PI_ZERO

#if BOARD_TYPE==PI_ZERO
#define ST_BASE (0x20003000)
#endif

#if BOARD_TYPE==PI_2 || BOARD_TYPE==PI_3
#define ST_BASE (0x3F003000)
#endif

uint16_t HardwareTimer::getTime()
{
	if(!initialised)
		Initialise();


	uint64_t t=*timer;
	if((t - last_updated) > 1000000)
	{
		last_updated = t;
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME,&ts);

		UpdateEstimator(t,ts);
		last_updated=t;
	}

	return uint16_t(1000*(X(1)*double(*timer % 1000000000000)/1000000 + X(2))); //this will wrap around - effectively modulo
}

void HardwareTimer::InitialiseHW_MHzCounter(void)  //this method requires root privaledges as it access /dev/mem directly
{
	void *st_base; // byte ptr to simplify offset math

	// get access to system core memory
	if (-1 == (fd = open("/dev/mem", O_RDONLY))) {
		fprintf(stderr, "open() failed.\n");
		return;
	}

	// map a specific page into process's address space
	if (MAP_FAILED == (st_base = mmap(NULL, 4096,
			PROT_READ, MAP_SHARED, fd, ST_BASE))) {
		fprintf(stderr, "mmap() failed.\n");
		return ;
	}

	// set up pointer, based on mapped page
	timer = (volatile uint64_t *)((char *)st_base + TIMER_OFFSET);

	// read initial timer
	initialised=true;
}
#endif


HardwareTimer::HardwareTimer():timer(new HardwareTimerImpl(48000))
{
}

float HardwareTimer::getTime(const uint64_t& counter, uint32_t* epoch) const
{
	if (epoch)
		*epoch=timer->getEpoch();
	return timer->getTime(counter);
}

void HardwareTimer::updateEstimator(const uint64_t& hw_counter, const struct timespec& rtc)
{
	timer->updateEstimator( hw_counter,  rtc);
}

void HardwareTimer::resetEstimator(uint32_t sample_rate)
{
	timer->reset(sample_rate);
}


const float s(1);
const float ms(1e-3);
const float us(1e-6);
const float ns(1e-9);


HardwareTimerImpl::HardwareTimerImpl(uint32_t nominal_tick_rate)
{
	R=pow((50 * ms),2);			//initialise the measurement error of the real time clock, 50ms rms


	reset(nominal_tick_rate);
}

void HardwareTimerImpl::reset(uint32_t nominal_tick_rate)
{
	Q(0,0)=pow((s/nominal_tick_rate  * 1e-6),2); //1ppm / s drift
	Q(1,0)=0;
	Q(0,1)=0;
	Q(1,1)=pow((1 * ms),2); 	// the starting time can jump around a little - there is some rtc jump from chrony 1ms / sec
	// but the scaling factor not so much - assume tick rate is within 10%
	eps = Q(1,1)*10;			//the aceptable amount of error for the starting point - causes an update every to ticks
	X(0)=s/nominal_tick_rate;	//default starting point - h/w ticker ticks in 48K ticks per second
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);

	epoch=ts.tv_sec;
	epoch = epoch /100;
	epoch = epoch * 100; //epoch accurate to within 10 seconds;
	X(1)=float(ts.tv_sec - epoch)*s + float(ts.tv_nsec)*ns;				// starting from now
	P=Q*1e18;
	P(0,1)=1;
	P(1,0)=1;
	last_hw_count = 0;
	unwrapped_base = 0;


}

HardwareTimerImpl::~HardwareTimerImpl()
{
}


double HardwareTimerImpl::getTime(const uint64_t& hw_counter)
{
//	if (hw_counter < last_hw_count) //we have wrapped the hw counter
//		unwrapped_base+=wrap;		//extend our unwrapped base to keep the count going
//
	last_hw_count=hw_counter;
	double t(X(0)*double(hw_counter+unwrapped_base)+X(1));
	static int ticker(0);
//	if (ticker % 50 == 0)
//	{
//		struct timespec now;
//		clock_gettime(CLOCK_REALTIME,&now);
//		now.tv_sec-=epoch;
//		double z=double(now.tv_sec) + double(now.tv_nsec)*1e-9;
//		std::cout << std::setprecision(std::numeric_limits<double>::digits10 + 1) << fmod(z*8000,8000) << std::endl;
//		std::cout << std::setprecision(std::numeric_limits<double>::digits10 + 1) << fmod(t*8000,8000) << std::endl;
//	}
//
//	ticker++;
	return t;
}

void HardwareTimerImpl::updateEstimator(const uint64_t& hw_counter, const struct timespec& rtc)
{
//	static uint32_t ticker(0);
	if (hw_counter < last_hw_count) //we have wrapped the hw counter
		unwrapped_base+=wrap;		//extend our unwrapped base to keep the count going
//	auto diff = hw_counter - last_hw_count;
	last_hw_count = hw_counter;

	//prediction


	P = P + Q;

	//if(P(1,1) > eps ) //error is to great - trying to lock in the filter - otherwise skip all this, its good enough for the time being
	{

		Eigen::RowVector2d 	H;
		double Z = double(rtc.tv_sec - epoch) * s +double(rtc.tv_nsec)*ns;
		H(0)=double(hw_counter + unwrapped_base);
		H(1)=double(1);
		auto Y=Z-H*X;
		auto S=R+H*P*H.transpose();
		auto K=(P*H.transpose())/S;
		X=X+K*Y;
		P=(Eigen::Matrix2d().setIdentity()-K*H)*P;
//		if (ticker % 50 ==0)
		{
//			std::cout << "Z = " << Z << " Y =" << Y << " H(0) = " << H(0) <<std::endl;
//			std::cout <<"P = \n"<< P << std::endl;
//			std::cout <<"X = \n" << X << std::endl;
		}
	}
//	ticker+=1;
}




