#ifndef __SHELL__
#define __SHELL__

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>
#include "parser.hpp"

struct user
{
	int socket_fd;
	std::string name;
	std::string port;
	std::string ip;
};

class shell
{
	parser cmd_parser;
	std::map<std::string, std::string> ENV;
	std::map< int, std::pair<int,int> > pipes;
	
	int next_command_count;
	
	
	std::string socket_name;
	bool is_exit;
public:
	int user_id;
	int socket_fd;
	static std::map<int, user> users;
	static std::map<std::pair<int, int>, std::pair<int, int>> chart_fifo; // userid, userid, ??
	shell()
	{}
	shell(int sockfd, std::string sockname)
		:ENV({{"PATH","bin:."}}), next_command_count(0), socket_fd(sockfd), socket_name(sockname), is_exit(false)
	{
		user_id = get_user_new_id();
		users[user_id] = {socket_fd, socket_name, "", ""};
		update_fd();
		print_hello();
		
	}
	shell(int in, int out, int err)
		:ENV({{"PATH","bin:."}}), next_command_count(0)
	{
		if(in != 0) close(0);dup(in);
		if(out != 1) close(1);dup(out);
		//if(err != 2) close(2);dup(err);
		
		if(in == out && in == err)
			close(in);
		
		print_hello();
		while(true)
		{
			std::vector<command> pipes = cmd_parser.parse_line();
			if(exam_command(pipes))
				run_command(pipes);
			print_success();
		}
	}
	void update_ip_port(std::string ip, std::string port)
	{
		users[user_id].ip = ip;
		users[user_id].port = port;
	}
	void update_fd()
	{
		std::cerr << "close 0, 1. dup " << socket_fd << std::endl;
		close(0);dup(socket_fd);
		close(1);dup(socket_fd);
		//close(2);dup(socket_fd);
	}
	void update_fd(int fd)
	{
		if(fd < 3)
			console::error("Error, update_fd can't smaller than 3");
		std::cerr << "close 0, 1. dup " << fd << std::endl;
		close(0);dup(fd);
		close(1);dup(fd);
		//close(2);dup(socket_fd);
	}
	int run_one_time()
	{	
		console::debug("run_one_time start");
		update_fd();
		std::vector<command> pipes = cmd_parser.parse_line();
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
		const std::string hello = "****************************************\n** Welcome to the information server. **\n****************************************\n% ";
		std::cout << hello;
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
			std::cerr << "===DEBUG CMDS PARA=== (" << i << "): ";
			std::cerr << "pipe_num: " << pipe.pipe_num << "; ";
			std::cerr << "command_idx: " << pipe.command_idx << "; ";
			std::cerr << "is_end: " << pipe.is_end << "; ";
			std::cerr << "file1: " << pipe.file1 << "; ";
			std::cerr << "file2: " << pipe.file2 << "; ";
			std::cerr << "user_out: " << pipe.user_out << "; ";
			std::cerr << "user_in: " << pipe.user_in << "; ";
			std::cerr << "\n";
			std::cerr << "===DEBUG CMDS ARGV=== (" << i << "): ";
			for(auto &para : pipe.argv)
				std::cerr << para << " ";
			std::cerr << "\n";
			std::cerr << "===================== (" << i << ")\n";
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
						yell( std::string("*** User from ") + users[user_id].ip + "/" + users[user_id].port + " is named '" + users[user_id].name + "'. ***\n" );
					}
				}
				return false;
			}
			else if (cmd.argv[0] == "yell")
			{
				if(cmd.argv.size() > 1 && cmd.argv[1] != "")
				{
					std::string msg("");
					msg += "*** " + users[user_id].name + " yelled ***: ";//*** (sender's name) yelled ***: (message)
					for(int i=1; i<cmd.argv.size(); i++)
						msg += cmd.argv[i];
					yell( msg+"\n" );
				}
				return false;
			}
			else if(cmd.argv[0] == "tell")
			{
				if(cmd.argv.size() > 2 && cmd.argv[1] != "")
				{
					int to_user_id = atoi(cmd.argv[1].c_str());
					
					//*** Error: user #3 does not exist yet. ***
					if(users.find(to_user_id) == users.end())
					{
						std::cout << "*** Error: user #" << to_user_id << " does not exist yet. ***\n";
						std::cout.flush();
					}
					else
					{
						std::string msg(""); // *** (sender's name) told you ***: (message)
						msg += "*** " + users[user_id].name + " told you ***: ";
						for(int i=2; i<cmd.argv.size(); i++)
							msg += cmd.argv[i];
						tell(to_user_id, msg+"\n");
					}
					//
				}
				return false;
			}
			else if(cmd.argv[0] == "exit")
			{
				is_exit = true;
				users.erase(user_id);
				close(socket_fd);
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
		if(cmd.user_out != 0 && users.find(cmd.user_out) == users.end())
		{//*** Error: user #1 does not exist yet. ***
			std::cout << "*** Error: user #" << cmd.user_out << " does not exist yet. ***\n";
			std::cout.flush();
			return false;
		}
		if(cmd.user_out != 0 && chart_fifo.find({user_id, cmd.user_out}) != chart_fifo.end())
		{//*** Error: the pipe #2->#1 already exists. ***
			std::cout << "*** Error: the pipe #" << user_id << "->#" << cmd.user_out << " does not exist yet. ***\n";
			std::cout.flush();
			return false;
		}
		if(cmd.user_in != 0 && chart_fifo.find({cmd.user_in, user_id}) == chart_fifo.end())
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
			tmp += "*** "+users[user_id].name+" (#"+std::to_string(user_id)+") just piped '' to "+users[cmd.user_out].name+" (#"+std::to_string(cmd.user_out)+") ***\n";
			yell(tmp);
		}
		if(cmd.user_in != 0)
		{//*** IamUser (#3) just received from student7 (#7) by 'cat <7' ***
			std::string tmp = "";
			tmp += "*** "+users[user_id].name+" (#"+std::to_string(user_id)+") just received from "+users[cmd.user_in].name+" (#"+std::to_string(cmd.user_in)+") by '' ***\n";
			yell(tmp);
		}
	}
	void yell(const std::string &msg)
	{
		for(auto &u_pair : users)
		{
			auto &u_user_id = u_pair.first;
			auto &u_user = u_pair.second;
			std::cout.flush();
			update_fd(u_user.socket_fd);
			std::cout << msg ;
			//std::cerr << "yell" << u_user_id << " " << user_id << std::endl;
			if( u_user_id != user_id)
				print_success();
			std::cout.flush();
		}
		update_fd();
	}
	void tell(int to_user_id, const std::string &msg)
	{
		int to_fd = users[to_user_id].socket_fd;
		console::debug(std::string("tell to ")+std::to_string(to_user_id)+" "+msg);
		update_fd(to_fd);
		std::cout << msg ;
		if( to_user_id != user_id)
			print_success();
		std::cout.flush();
		update_fd(socket_fd);
	}
	int get_user_new_id()
	{
		int tmp = 1;
		for(auto &u_pair : users)
		{
			auto &u_user_id = u_pair.first;
			if(u_user_id == tmp)
				tmp++;
			else
				break;
		}
		return tmp;
	}
	bool is_exist_name(const std::string name)
	{
		for(auto &u_pair : users)
		{
			auto &u_user = u_pair.second;
			if(u_user.name == name)
				return true;
		}
		return false;
	}
	bool change_name(const std::string &name)
	{
		if(!is_exist_name(name))
		{
			console::debug("change name success");
			users[user_id].name = name;
			return true;
		}
		else
		{
			std::cout << "*** User '(" << name <<")' already exists. ***" << "\n";
			std::cout.flush();
			return false;
		}
	}
	void who()
	{
		std::cout << "<ID>	<nickname>	<IP/port>	<indicate me>\n";
		for(auto &u_pair : users)
		{
			auto &u_user_id = u_pair.first;
			auto &u_user = u_pair.second;
			std::cout << u_user_id << "\t" << u_user.name << "\t" << u_user.ip << "/" << u_user.port;
			if(u_user_id == user_id)
				std::cout << "\t<-me" << "\n";
			else
				std::cout << "\n";
		}
		std::cout.flush();
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
	void create_fifo(std::vector<command> &cmds, int pipe_i)
	{
		auto &cmd = cmds[pipe_i];
		if(cmd.user_out != 0)
		{
			std::cerr << "A" << user_id << " " << cmd.user_out << std::endl;
			std::string fifo_name;
			fifo_name += "fifo_" + std::to_string(user_id) + "_" + std::to_string(cmd.user_out);
			
			console::debug(fifo_name);
			
			unlink(fifo_name.c_str());
			if( mknod( fifo_name.c_str(),  S_IFIFO | 0666, 0 ) < 0 )
				console::error("Can't mkfifo");
			int fifo_0, fifo_1;
			
			if( (fifo_0 = open( fifo_name.c_str(), O_RDONLY|O_NONBLOCK )) < 0)
				console::error("Can't open fifo 0");
			if( (fifo_1 = open( fifo_name.c_str(), O_WRONLY )) < 0)
				console::error("Can't open fifo 1");
			chart_fifo[{user_id, cmd.user_out}] = {fifo_0, fifo_1};
			std::cerr << "B" << fifo_0 << " " << fifo_1 << std::endl;
			//std::cerr << "aa" << fifo << chart_fifo[{user_id, cmd.user_out}] << std::endl;
			//close(1);
			//dup(fifo);
			//close(0);
			//close(fifo);
		}
		if(cmd.user_in != 0)
		{}
	}
	void fifo_handler(std::vector<command> &cmds, int pipe_i)
	{ // in fork
		auto &cmd = cmds[pipe_i];
		if(cmd.user_out != 0)
		{
			auto fifo = chart_fifo[{user_id, cmd.user_out}];
			
			
			//std::cerr << "aa" << fifo << chart_fifo[{user_id, cmd.user_out}] << std::endl;
			close(1);
			dup(fifo.second);
			//close(0);
			//shutdown(1, SHUT_RD);
			//close(fifo);
			close(fifo.first);
			close(fifo.second);
		}
		if(cmd.user_in != 0)
		{
			//std::cerr << "aaaa3" << cmd.user_in << " " <<  user_id << chart_fifo.size()<< std::endl;
			auto fifo = chart_fifo[{cmd.user_in, user_id}];
			//std::string fifo_name;
			//fifo_name += "fifo_" + std::to_string(cmd.user_in) + "_" + std::to_string(user_id);
			//console::debug(fifo_name);
			//int fifo;
			//if( (fifo = open( fifo_name.c_str(), O_RDWR )) < 0)
			//	console::error("Can't open fifo");
			
			//console::debug(fifo_name);
			close(0);
			dup(fifo.first);
			//shutdown(1, SHUT_WR);
			//shutdown(fifo);
			std::cerr << fifo.first << fifo.second << std::endl;
			//close(fifo);
			close(fifo.first);
			close(fifo.second);
			//chart_fifo.erase({cmd.user_out, user_id});
			
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
			create_fifo(cmds, pipe_i);
			console::debug("run_command_impl");
			run_command_impl(cmds, pipe_i);	 // fork
			
			// close fifo, the user receive msg, than close fifo
			if(chart_fifo.find({cmd.user_in, user_id}) != chart_fifo.end() )
			{
				console::debug("Close fifo of master");
				//close(1);
				auto fifo = chart_fifo[{cmd.user_in, user_id}];
				//close(fifo);
				close(fifo.first);
				close(fifo.second);
				//close(7); close(6);
				//std::cout << "close " << chart_fifo[{cmd.user_in, user_id}] << std::endl;
				std::string fifo_name;
				fifo_name += "fifo_" + std::to_string(cmd.user_in) + "_" + std::to_string(user_id);
				console::debug(fifo_name);
				unlink(fifo_name.c_str());
				chart_fifo.erase({cmd.user_in, user_id});
				
			}
			//update_fd();
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
std::map<int, user> shell::users;
std::map<std::pair<int, int>, std::pair<int, int>> shell::chart_fifo;
#endif