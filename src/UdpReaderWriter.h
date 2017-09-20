/*
 * UdpReaderWriter.h
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */

#ifndef UDPREADERWRITER_H_
#define UDPREADERWRITER_H_


#include <string>

class TMM_Frame;


class UdpReaderWriter
{
public:
	UdpReaderWriter (void);

	UdpReaderWriter (std::string socket_details_);


  TMM_Frame&  Read (TMM_Frame& frame);
  const TMM_Frame&  Write (const TMM_Frame& frame);

  virtual ~UdpReaderWriter ();
private:
  int s_in;
  int s_out;
  std::string socket_details;
};




#endif /* UDPREADERWRITER_H_ */
