#ifndef __SERVER__
#define __SERVER__

#include <sys/socket.h> // socket
#include <netinet/in.h> // sockaddr_in
#include <netdb.h> // getservbyname
#include <stdlib.h> // atoi
#include <string>
#include "console.hpp"
#include "shell.hpp"

class server
{
public:
	
	static int passivsock(std::string service, std::string protocol = "tcp")
	{
		struct servent *p_server_info;
		struct protoent *p_protocol_info;
		struct sockaddr_in sin;
		int socket_fd, socket_type;
		
		memset((void*) &sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		
		///@brief set service (port number)
		p_server_info = getservbyname(service.c_str(), protocol.c_str());
		if( p_server_info )
			sin.sin_port = htons( ntohs( (u_short)(p_server_info->s_port) ) );
		else
			sin.sin_port = htons( (u_short)( atoi(service.c_str()) ) );
		if(sin.sin_port == 0)
			console::error("Can't get service entry");
		
		///@brief set protocol (udp or tcp)
		p_protocol_info = getprotobyname(protocol.c_str());
		if(p_protocol_info == 0)
			console::error("Can't get protocol entry");
		if(protocol == "udp")
			socket_type = SOCK_DGRAM;
		else
			socket_type = SOCK_STREAM;
		
		///@brief allocate a socket
		socket_fd = socket(PF_INET, socket_type, p_protocol_info->p_proto);
		if(socket_fd < 0)
			console::error("Can't create socket");
		if( bind(socket_fd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
			return -1;//console::error("Can't bind socket");
		if(socket_type == SOCK_STREAM)
			if(listen(socket_fd, 0)<0)
				console::error("Can't listen socket");
		return socket_fd;
	}
	
	
};

#endif