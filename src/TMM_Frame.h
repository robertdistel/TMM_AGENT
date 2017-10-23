/*
 * TMM_Frame.h
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */

#ifndef TMM_FRAME_H_
#define TMM_FRAME_H_
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

class UdpReaderWriter;
class AlsaReaderWriter;
class Codec;
class Mixer;



//a TMM frame must be big enough to contain two full expanded audio blocks (to deal with error compensation and silence suppression
//at 48k samples, so 48 (samples per ms) * 2 (16 bit samples) * 50ms * 2 = 9600 bytes - after compression these frames are a lot smaller - we wont need to fragment at the UDP layer
class TMM_Frame
{
public:
	static const uint16_t maxDataSize = 9600;

private:
#pragma pack(1) //set byte alignment
	struct header_t
	{
		uint8_t		stream_idx:8;	//stream index as set by the TMM_CTRL agent
		uint16_t    packet_sz:16;	//how large the packet is in bytes
		uint8_t		domain_ID:8;	//the security domain ID of this frame - used to index the key
		bool		encrypted:8;
		int8_t		pwr:8;			//stream rms pwr in db - excludes gain to be applied - should be weighted mean over previous 250ms +16..-112dB
		bool		linear:8;
		uint8_t     gain:8;			//gain to be applied to stream prior to rendering - apply to this frame -+64dB to -64dB of gain
		uint8_t     iv[16];			//initialisation vector used for encryption + time and stream_ctr;
		uint16_t    time:16;		//the real time in samples of when this packet was created - should be stable across the network as it is used to deal with network delay
		uint32_t    stream_ctr:24;	//pkt count of this stream
	};

	struct frame_t
	{
		header_t 	header;
		uint8_t     data[maxDataSize];	//the actual payload to send
	};


#pragma pack()	//restore default alignment
	std::shared_ptr<frame_t>	pFrame;

public:
	TMM_Frame();
	TMM_Frame(const TMM_Frame & rhs);

	uint16_t	packet_sz() const;
	uint16_t	data_sz() const {return packet_sz() - sizeof(header_t) ;}
	uint8_t		domain_ID() const;
	uint16_t   	time() const;
	TMM_Frame&	packet_sz(uint16_t);
	TMM_Frame&	data_sz(uint16_t sz){return packet_sz(sz + sizeof(header_t));}
	TMM_Frame&	domain_ID(uint8_t);
	TMM_Frame&  time(uint16_t);
	TMM_Frame&	IV(const uint8_t*); //only sets part of the IV - as we use the counter and timestamp as part of the IV
	const uint8_t*	IV() const;     //returns a pointer to the full 16byte IV, includes the counter and timestamp
	uint8_t*	data();
	const uint8_t*	data() const;
	uint8_t*	frame();
	const uint8_t*	frame() const;
	bool		linear() const;
	bool		plaintext() const;
	TMM_Frame&	linear(bool b);
	TMM_Frame&	plaintext(bool b);
	uint8_t 	stream_idx() const;
	TMM_Frame& 	stream_idx(uint8_t idx);
	uint32_t 	stream_ctr() const;
	TMM_Frame& 	stream_ctr(uint32_t ctr);
	TMM_Frame&  set_pwr(); //measure the frames power (if linear and non-encrypted) and set the pwr field
	TMM_Frame&  pwr(int8_t p);
	int8_t		pwr() const;
	TMM_Frame&  gain(int8_t g);
	int8_t		gain() const;
	TMM_Frame&  set_gain(); //apply the frames gain setting - clearing it and updating the frames linear values accordingly


	static const uint16_t maxFrameSize = sizeof(frame_t);
	void Dump(void);


};



#endif /* TMM_FRAME_H_ */
