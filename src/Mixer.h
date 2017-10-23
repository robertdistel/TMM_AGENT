/*
 * Mixer.h
 *
 *  Created on: 15 Sep. 2017
 *      Author: robert
 */

#ifndef MIXER_H_
#define MIXER_H_

#include <vector>
#include <cstdint>
class TMM_Frame;

class Mixer
{
private:

	std::vector<uint16_t> sample_buffer;
	uint16_t target_latency;
	uint16_t samples_per_ms;
	uint32_t buff_sz;
	uint32_t last_read;
	int32_t phase;



public:
	uint32_t rx_last_active;

	TMM_Frame&  Read (TMM_Frame& tmm_frame);
	const TMM_Frame&  Write (const TMM_Frame& tmm_frame);
	Mixer(uint16_t target_latency,uint32_t sample_rate);

};



#endif /* MIXER_H_ */
