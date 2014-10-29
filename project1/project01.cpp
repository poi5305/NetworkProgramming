#include <sys/socket.h> // socket
#include <sys/wait.h> // wait
#include <sys/stat.h> // open flag
#include <netinet/in.h> // sockaddr_in
#include <unistd.h> //exec, close
#include <fcntl.h> // open file
#include <signal.h> //signal
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>

//void nothinghandler(int sig){}
//void childhandler(int sig){
//    int status;
//    int kk = wait(&status);
//    std::cout << "childhandler " << kk << " " << status << std::endl;
//    return;
//}

class console
{
public:
	static void error(std::string &&msg)
	{
		std::cerr << msg << std::endl;
		exit(1);
	}
	static void log(std::string &&msg)
	{
		std::cout << msg << std::endl;
	}
};

struct command
{
	int pipe_num;
	int command_idx;
	bool is_end;
	std::vector<std::string> argv;
	std::string file1;
	std::string file2;
};

class parser
{
public:
	std::vector<command> parse_line()
	{
		std::vector<command> pipes;
		std::vector<std::string> argvs;
		std::string argv, file1, file2;
		int file_num = 0, pipe_num = 1;
		while(true)
		{
			char c = std::cin.get();
			if(!std::cin.good() || std::cin.eof())
				exit(0);	
			
			if(c == '"')
				std::getline(std::cin, argv, '"');
			else if(c == '\'')
				std::getline(std::cin, argv, '\'');
			
			if(c == ' ' || c == '|' || c == '\n')
			{
				if(argv.size() != 0)
				{
					argv.erase(std::remove(argv.begin(), argv.end(), '\r'), argv.end());
					argvs.push_back(argv);
					argv = "";
				}
			}
			if(c == ' ' || c == '\r') 
			{}
			else if(c == '|')
			{
				while(true)
				{
					char nc = std::cin.peek();
					if(nc >= 48 && nc <= 57)// is 0~9
						argv.push_back(std::cin.get());
					else
						break;
				}
				pipe_num = std::atoi(argv.c_str());
				if(pipe_num == 0)
					pipe_num = 1;
				pipes.push_back({pipe_num, 0, false, argvs, file1, file2});
				argv = ""; file1 = ""; file2 = "";
				argvs.resize(0);				
				pipe_num = 1;
			}
			else if(c == '>')
			{
				int file_num = 1;
				// ls >f, ls > f
				if(argv.size() != 0)
				{ // ls 2> f, ls 2>f
					int num = std::atoi(argv.c_str());
					if(num == 0) // ls>f
						argvs.push_back(argv);
					else
						file_num = num;
				}
				argv = "";
				if(parse_next_word(argv) == 0)
				{
					syntax_error(std::cin.peek());
					pipes.resize(0); argvs.resize(0);
				}
				if(file_num == 1)
					file1 = argv;
				else
					file2 = argv;
				argv = "";
			}
			else if(c == '\n')
			{
				if(argvs.size() > 0)
					pipes.push_back({pipe_num, 0, true, argvs, file1, file2});
				break;
			}
			else
				argv.push_back(c);
		}
		return pipes;
	}
	int parse_next_word(std::string &argv)
	{
		while(std::cin.peek() == ' ')
			std::cin.get();
		while(true)
		{
			char nc = std::cin.peek();
			if(nc == '|' || nc == ' ' || nc == '\n' || nc == '\r')
				break;
			else
				argv.push_back(std::cin.get());
		}
		return argv.size();
	}
	std::vector<command> parse_line2()
	{
		std::vector<command> pipes;
		std::vector<std::string> argvs;
		std::string argv;
		std::string file1;
		std::string file2;
		bool is_line_end = false;
		bool is_still_q1 = false;
		bool is_still_q2 = false;
		bool is_argv_break = false;
		bool is_pipe = false;
		bool is_file = false;
		int file_num = 0;
		int pipe_num = 1;
		while(true)
		{
			char c = std::cin.get();
			if(!std::cin.good() || std::cin.eof())
				exit(0);
			if(is_still_q1)
			{
				if(c == '"')
					is_still_q1 = false;
				else
					argv.push_back(c);
				continue;
			}
			if(is_still_q2)
			{
				if(c == '\'')
					is_still_q2 = false;
				else
					argv.push_back(c);
				continue;
			}
			switch(c)
			{
				case ' ':
					is_argv_break = true;
					break;
				case '|':
					is_argv_break = true;
					is_pipe = true;
					break;
				case '>':
					is_argv_break = true;
					is_file = true;
					break;
				case '\r':
					break;
				case '\n':
				case ';':
					is_argv_break = true;
					is_line_end = true;
					break;
				case '"':
					is_still_q1 = true;
					break;
				case '\'':
					is_still_q2 = true;
					break;
				default:
					argv.push_back(c);
					break;
			}
			if(is_argv_break)
			{
				is_argv_break = false;
				if(is_file)
				{
					// cmd file> xxx || cmd file > xxx
					if(argv.size() != 0 && file_num == 0)
					{
						int num = std::atoi(argv.c_str());
						if(num != 0)
						{
							file_num = num;
							argv = ""; // no push back into argvs
						}
					}
					if(c == '>' && argv.size() != 0)
					{
						argvs.push_back(argv);
						argv = "";
					}
					if(file_num == 0)
						file_num = 1;
					if(argv.size() == 0)
					{
						if(is_pipe && file1=="" && file2=="")
							syntax_error(c);
						else
							continue;
					}
					if(file_num == 1)
					{
						file1 = argv;
						argv = "";
						is_file = false;
						file_num = 0;
					}
					else if(file_num == 2)
					{
						file2 = argv;
						argv = "";
						is_file = false;
						file_num = 0;
					}
				}
				if(is_pipe)
				{
					if(c == '|' && argv.size() != 0)
					{
						argvs.push_back(argv);
						argv = "";
					}
					while(true)
					{
						char nc = std::cin.peek();
						if(nc >= 48 && nc <= 57)// is 0~9
							argv.push_back(std::cin.get());
						else
							break;
					}
					int num = std::atoi(argv.c_str());
					pipe_num = num;
					if(pipe_num == 0)
						pipe_num = 1;
					pipes.push_back({pipe_num, 0, false, argvs, file1, file2});
					
					argv = ""; file1 = ""; file2 = "";
					argvs.resize(0);
					is_pipe = false; is_file = false; file_num = 0;
				}
				if(argv.size() != 0)
				{
					argvs.push_back(argv);
					argv = "";
				}
			}
			if(is_line_end)
			{
				if(argvs.size() > 0)
					pipes.push_back({pipe_num, 0, true, argvs, file1, file2});
				break;
			}
		}
		return pipes;
	}
	void syntax_error(char c)
	{
		while(true)
		{
			char k = std::cin.get();
			if(k == '\n')
				break;
		}
		std::cerr << "syntax error near unexpected token '"<< c << "'" << std::endl;
		std::cout << "% ";
		std::cout.flush();
	}
};

