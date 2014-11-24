#ifndef __SHELL__
#define __SHELL__

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>
#include <signal.h>
#include "struct.hpp"
#include "parser.hpp"

template<class METHOD>
class shell
	:public METHOD
{
	parser cmd_parser;
	std::map<std::string, std::string> ENV;
	std::map< int, std::pair<int,int> > pipes;
	
	int next_command_count;
	
	std::string socket_name;
	bool is_exit;
public:
	static int g_user_id;
	int user_id;
	int socket_fd;
	std::string line;
	//static std::map<int, user> users;
	//static std::map<std::pair<int, int>, std::pair<int, int>> chart_fifo; // userid, userid, ??
	shell()
	{}
	// project1
	shell(int in, int out, int err)
		:ENV({{"PATH","bin:."}}), next_command_count(0)
	{
		if(in != 0) close(0);dup(in);
		if(out != 1) close(1);dup(out);
		if(err != 2) close(2);dup(err);
		
		if(in == out && in == err)
			close(in);
			
		g_user_id = user_id = struct_utility::get_new_id(users, USER_LEN, 1);
		METHOD::set_id(user_id, 0);
		//std::cerr << "user_id: " << user_id << " " << socket_fd << std::endl;
		struct_utility::active_user(user_id, socket_fd, 0, "no name", "localhost", "0");
		
		print_hello();
		while(true)
		{
			std::vector<command> pipes = cmd_parser.parse_line();
			line = cmd_parser.line;
			if(exam_command(pipes))
				run_command(pipes);
			print_success();
		}
	}
	
	// project2 single process
	shell(int sockfd, std::string sockname, std::string ip, std::string port)
		:ENV({{"PATH","bin:."}}), next_command_count(0), socket_fd(sockfd), socket_name(sockname), is_exit(false)
	{
		g_user_id = user_id = struct_utility::get_new_id(users, USER_LEN, 1);
		METHOD::set_id(user_id, socket_fd);
		//std::cerr << "user_id: " << user_id << " " << socket_fd << std::endl;
		struct_utility::active_user(user_id, socket_fd, 0, socket_name, ip, port);
		
		this->update_fd();
		print_hello();
		
		//*** User '(no name)' entered from (IP/port). ***
		//std::string msg;
		//msg += "*** User '(no name)' entered from "+ip+"/"+port+". ***\n";
		//this->yell(msg);
		
	}
	
	
	// project2 multiple process
	shell(int sockfd, int pid, std::string sockname, std::string ip, std::string port)
		:ENV({{"PATH","bin:."}}), next_command_count(0), socket_fd(sockfd), socket_name(sockname), is_exit(false)
	{
		close(0);dup(sockfd);
		close(1);dup(sockfd);
		close(2);dup(sockfd);
		//close(sockfd);
		pid = getpid();
		
		struct_utility::lock();
		g_user_id = user_id = struct_utility::get_new_id(users, USER_LEN, 1);
		
		METHOD::set_id(user_id, socket_fd);
		//std::cerr << "user_id: " << user_id << " " << socket_fd << " pid " << pid << std::endl;
		struct_utility::active_user(user_id, socket_fd, pid, socket_name, ip, port);
		struct_utility::unlock();
		
		print_hello();
		
		//*** User '(no name)' entered from (IP/port). ***
		//std::string msg;
		//msg += "*** User '(no name)' entered from "+ip+"/"+port+". ***\n";
		//this->yell(msg);
		
		
		while(true)
		{
			std::vector<command> pipes = cmd_parser.parse_line();
			line = cmd_parser.line;
			debug_cmd(pipes);
			if(exam_command(pipes))
				run_command(pipes);
			print_success();
		}
	}

	int run_one_time()
	{	
		console::debug("run_one_time start");
		//std::cerr << socket_fd << " " <<METHOD::socket_fd << std::endl;
		this->update_fd();
		std::vector<command> pipes = cmd_parser.parse_line();
		line = cmd_parser.line;
		debug_cmd(pipes);
		if(exam_command(pipes))
			run_command(pipes);
		if(is_exit)
			return 0;
		print_success();
		console::debug("run_one_time end");
		return 1;
	}
	void print_hello()
	{
		//const std::string hello = "****************************************\n** Welcome to the information server. **\n****************************************\n% ";
		const std::string hello = "****************************************\n** Welcome to the information server. **\n****************************************\n";
		std::cout << hello;
		std::cout.flush();
		
		std::string msg;
		msg = msg + "*** User '(no name)' entered from "+users[user_id].ip+"/"+users[user_id].port+". ***\n";
		this->yell(msg);
		
		std::cout << "% ";
		std::cout.flush();
	}
	void print_success()
	{
		std::cout << "% ";
		std::cout.flush();
	}
private:
	void debug_cmd(std::vector<command> &pipes)
	{
		int i = 0;
		for(auto &pipe : pipes)
		{
			//std::cerr << "===DEBUG CMDS PARA=== (" << i << "): ";
			//std::cerr << "pipe_num: " << pipe.pipe_num << "; ";
			//std::cerr << "command_idx: " << pipe.command_idx << "; ";
			//std::cerr << "is_end: " << pipe.is_end << "; ";
			//std::cerr << "file1: " << pipe.file1 << "; ";
			//std::cerr << "file2: " << pipe.file2 << "; ";
			//std::cerr << "user_out: " << pipe.user_out << "; ";
			//std::cerr << "user_in: " << pipe.user_in << "; ";
			//std::cerr << "\n";
			//std::cerr << "===DEBUG CMDS ARGV=== (" << i << "): ";
			//for(auto &para : pipe.argv)
			//	std::cerr << para << " ";
			//std::cerr << "\n";
			//std::cerr << "===================== (" << i << ")\n";
			i++;
		}
	}
	bool exam_command(std::vector<command> &cmds)
	{
		for(auto &cmd : cmds)
		{
			if(cmd.argv[0] == "setenv")
			{
				if(cmd.argv.size() > 2 && cmd.argv[1] != "")
					ENV[cmd.argv[1]] = cmd.argv[2];
				return false;
			}
			else if(cmd.argv[0] == "printenv")
			{
				if(cmd.argv.size() > 1 && ENV.find(cmd.argv[1]) != ENV.end())
					std::cout << cmd.argv[1] << "=" << ENV[cmd.argv[1]] << std::endl;
				return false;
			}
			else if(cmd.argv[0] == "who")
			{
				who();
				return false;
			}
			else if(cmd.argv[0] == "name")
			{
				if(cmd.argv.size() > 1 && cmd.argv[1] != "")
				{
					if( change_name(cmd.argv[1]) )
					{ // broad cast
						this->yell( std::string("*** User from ") + users[user_id].ip + "/" + users[user_id].port + " is named '" + users[user_id].name + "'. ***\n" );
					}
				}
				return false;
			}
			else if (cmd.argv[0] == "yell")
			{
				if(cmd.argv.size() > 1 && cmd.argv[1] != "")
				{
					std::string msg("");
					msg += std::string("*** ") + users[user_id].name + " yelled ***: ";//*** (sender's name) this->yelled ***: (message)
					msg += line2words(line, 2, 1);
					//msg += cmd.argv[1];
					//for(int i=2; i<cmd.argv.size(); i++)
					//	msg += " " + cmd.argv[i];
					this->yell( msg+"\n" );
				}
				return false;
			}
			else if(cmd.argv[0] == "tell")
			{
				if(cmd.argv.size() > 2 && cmd.argv[1] != "")
				{
					int to_user_id = atoi(cmd.argv[1].c_str());
					
					struct_utility::lock();
					int user_find = struct_utility::find(users, to_user_id);
					struct_utility::unlock();
					
					//*** Error: user #3 does not exist yet. ***
					if(user_find  == -1)
					//if(users.find(to_user_id) == users.end())
					{
						std::cout << "*** Error: user #" << to_user_id << " does not exist yet. ***\n";
						std::cout.flush();
					}
					else
					{
						std::string msg(""); // *** (sender's name) told you ***: (message)
						msg += std::string("*** ") + users[user_id].name + " told you ***: ";
						msg += line2words(line, 3, 2);
						//msg += cmd.argv[2];
						//for(int i=3; i<cmd.argv.size(); i++)
						//	msg += " " + cmd.argv[i];
						this->tell(to_user_id, msg+"\n");
					}
				}
				return false;
			}
			else if(cmd.argv[0] == "exit")
			{//*** User '(name)' left. ***
				
				struct_utility::lock();
				auto my_name = users[user_id].name;
				is_exit = true;
				
				for(int i=0; i<FIFO_LEN; i++)
				{
					auto &fifo = chart_fifo[i];
					if(fifo.state == 0)
						continue;
					//if(fifo.first.first == user_id || fifo.first.second == user_id)
					if(fifo.from_user == user_id || fifo.to_user == user_id)
					{
						close(fifo.read_fd);
						close(fifo.write_fd);
						//close(fifo.second.first);
						//close(fifo.second.second);
						std::string fifo_name;
						//fifo_name += "fifo_" + std::to_string(fifo.first.first) + "_" + std::to_string(fifo.first.second);
						fifo_name += "/tmp/andy_fifo_" + std::to_string(fifo.from_user) + "_" + std::to_string(fifo.to_user);
						console::debug(fifo_name);
						unlink(fifo_name.c_str());
						fifo.state = 0;
						//tmp_pairs.push_back(fifo.first);
						
					}
				}
				struct_utility::unlock();
				std::string msg;
				msg += std::string("*** User '") + my_name + "' left. ***\n";
				this->yell(msg);
				
				struct_utility::lock();
				struct_utility::remove(users, user_id);
				struct_utility::unlock();
				//close(socket_fd);
				//close(0);
				//close(1);
				//close(2);
				
				this->exit_impl();
				return false;
				//exit(0);
			}
			
			std::string filename = test_filename(cmd.argv[0]);
			if( filename == "" || cmd.argv[0].size() == 0)
			{
				if(cmd.argv[0].size()!=0)
					std::cout << "Unknown command: [" << cmd.argv[0] << "]." << std::endl;
				break;
			}
			else
			{
				if(!exam_fifo_to_user(cmd))
					return false;
				//fifo_to_user_msg(cmd);
				next_command_count++;
				cmd.command_idx = next_command_count;
			}
		}
		return true;
	}
	bool exam_fifo_to_user(command &cmd)
	{
		// check fifo user exist
		int find_id = -1;
		
		struct_utility::lock();
		find_id = struct_utility::find(users, cmd.user_out);
		struct_utility::unlock();
		
		if(cmd.user_out != 0 &&  find_id == -1)
		{//*** Error: user #1 does not exist yet. ***
			std::cout << "*** Error: user #" << cmd.user_out << " does not exist yet. ***\n";
			std::cout.flush();
			return false;
		}
		
		struct_utility::lock();
		find_id = struct_utility::find(chart_fifo, user_id, cmd.user_out);
		struct_utility::unlock();
		
		if(cmd.user_out != 0 && find_id != -1)
		{//*** Error: the pipe #2->#1 already exists. ***
			std::cout << "*** Error: the pipe #" << user_id << "->#" << cmd.user_out << " already exists. ***\n";
			std::cout.flush();
			return false;
		}
		
		struct_utility::lock();
		find_id = struct_utility::find(chart_fifo, cmd.user_in, user_id);
		struct_utility::unlock();
		
		if(cmd.user_in != 0 && find_id == -1)
		{//*** Error: the pipe #3->#1 does not exist yet. ***
			std::cout << "*** Error: the pipe #" << cmd.user_in << "->#" << user_id << " does not exist yet. ***\n";
			std::cout.flush();
			return false;
		}
		return true;
	}
	void fifo_to_user_msg(command &cmd)
	{
		if(cmd.user_out != 0)
		{//*** IamUser (#3) just piped 'cat test.html | cat >1' to Iam1 (#1) ***
			std::string tmp = "";
			struct_utility::lock();
			tmp += std::string("*** ") + users[user_id].name+" (#"+std::to_string(user_id)+") just piped '"+line+"' to "+users[cmd.user_out].name+" (#"+std::to_string(cmd.user_out)+") ***\n";
			struct_utility::unlock();
			this->yell(tmp);
		}
		if(cmd.user_in != 0)
		{//*** IamUser (#3) just received from student7 (#7) by 'cat <7' ***
			std::string tmp = "";
			struct_utility::lock();
			tmp += std::string("*** ")+users[user_id].name+" (#"+std::to_string(user_id)+") just received from "+users[cmd.user_in].name+" (#"+std::to_string(cmd.user_in)+") by '"+line+"' ***\n";
			struct_utility::unlock();
			this->yell(tmp);
		}
	}
	std::string line2words(const std::string &line, int limit_w, int limit_s)
	{
		std::string words;
		int state = 0;
		int w = 0, s = 0, idx = 0;
		
		for(int i=0; i<line.size(); i++)
		{
			if(line[i] != ' ' && state == 0)
			{
				w++;
				state = 1;
			}
			else if(line[i] == ' ' && state == 1)
			{
				s++;
				state = 0;
			}
			idx = i;
			if(w == limit_w && s == limit_s)
				break;
		}
		return line.substr(idx);
	}
	bool is_exist_name(const std::string name)
	{
		struct_utility::lock();
		for(int i = 1; i<USER_LEN; ++i)
		//for(auto &u_pair : users)
		{
			int u_user_id = i;
			auto &u_user = users[u_user_id];
			if(u_user.state == 0)
				continue;
			//auto &u_user = u_pair.second;
			if(name == u_user.name)
			{
				struct_utility::unlock();
				return true;
			}		
		}
		struct_utility::unlock();
		return false;
	}
	bool change_name(const std::string &name)
	{
		if(!is_exist_name(name))
		{
			console::debug("change name success");
			//users[user_id].name = name;
			struct_utility::lock();
			strcpy(users[user_id].name, name.c_str());
			struct_utility::unlock();
			return true;
		}
		else
		{
			std::cout << "*** User '" << name <<"' already exists. ***" << "\n";
			std::cout.flush();
			return false;
		}
	}
	void who()
	{
		struct_utility::lock();	
		std::cout << "<ID>	<nickname>	<IP/port>	<indicate me>\n";
		//for(auto &u_pair : users)
		
		for(int i = 1; i<USER_LEN; ++i)
		//for(auto &u_pair : users)
		{
			int u_user_id = i;
			auto &u_user = users[u_user_id];
			if(u_user.state == 0)
				continue;
			//std::cerr << "who " << u_user_id << " exist" << std::endl;
			std::cout << u_user_id << "\t" << u_user.name << "\t" << u_user.ip << "/" << u_user.port;
			if(u_user_id == user_id)
				std::cout << "\t<-me" << "\n";
			else
				std::cout << "\n";
		}
		std::cout.flush();
		struct_utility::unlock();
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
	
	void fifo_handler(std::vector<command> &cmds, int pipe_i)
	{ // in fork
		struct_utility::lock();
		auto &cmd = cmds[pipe_i];
		if(cmd.user_out != 0)
		{
			int i = struct_utility::find(chart_fifo, user_id, cmd.user_out);
			auto &fifo = chart_fifo[i];
			close(1);
			dup(fifo.write_fd);
			close(fifo.read_fd);
			close(fifo.write_fd);
		}
		if(cmd.user_in != 0)
		{
			int i = struct_utility::find(chart_fifo, cmd.user_in, user_id);
			auto &fifo = chart_fifo[i];
			close(0);
			//std::cerr << "infork read fd " << fifo.read_fd << "chart id" << i << std::endl;
			dup(fifo.read_fd);
			close(fifo.read_fd);
			close(fifo.write_fd);
			fifo.state = 0;
		}
		struct_utility::unlock();
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
			console::debug("pipe_handler");
			pipe_handler(cmds, pipe_i); // close or dup pipe
			console::debug("pipe_tofile");
			pipe_tofile(cmds, pipe_i); // check and stdout or open file
			console::debug("fifo_handler");
			fifo_handler(cmds, pipe_i);
			console::debug("exec");
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
		//*** IamUser (#3) just piped 'cat test.html | cat >1' to Iam1 (#1) ***
		for(int pipe_i=0; pipe_i<cmds.size(); pipe_i++)
		{
			auto &cmd = cmds[pipe_i];
			create_pipe(cmds, pipe_i);
			
			if(cmd.command_idx == 0) break;
			this->create_fifo(cmds, pipe_i);
			console::debug("run_command_impl");
			run_command_impl(cmds, pipe_i);	 // fork
			
			struct_utility::lock();
			// close fifo, the user receive msg, than close fifo
			if(struct_utility::find(chart_fifo, cmd.user_in, user_id) != -1)
			{
				console::debug("Close fifo of master in");
				
				int i = struct_utility::find(chart_fifo, cmd.user_in, user_id);
				auto &fifo = chart_fifo[i];
				
				close(fifo.read_fd);
				close(fifo.write_fd);
				
				std::string fifo_name;
				fifo_name += "fifo_" + std::to_string(cmd.user_in) + "_" + std::to_string(user_id);
				console::debug(fifo_name);
				unlink(fifo_name.c_str());
				//chart_fifo.erase({cmd.user_in, user_id});
				//fifo.state = 0;
			}
			struct_utility::unlock();
			
			// close pipe
			auto iter = pipes.find(cmd.command_idx);
			if( iter != pipes.end())
			{
				console::debug("Close pipe" + std::to_string(iter->second.first) + std::to_string(iter->second.second));
				close(iter->second.first);
				close(iter->second.second);
				pipes.erase(iter);
			}
			int kk;
			wait(&kk);
			fifo_to_user_msg(cmd);
		}
	}
	
};
template<class METHOD>
int shell<METHOD>::g_user_id;


#include "multiple_process.hpp"
#include "single_process.hpp"

#endif
