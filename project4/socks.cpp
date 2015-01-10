#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/uio.h>
#include <cstdio>
#include <array>

#include <string>
#include <vector>
#include <map>
#include <fstream>

#include <iostream>
#include <functional>
#include "server.hpp"

struct REQUEST_INFO
{
	uint8_t vn;
	uint8_t cd;
	uint16_t port;
	uint32_t ip;
	// 8 bytes
	std::string user_id;
	std::string domain_name;
};

struct Client
{
	int sockfd;
	bool is_connected;
	uint16_t port;
	uint32_t ip;
	struct sockaddr_in server_addr;
};
struct Permit_ip
{
	Permit_ip()
		:ips(4)
	{}
	std::vector<std::string> ips;
	//std::string ip1;
	//std::string ip2;
	//std::string ip3;
	//std::string ip4;
};

class SOCKS
{
	int s_sockfd;
	int c_sockfd;
	int b_sockfd;
	REQUEST_INFO request_info;
	Client client;
	int is_bind;
	fd_set wfds, rfds, afds;
	std::vector<Permit_ip> b_permit_ips;
	std::vector<Permit_ip> c_permit_ips;
public:
	
	SOCKS(int fd)
		:s_sockfd(fd), is_bind(false)
	{
		std::cerr << "NEW CLIENT" << std::endl;
		conf();
		FD_ZERO(&rfds);
		FD_ZERO(&afds);
		init();
	}
	void parse_ips(std::vector<std::string> &ips, std::string line)
	{
		int idx = 0;
		for(int i=0; i<line.size(); i++)
		{
			//std::cerr << line << " "<<  line.size() << " : " << line[i] << std::endl;
			if(line[i] == '.')
				idx++;
			else if(line[i] == ' ')
				break;
			else if(idx == 4)
				break;
			else
				ips[idx].push_back(line[i]);
		}
	}
	void parse_conf(std::string &line)
	{
		if(line.size() < 12 || line.substr(0, 6) != "permit")
			return;
		
		Permit_ip ip;
		parse_ips(ip.ips, line.substr(9));
		
		if(line[7] == 'b')
			b_permit_ips.push_back(ip);
		else if(line[7] == 'c')
			c_permit_ips.push_back(ip);
	}
	void conf()
	{
		std::ifstream file("socks.conf");
		std::string line;
		char ip1[4], ip2[4], ip3[4], ip4[4];
		while(!file.eof() && file.good() && std::getline(file, line))
			parse_conf(line);
		
		for(auto kk : b_permit_ips)
			std::cout << kk.ips[0] << kk.ips[1] << kk.ips[2] << kk.ips[3] << std::endl;
	}
	bool check_ip_pattern(std::string permit, std::string other)
	{
		std::cerr << permit << " : " << other << std::endl;
		if(permit == "*" || permit == other)
			return true;
		return false;
	}
	bool check_ip(char type, in_addr &addr)
	{
		char *some_addr;
		some_addr = inet_ntoa(addr);
		//printf("THIS %s\n", some_addr); // prints "10.0.0.1"
		
		std::string ip_line (some_addr);
		std::vector<std::string> ips(4);
		std::cerr << "B " << ip_line<< std::endl;
		parse_ips(ips, ip_line);
		
		std::cerr << "this" << std::endl;
		std::cout << ips[0] << ips[1] << ips[2] << ips[3] << std::endl;
		
		bool is_allow = false;
		
		std::vector<Permit_ip> *p_premits_ip;
		if(type == 'b')
			p_premits_ip = &b_permit_ips;
		else
			p_premits_ip = &c_permit_ips;
		
		for(Permit_ip &permit_ip : *p_premits_ip)
		{
			if(	check_ip_pattern(permit_ip.ips[0], ips[0])
				&& check_ip_pattern(permit_ip.ips[1], ips[1])
				&& check_ip_pattern(permit_ip.ips[2], ips[2])
				&& check_ip_pattern(permit_ip.ips[3], ips[3]))
			{
				is_allow = true;
				break;
			}
		}
		std::cerr << "is allow " << is_allow << std::endl;
		return is_allow;
	}
	void init()
	{
		std::cerr << "sockfd " << s_sockfd << " "<< sizeof(long) << std::endl;
		get_request();
		
		if(request_info.cd == 2)
		{
			
			client.ip = 0;
			//client.port = 3000;
			srand(time(NULL));
			
			client.port = (rand() % 30000)+10000;
			
			std::cerr << "======= BIND ===== "<< client.port << std::endl;
			is_bind = true;
			
			int tmp_fd = 0;
			while ((tmp_fd = server::passivsock(std::to_string(client.port)) ) == -1 )
			{
				client.port += 2;
				std::cerr << "server retry "<< client.port << std::endl;
				
			}
			send_request(true);
			std::cerr << "port"<< client.port << " fd " << tmp_fd << std::endl;
			//struct sockaddr_in client_addr;
			int tmp_client;
			c_sockfd = accept(tmp_fd, (struct sockaddr *) &(client.server_addr), (socklen_t *) &tmp_client);
			
			if(!check_ip('b', client.server_addr.sin_addr))
			{
				send_request(false);
				exit(0);
			}
			
			close(tmp_fd);
			if(c_sockfd == -1)
			{
				perror("cannot accept");
			}
			send_request(true);
			std::cerr << "bind sockfd:" << c_sockfd << tmp_fd << " port " <<client.port << std::endl;
			
			FD_SET(c_sockfd, &afds);
			FD_SET(s_sockfd, &afds);
			
			run();
			
			//std::cerr << "RUN finish" << std::endl;
		}
		else if(request_info.cd == 1)
		{
			int state = create_client();
			
			
			FD_SET(c_sockfd, &afds);
			FD_SET(s_sockfd, &afds);
			
			if(state == -1)
			{
				send_request(false);
				std::cerr << "client fail" << std::endl;
				exit(0);
			}
			else
			{
				send_request(true);
				std::cerr << "client success" << std::endl;
			}
			std::cerr << "state " << state << std::endl;
			
			
			run();
		}
		else
		{
			send_request(false);
			exit(0);
		}
		
	}
	void close_all()
	{
		FD_CLR(c_sockfd, &afds);
		FD_CLR(s_sockfd, &afds);
		close(c_sockfd);
		close(s_sockfd);
		exit(0);
	}
	void run()
	{
		int nfds = c_sockfd+5;
		
		char s_buf[4097];
		char c_buf[4097];
		
		struct timeval ts;
		ts.tv_sec = 300;
		ts.tv_usec = 0;
		
		int retry = 0;
		int select_state = 0;
		while(true)
		{
			memcpy(&rfds, &afds, sizeof(fd_set));
			//std::cerr << "select wait " << std::endl;
			
			if((select_state = select(nfds, &rfds, NULL, NULL, &ts)) < 0)
			{
				std::cerr << errno << "\n";
				perror("cannot select");
			}
			//std::cerr << "select wait finish "<< select_state << FD_ISSET(c_sockfd, &rfds)<< FD_ISSET(s_sockfd, &rfds) << std::endl;
			
			if(select_state == 0 && !is_bind)
			{
				std::cerr << "========== Time OUT ==========" << std::endl;
				close_all();
			}
			
			// client
			if (FD_ISSET(c_sockfd, &rfds))
			{
				//std::cerr << "B" << std::endl;
				int len, len2;
				
				len = read(c_sockfd, c_buf, 4096);
				len2 = write(s_sockfd, c_buf, len);
				
				if(len == 0)
				{
					close_all();
				}
			}
			
			if (FD_ISSET(s_sockfd, &rfds))
			{
				int len = read(s_sockfd, s_buf, 4096);
				int len2 = write(c_sockfd, s_buf, len);
				if(len == 0)
				{
					close_all();
				}
			}
		}
	}	
	void parse_request(char *buf, int len)
	{
		if(len >= 9)
		{
			request_info.vn = buf[0];
			request_info.cd = buf[1];
			request_info.port = ((uint8_t)buf[2]) << 8 | (uint8_t)buf[3];
			request_info.ip = (((uint8_t)buf[4])<<24 | ((uint8_t)buf[5])<<16 | ((uint8_t)buf[6])<<8 | (uint8_t)buf[7]);
			
			std::cout << "request_info.vn: " << (int) request_info.vn << std::endl;
			std::cout << "request_info.cd: " << (int) request_info.cd << std::endl;
			std::cout << "request_info.port " << (int) request_info.port << std::endl;
			std::cout << "request_info.ip: " <<  request_info.ip << std::endl;
		}
		if(len > 9)
		{
			request_info.user_id = (buf+8);
			std::cout << "request_info.user_id: " << request_info.user_id << std::endl;
		}
		if(len > (request_info.user_id.size()+10) )
		{
			request_info.domain_name = (buf+(request_info.user_id.size()+8+1));
			std::cout << "request_info.domain_name: " << request_info.domain_name << std::endl;
		}
	}
	