class shell
{
	parser cmd_parser;
	std::map<std::string, std::string> ENV;
	std::map< int, std::pair<int,int> > pipes;
	int next_command_count;
public:
	shell(int in, int out, int err)
		:ENV({{"PATH","bin:."}}), next_command_count(0)
	{
		if(in != 0) close(0);dup(in);
		if(out != 1) close(1);dup(out);
		if(err != 2) close(2);dup(err);
		if(in == out && in == err)
			close(in);
		
		print_hello();
		while(true)
		{
			std::vector<command> pipes = cmd_parser.parse_line();
			//for(auto pipe : pipes)
			//{
			//	std::cerr << "argv " << pipe.argv[0] << std::endl;
			//	std::cerr << "pipe_num " << pipe.pipe_num << std::endl;
			//}
			if(exam_command(pipes))
				run_command(pipes);
			print_success();
		}
	}
private:
	bool exam_command(std::vector<command> &cmds)
	{
		for(auto &cmd : cmds)
		{
			if(cmd.argv[0] == "setenv")
			{
				ENV[cmd.argv[1]] = cmd.argv[2];
				return false;
			}
			else if(cmd.argv[0] == "printenv")
			{
				std::cout << cmd.argv[1] << "=" << ENV[cmd.argv[1]] << std::endl;
				return false;
			}
			else if(cmd.argv[0] == "exit")
				exit(0);
			
			std::string filename = test_filename(cmd.argv[0]);
			if( filename == "" || cmd.argv[0].size() == 0)
			{
				if(cmd.argv[0].size()!=0)
					std::cout << "Unknown command: [" << cmd.argv[0] << "]." << std::endl;
				break;
			}
			else
			{
				next_command_count++;
				cmd.command_idx = next_command_count;
			}
		}
		return true;
	}
	std::string test_filename(std::string &f)
	{
		std::vector<std::string> tmp_vec;
		boost::split(tmp_vec, ENV["PATH"], boost::is_any_of(":"), boost::token_compress_on);
		for(auto &path : tmp_vec)
		{
			std::string filename = path + "/" + f;
			if( access( filename.c_str(), X_OK ) != -1)
				return filename;
		}
		return "";
	}
	void create_pipe(std::vector<command> &cmds, int pipe_i)
	{
		auto &cmd = cmds[pipe_i];
		if(cmd.command_idx == 0)
			return;
		if(cmd.is_end)
			return;
		if(pipes.find(cmd.command_idx + cmd.pipe_num) == pipes.end())
		{
			int fd[2];
			if(pipe(fd) == -1)
				console::error("Error, can't pipe");
			pipes[cmd.command_idx + cmd.pipe_num] = {fd[0], fd[1]};
			//std::cerr << fd[0] << fd[1] << std::endl;
		}
	}
	void pipe_tofile(std::vector<command> &cmds, int pipe_i)
	{
		auto &cmd = cmds[pipe_i];
		if(cmd.file1 != "")
		{
			int fd;
			if( (fd = open(cmd.file1.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0)
				console::error("Error, can not open file " + cmd.file1);
			close(1);
			dup(fd);
		}
		if(cmd.file2 != "")
		{
			int fd;
			if( (fd = open(cmd.file2.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0)
				console::error("Error, can not open file " + cmd.file2);
			close(2);
			dup(fd);
		}
	}
	void pipe_handler(std::vector<command> &cmds, int pipe_i)
	{
		auto &cmd = cmds[pipe_i];
		if( pipes.find(cmd.command_idx + cmd.pipe_num) != pipes.end() && !cmd.is_end )
		{//has pipe
			close(1);
			dup(pipes[cmd.command_idx + cmd.pipe_num].second);
		}
		if(pipes.find(cmd.command_idx) != pipes.end())
		{
			close(0);
			dup(pipes[cmd.command_idx].first);
		}
		for(auto &pipe_pair : pipes)
		{
			close(pipe_pair.second.first);
			close(pipe_pair.second.second);
		}
	}
	void run_command_impl(std::vector<command> &cmds, int pipe_i)
	{
		int child_pid;
		if ( (child_pid = fork()) < 0  )
			console::error("Error, can't fork");
		else if (child_pid == 0)
		{
			pipe_handler(cmds, pipe_i); // close or dup pipe
			pipe_tofile(cmds, pipe_i); // check and stdout or open file
			// run execv
			auto &cmd =cmds[pipe_i];
			const char * argv[cmd.argv.size()+1];
			for(int i=0; i<cmd.argv.size(); i++)
				argv[i] = cmd.argv[i].c_str();
			argv[cmd.argv.size()] = 0;
			std::string filename = test_filename(cmd.argv[0]);
			int kk = execv(filename.c_str(), (char **)argv);
			if(kk < 0)
				std::cout << "Unknown command: [" << cmd.argv[0] << "]." << std::endl;
			exit(0);
		}
	}
	void run_command(std::vector<command> &cmds)
	{
		for(int pipe_i=0; pipe_i<cmds.size(); pipe_i++)
		{
			auto &cmd = cmds[pipe_i];
			create_pipe(cmds, pipe_i);
			
			if(cmd.command_idx == 0) break;
			run_command_impl(cmds, pipe_i);	 // fork
			
			auto iter = pipes.find(cmd.command_idx);
			if( iter != pipes.end())
			{
				close(iter->second.first);
				close(iter->second.second);
				pipes.erase(iter);
			}
			int kk;
			int gg = wait(&kk);
		}
	}
	void print_hello()
	{
		const std::string hello = "****************************************\n** Welcome to the information server. **\n****************************************\n% ";
		std::cout << hello;
		std::cout.flush();
	}
	void print_success()
	{
		std::cout << "% ";
		std::cout.flush();
	}
};

class server
{
public:
	server(std::function<void()> server_function)
	{
		signal(SIGCHLD, [](int k){});
		server_function();
	}
	server(int address, int port, std::function<void(int)> server_function)
	{
		int server_post = port;
		int sockfd, newsockfd, clilen, child_pid;
		struct sockaddr_in server_addr, client_addr;
		std::vector<int> child_pids(0);
		
		if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			console::error("Error, can't not open socket.");
		
		memset((void*) &server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(address);
		server_addr.sin_port = htons(server_post);
		
		if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
		{
			std::cout << "Error, can't bind socket" << std::endl;
			//console::error("Error, can't bind socket");
			server_addr.sin_port = htons(server_post+1);
			bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
			std::cout << server_post+1 << std::endl;
		}
		listen(sockfd, 1);
		
		for(int i=0;i<109;i++)
		{
			newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, (socklen_t *) &clilen);
			//std::cout << client_addr.sin_addr.s_addr << std::endl;
			if (newsockfd < 0)
				console::error("Error, can't accept");
			if ( (child_pid = fork()) < 0  )
				console::error("Error, can't fork");
			else if (child_pid == 0)
			{// child procress
				close(sockfd);
				signal(SIGCHLD, [](int k){});
				//signal(SIGCHLD, nothinghandler); // child not auto wait
				server_function(newsockfd);
				exit(0);
			}
			else
			{// parent procress
				child_pids.push_back(child_pid);
			}
			close(newsockfd);
		}
		close(sockfd);
	}
};


int main (int argc, char** argv)
{
	signal(SIGCHLD, SIG_IGN); // signal(SIGCHLD, childhandler);
	std::function<void()> local = [](){
		shell my_sh(0, 1, 2);
	};
	std::function<void(int)> remote = [](int sockfd){
		shell my_sh(sockfd, sockfd, sockfd);
	};
	int port = 0;
	if(argc > 1)
		port = atoi(argv[1]);
	if(port)
		server s(INADDR_ANY, port, remote);
	else
		server s(local);
	
	return 0;
}
