/*
 * perfmon.cpp
 *
 *  Created on: 20 Sep. 2017
 *      Author: robert
 */
#include  <unistd.h>
#include <cstdint>
#include <vector>
#include <time.h>
#include <assert.h>
#include <map>
#include "perfmon.h"


const char* uninitialised_name="UNINITIALISED";

struct perfmon_event
{

//	perfmon_event():idx(0),entered_at(0),accounted_child_time(0){}
	uint16_t	idx;
	uint64_t	entered_at;
	uint64_t	accounted_child_time;
};

struct perfmon_statistics
{
public:
//	perfmon_statistics():lapsed_time(0),self_time(0),min(0),max(0),call_count(0),name(_blank){}
	uint64_t	lapsed_time;
	uint64_t	self_time;
	uint64_t	min;
	uint64_t	max;
	uint64_t    call_count;
	const char* name;
};


__thread perfmon_statistics 		stats[0x010000L];
__thread perfmon_event  			event_stack[2048];
__thread uint16_t 					stack_ptr=1;
__thread bool 						initialised=false;





Perfmon::Perfmon(uint16_t idx_, const char* name)
{
	idx=idx_;
	if (!initialised)
	{
		for (size_t k=0; k<(sizeof(stats)/sizeof(stats[0])); k++)
		{
			stats[k].lapsed_time=0;
			stats[k].self_time=0;
			stats[k].min=UINT64_MAX;
			stats[k].max=0;
			stats[k].call_count=0;
			stats[k].name=uninitialised_name;
		}
		initialised=true;
	}
	if (stats[idx].name==uninitialised_name)
		stats[idx].name=name;

	assert(stats[idx].name==name);

	perfmon_event ev;
	ev.idx=idx;
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC,&now);
	ev.entered_at=1000000000LL * now.tv_sec + now.tv_nsec;
	ev.accounted_child_time=0;
	event_stack[stack_ptr++]=ev;
	assert(stack_ptr<2048);
}

 Perfmon::~Perfmon(void)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC,&now);
	perfmon_event ev=event_stack[--stack_ptr];
	assert (idx == ev.idx); //otherwise we have screwed up
	uint64_t exited_at=1000000000LL * now.tv_sec + now.tv_nsec;
	uint64_t lapsed_time=exited_at-ev.entered_at;
	uint64_t self_time=lapsed_time - ev.accounted_child_time;
	event_stack[stack_ptr-1].accounted_child_time+=lapsed_time;
	stats[idx].lapsed_time+=lapsed_time;

	stats[idx].self_time+=self_time;
	stats[idx].call_count++;
	stats[idx].min=std::min(stats[idx].min,lapsed_time);
	stats[idx].max=std::max(stats[idx].max,lapsed_time);
}

void Perfmon::dump(const char* thread_name)
{
	std::multimap<uint64_t,uint8_t> sort_by_lapsed_time;
	std::multimap<uint64_t,uint8_t> sort_by_self_time;


	printf("Dumping thread %s \n",thread_name);
	for (size_t k=0; k<(sizeof(stats)/sizeof(stats[0])); k++)
	{
		if (stats[k].name!=uninitialised_name)
		{
			sort_by_lapsed_time.emplace(stats[k].lapsed_time,k);
			sort_by_self_time.emplace(stats[k].self_time,k);
		}
	}
	printf("\n%-30.30s %10.10s %10.10s %10.10s %10.10s %10.10s %10.10s \n","name","lapsed","self","min","max","avg","count");
	for (auto k=sort_by_lapsed_time.rbegin(); k!=sort_by_lapsed_time.rend(); k++)
	{
		auto s(stats[k->second]);
		printf("%-30.30s %10f %10f %10f %10f %10f %10lu \n" ,
				s.name,double(s.lapsed_time)/1e9,double(s.self_time)/1e9,double(s.min)/1e9,double(s.max)/1e9,double(s.lapsed_time/s.call_count)/1e9,long(s.call_count));
	}

	printf("\n%-30.30s %10.10s %10.10s %10.10s %10.10s %10.10s %10.10s \n","name","lapsed","self","min","max","avg","count");
	for (auto k=sort_by_self_time.rbegin(); k!=sort_by_self_time.rend(); k++)
	{
		auto s(stats[k->second]);
		printf("%-30.30s %10f %10f %10f %10f %10f %10lu \n",
				s.name,double(s.lapsed_time)/1e9,double(s.self_time)/1e9,double(s.min)/1e9,double(s.max)/1e9,double(s.lapsed_time/s.call_count)/1e9,long(s.call_count));
	}
	printf("\n\n");
}