	void get_request()
	{
		char buf[256];
		int len = 0;
		int len2 = 0;
		len = read(s_sockfd, buf, 256);
		for(int i=0; i<len; i++)
			std::cerr << "buf " << (int) buf[i] << std::endl;

		parse_request(buf, len);
		std::cerr << "fffget_request len " << len << " len2 " << len2 << std::endl;
	}
	
	void send_request(bool success)
	{
		char pack[8];
		
		pack[0] = 0;
		pack[1] = (uint8_t)90;
		pack[2] = client.port/256;
		pack[3] = client.port%256;
		pack[4] = client.ip >> 24;
		pack[5] = (client.ip>>16) & 0xFF;
		pack[6] = (client.ip>>8) & 0xFF;
		pack[7] = client.ip & 0xFF;
		
		if(!success)
			pack[1] = (uint8_t)91;
		std::cerr << (int) pack[1] << std::endl;
		int len = write(s_sockfd, pack, 8);
		std::cerr << "send r " << len << " " << client.ip << " " << client.port << std::endl;
	}
	int create_client()
	{
		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		c_sockfd = sockfd;
		
		client.sockfd = sockfd;
		client.ip = request_info.ip;
		client.port = request_info.port;
		client.is_connected = false;
		
		memset(&client.server_addr, '0', sizeof(client.server_addr));
		client.server_addr.sin_family = AF_INET;
		client.server_addr.sin_addr.s_addr = htonl(request_info.ip);
		client.server_addr.sin_port = htons(client.port);
		
		print_ip(client.server_addr.sin_addr);
		
		if(!check_ip('c', client.server_addr.sin_addr))
			return -1;
		
		//int flags = fcntl(client.sockfd, F_GETFL, 0);
		//fcntl(client.sockfd, F_SETFL, flags | O_NONBLOCK);
		
		int state = connect(client.sockfd, (sockaddr *) &client.server_addr, sizeof(client.server_addr));
		
		
		return state;
	}
	
	void print_ip(in_addr &addr)
	{
		char *some_addr;
		some_addr = inet_ntoa(addr);
		printf("%s\n", some_addr); // prints "10.0.0.1"
	}
};

int main(int argc, char** argv)
{
	signal(SIGUSR1, [](int k){});
	signal(SIGUSR2, [](int k){});
	signal(SIGCHLD, SIG_IGN); // signal(SIGCHLD, childhandler);
	std::function<void(server_info)> run_socks = [](server_info s_info)
	{
		//HTTPD httpd(s_info.sockfd);
		SOCKS(s_info.sockfd);
	};
	
	
	server::forked_server(argv[1], run_socks);
	
	return 0;
}