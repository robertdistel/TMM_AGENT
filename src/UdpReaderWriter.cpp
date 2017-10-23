/*
 * UdpReaderWriter.cpp
 *
 *  Created on: 8 Sep. 2017
 *      Author: robert
 */



#include "UdpReaderWriter.h"
#include "TMM_Frame.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>          /* hostent struct, gethostbyname() */
#include <unistd.h>

#include <regex>
#include <iostream>
#include <stdlib.h>
#include <string>
#include "perfmon.h"

UdpReaderWriter:: UdpReaderWriter (void) :
s_in(0),s_out(0), socket_details ()
{
}

UdpReaderWriter::UdpReaderWriter (std::string socket_details_) :
						s_in(0),s_out(0), socket_details (socket_details_)
{
}


TMM_Frame&  UdpReaderWriter::Read (TMM_Frame& tmm_frame)
{

	if (!s_in)
	{
		struct sockaddr_in reader_socket_addr;

		if ((s_in = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		{
			std::cerr << "socket failed" << std::endl;
			exit (-1);
		}


		std::smatch sm;
		std::regex rex ("([^:]*):(.*)");
		std::regex_match (socket_details, sm, rex);
		auto destination_name = std::string (sm[1]);
		auto port = atoi (std::string (sm[2]).c_str ());

		/* Construct the server sockaddr_in structure */
		memset (&reader_socket_addr, 0, sizeof(reader_socket_addr));/* Clear struct */
		reader_socket_addr.sin_family = AF_INET; 					/* Internet/IP */
		reader_socket_addr.sin_addr.s_addr = htonl (INADDR_ANY); 	/* Incoming addr */
		reader_socket_addr.sin_port = htons (port); 				/* server port */

		if (bind (s_in, (struct sockaddr *) &reader_socket_addr, sizeof(reader_socket_addr)) < 0)
		{
			std::cerr << "bind failed" << std::endl;
			exit (-1);
		}
		//read will return 2 times per second with 0 length reads if its idle - we do this after the bind as it may call resolv which is necessarily slow
		struct timeval tv;
		tv.tv_sec = 0;  /* 1/2 Sec Timeout */
		tv.tv_usec = 500000;  // Not init'ing this can cause strange errors
		setsockopt(s_in, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

	}

	if (!s_in)
	{
		std::cerr << "Couldn't open recv udp port on port " << socket_details << std::endl;
		exit (-1);
	}

	MON("UdpReaderWriter::Read");

	int recieved_bytes (0);
	WRAP("udp recv");
	if ((recieved_bytes = recv (s_in, tmm_frame.frame (), TMM_Frame::maxFrameSize, 0)) < 0)
	{
		recieved_bytes = 0;
		//most likely we got a SIG_INTR to unblock waiting for data.
	}
	END();
	tmm_frame.packet_sz(recieved_bytes); //and resize buffer to reflect how many bytes were really read
	return tmm_frame;
}

const TMM_Frame&  UdpReaderWriter::Write (const TMM_Frame& tmm_frame)
{
	if (!s_out)
	{
		std::smatch sm;
		std::regex rex ("([^:]*):(.*)");
		std::regex_match (socket_details, sm, rex);
		auto destination_name = std::string (sm[1]);
		auto port = atoi (std::string (sm[2]).c_str ());

		struct sockaddr_in writer_destination_addr;

		if ((s_out = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		{
			std::cerr << "open socket failed" << std::endl;
			exit (-1);
		}

		struct hostent *host; /* host information */
		if ((host = gethostbyname (destination_name.c_str ())) == NULL)
		{
			std::cerr << "name lookup failed on '" << destination_name << ":" << port << "'" << std::endl;
			exit (1);
		}

		/* Construct the server sockaddr_in structure */
		memset (&writer_destination_addr, 0, sizeof(writer_destination_addr)); /* Clear struct */
		writer_destination_addr.sin_family = AF_INET; /* Internet/IP */
		writer_destination_addr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]); /* Incoming addr */
		writer_destination_addr.sin_port = htons (port); /* server port */

		if (connect (s_out, (struct sockaddr *) &writer_destination_addr, sizeof(writer_destination_addr)) < 0)
		{
			std::cerr << "connect failed" << std::endl;
			exit (-1);
		}

	}

	if (!s_out)
	{
		std::cerr << "Couldn't open tx udp port on '" << socket_details << "'" << std::endl;
		exit (-1);
	}

	MON("UdpReaderWriter::Write");
	if (tmm_frame.data_sz()==0)
		return tmm_frame; //there is infact no audio to send - DTX detected - so send nothing

	ssize_t rc=0;
	if ((rc = send (s_out, tmm_frame.frame(), tmm_frame.packet_sz (), 0)) != int (tmm_frame.packet_sz ()))
	{
		if (rc<0)
			std::cerr << "Send failed with erno " << strerror(errno) << std::endl;
		else
			std::cerr << "Send sent to little data. sent "<< rc << " expected " <<  tmm_frame.packet_sz () << std::endl;
	}
	return tmm_frame;
}

UdpReaderWriter::~UdpReaderWriter ()
{
	if (s_in)
		close (s_in);
	if (s_out)
		close (s_out);
}

