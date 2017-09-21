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



//a TMM frame must be big enough to contain the full expanded audio block - so 512- bytes->125 samples->15.625ms at 8kHz, 7.8125 at 16kHz
class TMM_Frame
{
public:
	static const uint16_t maxDataSize = 3000;

private:
#pragma pack(1) //set byte alignment
	struct header_t
	{
		uint16_t    packet_sz:16;	//how large the packet is in bytes
		uint8_t		domain_ID:8;	//the security domain ID of this frame - used to index the key
		bool		encrypted:8;
		bool		linear:8;
		uint16_t    time:16;		//the real time in samples of when this packet was created - should be stable across the network as it is used to deal with network delay
//		uint8_t    	duration:8;		//the real time in ms of the length of the media in this packet
		uint8_t     iv[16];			//initialisation vector used for encryption
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

	TMM_Frame& 	compress(Codec& compressor);
	TMM_Frame& 	expand(Codec& expander);
	uint16_t	packet_sz() const;
	uint16_t	data_sz() const {return packet_sz() - sizeof(header_t) ;}
	uint8_t		domain_ID() const;
	uint16_t   	time() const;
	TMM_Frame&	packet_sz(uint16_t);
	TMM_Frame&	data_sz(uint16_t sz){return packet_sz(sz + sizeof(header_t));}
	TMM_Frame&	domain_ID(uint8_t);
	TMM_Frame&  time(uint16_t);
	uint8_t*	IV();
	const uint8_t*	IV() const;
	uint8_t*	data();
	const uint8_t*	data() const;
	uint8_t*	frame();
	const uint8_t*	frame() const;
	bool		linear() const;
	bool		plaintext() const;
	TMM_Frame&	linear(bool b);
	TMM_Frame&	plaintext(bool b);

	static const uint16_t maxFrameSize = sizeof(frame_t);
	void Dump(void);


};



#endif /* TMM_FRAME_H_ */
