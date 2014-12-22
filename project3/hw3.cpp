#include <iostream>

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

#include <string>
#include <vector>
#include <map>
#include <fstream>

#include <boost/algorithm/string.hpp>


struct Client
{
	int id;
	int sockfd;
	int port;
	int txt_idx;
	bool is_connected;
	bool is_write;
	bool is_exit;
	std::string ip;
	std::string txt;
	std::vector<std::string> txt_content;
	struct sockaddr_in server_addr;
	struct hostent *server;
};

class cgi
{
public:	
	std::map<std::string, std::string> query_strings;
	std::vector<Client> clients;
	fd_set rfds, afds;
	int max_fd;
	cgi()
		:max_fd(2)
	{
		FD_ZERO(&rfds);
		FD_ZERO(&afds);
	}
	
	int new_client(int id, std::string ip, int port, std::string txt, Client *p_client = NULL)
	{
		if(p_client == NULL)
		{
			clients.emplace_back(Client());
			p_client = &(clients.back());
		}
		auto &client = *p_client;
		
		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			std::cerr << "Cannot open socket.\n";
			exit(1);
		}
		client.id = id;
		client.sockfd = sockfd;
		client.ip = ip;
		client.is_connected = false;
		client.is_write = false;
		client.port = port;
		client.txt_idx = 0;
		client.txt = txt;
		
		client.server = gethostbyname(client.ip.c_str());
		if (client.server == NULL) {
			std::cerr << "Cannot get host name.\n";
			exit(1);
		}

		memset(&client.server_addr, '0', sizeof(client.server_addr));
		client.server_addr.sin_family = AF_INET;
		client.server_addr.sin_addr = *((struct in_addr *)client.server->h_addr);
		client.server_addr.sin_port = htons(client.port);
		
		// Non blocking
		int flags = fcntl(sockfd, F_GETFL, 0);
		fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
		
		int state = connect(client.sockfd, (sockaddr *) &client.server_addr, sizeof(client.server_addr));
		
