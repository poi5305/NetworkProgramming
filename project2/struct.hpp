#ifndef __STRUCT__
#define __STRUCT__
#include <string>

#define USER_LEN 10
#define FIFO_LEN 20
#define MSG_LEN 200
#define MSG_NUM 5

struct user2
{
	int socket_fd;
	std::string name;
	std::string port;
	std::string ip;
};

struct user
{
	int state;
	int socket_fd;
	int pid;
	char port[6];
	char name[30];
	char ip[15];
};

struct fifo
{
	int state;
	int from_user;
	int to_user;
	int read_fd;
	int write_fd;
};

struct msg
{
	int state;
	int from_user;
	int to_user;
	char msg[MSG_LEN];
};


struct shared
{
	user users[USER_LEN];
	fifo chart_fifo[FIFO_LEN];
	msg msgs[MSG_NUM];
	int broadcast_user;
	char broadcast[MSG_LEN];
};

shared *p_shared;

user *users;
fifo *chart_fifo;

class struct_utility
{
public:
	static int find(user us[], int key)
	{
		for(int i=1; i<USER_LEN; ++i)
		{
			if(us[i].state == 1 && i == key)
				return i;
		}
		return -1;
	}
	
	static int find(fifo cf[], int from_user, int to_user)
	{
		for(int i=0; i<FIFO_LEN; ++i)
		{
			if(cf[i].state > 0 && cf[i].from_user == from_user && cf[i].to_user == to_user)
				return i;
		}
		return -1;
	}
	static void remove(user us[], int key)
	{
		us[key].state = 0;
		us[key].socket_fd = 0;
		//us[key].port[0] = '\0';
		//us[key].name[0] = '\0';
		//us[key].ip[0] = '\0';
	}
	template<class ARRAY>
	static int get_new_id(ARRAY arr, int len, int start=0)
	{
		for(int i=start; i < len; i++)
		{
			//std::cerr << i << std::endl;
			if(arr[i].state == 0)
				return i;
		}
		return -1;
	}
	static void active_user(int user_id, int socket_fd, int pid, std::string socket_name, std::string ip, std::string port)
	{
		users[user_id].state = 1;
		users[user_id].socket_fd = socket_fd;
		users[user_id].pid = pid;
		strcpy(users[user_id].name, socket_name.c_str());
		strcpy(users[user_id].ip, ip.c_str());
		strcpy(users[user_id].port, port.c_str());
	}
};


#endif