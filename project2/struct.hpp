#ifndef __STRUCT__
#define __STRUCT__
#include <string>
#include <string.h>
#include <semaphore.h>  /* Semaphore */

#define SHMKEY ((key_t) 5567)
#define PERMS 0666
int shmid, clisem, servsem;

#define USER_LEN 30
#define FIFO_LEN 60
#define MSG_LEN 1024
#define MSG_NUM 10

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
sem_t *mutex;

char SEM_NAME[]= "ggg78";

class struct_utility
{
public:
	static void shm_init()
	{
		std::cout << "shared size " << sizeof(shared) << std::endl;
		if ((shmid = shmget(SHMKEY, sizeof(shared), PERMS|IPC_CREAT))<0)
			console::error("server: can't get shared memory");	
		if ( (p_shared = (shared *)shmat(shmid, (char *) 0, 0)) == 0)
			console::error("...");
		memset((void*) p_shared, 0, sizeof(shared));
				
		mutex = sem_open(SEM_NAME, O_CREAT,0644,1);
		if(mutex == SEM_FAILED)
	    {
		    perror("unable to create semaphore");
			sem_unlink(SEM_NAME);
			exit(1);
	    }

		signal(SIGINT, [](int k){
			shmdt(p_shared);
			shmctl(shmid, IPC_RMID, NULL);
			//sem_destroy(&mutex);
			sem_close(mutex);
			sem_unlink(SEM_NAME);
			exit(0);
		});
	}
	static void lock()
	{
		sem_wait(mutex);
	}
	static void unlock()
	{
		sem_post(mutex);
	}
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