		//std::cerr << "state: " << state << " fd: "<< client.sockfd << " Connect: " << client.ip << ":" << client.port << " " << client.txt << " " << client.server->h_addr << std::endl;
		return client.sockfd;
	}
	
	void init()
	{
		get_query_string();
		
		for(int i=1;i<=5;i++)
		{
			char ci = i+48;
			std::string idx(&ci,1);
			std::string s_ip = "h" + idx;
			std::string s_port = "p" + idx;
			std::string s_txt = "f" + idx;
			
			if(query_strings[s_ip] == "" || query_strings[s_port]=="" || query_strings[s_txt] == "")
				continue;
				
			int sockfd = new_client(i-1, query_strings[s_ip], std::atoi(query_strings[s_port].c_str()), query_strings[s_txt], NULL);
			
			max_fd = std::max(max_fd, sockfd);
			FD_SET(sockfd, &afds);
		}
		
	}
	void read_txt(Client &client)
	{
		std::string line;
		std::ifstream fp(client.txt);
		while(std::getline(fp, line))
		{
			client.txt_content.push_back(line+'\n');
		}
		fp.close();
	}
	int contain_prompt ( char* line )
	{
		int i, prompt = 0 ;
		for (i=0; line[i]; ++i)
		{
			switch ( line[i] ) {
				case '%' : prompt = 1 ; break;
				case ' ' : if ( prompt ) return 1;
				default: prompt = 0;
			}
		}
		return 0;
	} 
	
	int recv_msg(Client &client)
	{
		char buf[3000],*tmp;
		int len,i;
		
		len=read(client.sockfd, buf,sizeof(buf)-1);
		if(len < 0) return -1;
		
		buf[len] = 0;
		if(len>0)
		{
			for(tmp=strtok(buf,"\n"); tmp; tmp=strtok(NULL,"\n"))
			{
				if ( contain_prompt(tmp) )
					client.is_write = true;
				print_html(client, tmp, false);
				//printf("%d| %s\n",0,tmp);  // echo input
			}
		}
		fflush(stdout); 
		return len;
	}
	void send_msg(Client &client)
	{
		if(client.txt_content.size() == 0)
			read_txt(client);
		if(client.is_write && client.txt_idx != client.txt_content.size())
		{
			auto &line = client.txt_content[client.txt_idx];
			//std::cout << line << std::endl;
			print_html(client, line, true);
			write(client.sockfd, line.c_str(), line.length());
			client.is_write = false;

			if(line.find("exit") != std::string::npos)
			client.is_exit = true;
			client.txt_idx++;
		}
	}
	void print_html(Client &client, std::string str, bool border)
	{
		boost::replace_all(str, "\r", " ");
		boost::replace_all(str, "\n", " ");
		std::cout << "<script>document.all['m" << client.id <<"'].innerHTML += \"";
		if (border) std::cout << "<b>";
		std::cout << str;
		if (border) std::cout << "</b>";

		if (str[0] != '%')
			std::cout << "<br/>";
		std::cout << "\";</script>" << std::endl;
		std::cout.flush();
	}
	void run()
	{
		int nfds = max_fd+1;
		//int nfds = getdtablesize();
		//nfds = 8;
		while (true)
		{
			memcpy(&rfds, &afds, sizeof(fd_set));
			//if(select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0)
			//std::cerr << "AAAA "<< std::endl;
			if(select(nfds, &rfds, NULL, NULL, NULL) < 0)
			{
				std::cerr << errno << "\n";
				perror("cannot select");
			}
			//std::cerr << "BBB "<< std::endl;
			for (auto &client : clients)
			{
				auto client_fd = client.sockfd;
				if (FD_ISSET(client_fd, &rfds))
				{
					std::cerr << "active " << client_fd << std::endl;
					if(!client.is_connected)
					{
						int state = connect(client.sockfd, (sockaddr *) &client.server_addr, (socklen_t)sizeof(client.server_addr));
						if(state == 0)
							client.is_connected = true;
						else if(state < 0)
						{
							perror("cannot connect");
							if(errno == EISCONN)
								client.is_connected = true;
							else if(errno == EINVAL)
							{ // bsd Invalid argument
								close(client_fd);
								FD_CLR(client_fd, &afds);
								int sockfd = new_client(client.id, client.ip, client.port, client.txt, &client);
								max_fd = std::max(max_fd, sockfd);
								FD_SET(sockfd, &afds);
							}
						}
						//std::cerr << "active " << client_fd << " " << state << " " << errno << " "<< EISCONN<< std::endl;
						//sleep(1);
						continue; // if not connected, then continue
					}
					// already connect
					//std::cerr << "Connected~" << client_fd << std::endl;
					//std::cerr << "recv~" << client_fd << std::endl;
					recv_msg(client);
					//std::cerr << "send~" << client_fd << std::endl;
					send_msg(client);
					//std::cerr << "send end~" << client_fd << std::endl;
					if(client.is_exit)
					{
						//std::cerr << "FINISH" << std::endl;
						FD_CLR(client_fd, &afds);
						close(client_fd);
					}
				}
			}
			
			// check is all client finish
			bool all_finish = true;
			for (auto &client : clients)
			{
				//std::cerr << "check " << client.sockfd << " " << client.is_exit << std::endl;
				if(!client.is_exit)
					all_finish = false;
			}
			if(all_finish)
			{
				//std::cerr << "All Finish" << std::endl;
				break;
			}	
			//sleep(1);
		}
	}
	void get_query_string()
	{
		query_strings.clear();
		
		std::string query_string = "h1=140.113.235.169&p1=5555&f1=t1.txt&h2=140.113.235.169&p2=5555&f2=t2.txt&h3=140.113.235.169&p3=5555&f3=t3.txt&h4=140.113.235.169&p4=5555&f4=t4.txt&h5=140.113.235.169&p5=5555&f5=t5.txt"; 
		const char* env = getenv("QUERY_STRING");
		if(env)
			query_string = env;
		
		std::vector<std::string> tmp_vec;
		boost::split(tmp_vec, query_string, boost::is_any_of("=&"), boost::token_compress_off);
		for(int i=1; i<tmp_vec.size(); i+=2)
		{
			query_strings[tmp_vec[i-1]] = tmp_vec[i];
			//result.push_back({tmp_vec[i-1], tmp_vec[i]});
			std::cerr << "Query string : " << tmp_vec[i-1] << " = " << tmp_vec[i] << std::endl;
		}
	}
	void print_header()
	{
		std::string header = R"(
			<html>
	        <head>
	        <meta http-equiv="Content-Type" content="text/html; charset=big5" />
	        <title>Network Programming Homework 3</title>
	        </head>
	        <body bgcolor=#336699>
	        <font face="Courier New" size=2 color=#FFFF99>
	        <table width="800" border="1">
	        <tr>
	    )";
	    std::cout << header ;
	    
	    for(auto &client : clients)
		    std::cout << "<td>" << client.ip << "</td>";
	    
	    std::cout << "</tr><tr>";
	    for(auto &client : clients)
		    std::cout << "<td valign=\"top\" id=\"m" << client.id << "\"></td>";
	    
	    std::cout << "</tr></table>";
	    
		std::cout << std::endl;
		std::cout.flush();
	}
	void print_footer()
	{
		std::string footer = R"(
			</font>
			</body>
			</html>
		)";
		std::cout << footer << std::endl;
		std::cout.flush();
	}
};





int main(int argc, char** argv)
{
	std::cout << "Content-Type: text/html\n\n";
	std::cout.flush();
	
	cgi hw3;
	hw3.init();
	hw3.print_header();
	
	hw3.run();
	hw3.print_footer();
	//std::vector<std::pair<std::string, std::string>> query_strings = get_query_string();
	
	//std::cout << "Test OK" << std::endl;
	//std::cout << "Test OK" << std::endl;
	//std::cout << "Test OK" << std::endl;
	return 0;
}