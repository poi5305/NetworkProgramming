#ifndef __SHELL__
#define __SHELL__


#define SEMKEY 55665566L
#define PERMS 0666
#define BIGCOUNT 100

#define SHMKEY ((key_t) 5567)
#define SEMKEY1 ((key_t) 5568)
#define SEMKEY2 ((key_t) 5569)


static struct sembuf op_endcreate[2] =
{
	1, -1, SEM_UNDO,
	2, -1, SEM_UNDO
};
static struct sembuf op_lock[2] =
{
	2, 0, 0, /* wait for sem#0 to become 0 */
	2, 1, SEM_UNDO /* then increment sem#0 by 1 */
};
static struct sembuf op_unlock[1] =
{
	2, -1, SEM_UNDO /* decrement sem#0 by 1 (sets it to 0) */
};
static struct sembuf op_open[1] =
{
	1, -1, SEM_UNDO
};
static struct sembuf op_close[3] =
{
	2, 0, 0, /* wait for sem#0 to become 0 */
	2, 1, SEM_UNDO, /* then increment sem#0 by 1 */
	1, 1, SEM_UNDO
};
static struct sembuf op_op[1] =
{
	0, 99, SEM_UNDO
};


semun semctl_arg;

int sem_create(key_t key, int initval)
{
	int id, semval;
	if (key == IPC_PRIVATE) return(-1); /* not intended for private semaphores */ 
	else if (key == (key_t) -1) return(-1); /* probably an ftok() error by caller */
	
	if ( (id = semget(key, 3, 0666 | IPC_CREAT)) < 0) // key, number of sem, flag
		return(-1);
	
	again:
	if (semop(id, &op_lock[0], 2) < 0) //id, op, number of ops
	{
		if (errno == EINVAL)
			goto again;
		console::error("can't lock");
	}
	if ( (semval = semctl(id, 1, GETVAL, 0)) < 0) // id, sem number, cmd, arg
		console::error("can't GETVAL");
	if (semval == 0)
	{
		semctl_arg.val = initval;
		if (semctl(id, 0, SETVAL, semctl_arg) < 0)
			console::error("can’t SETVAL[0]");
		semctl_arg.val = BIGCOUNT;
		if (semctl(id, 1, SETVAL, semctl_arg) < 0)
			console::error("can’t SETVAL[1]");
	}
	if (semop(id, &op_endcreate[0], 2) < 0)
		console::error("can't end create");
	return id;
}
void sem_rm(int id)
{
	if (semctl(id, 0, IPC_RMID, 0) < 0)
		console::error("can't IPC_RMID");
}
int sem_open(key_t key)
{
	int id;
	if (key == IPC_PRIVATE) return(-1);
	else if (key == (key_t) -1) return(-1);
	if ( (id = semget(key, 3, 0)) < 0)
		return(-1);
	// Decrement the process counter. We don't need a lock to do this.
	if (semop(id, &op_open[0], 1) < 0)
		console::error("can't open"); 
	return(id);
}
void sem_close(int id)
{
	int semval;
	// The following semop() first gets a lock on the semaphore,
	// then increments [1] - the process counter.
	if (semop(id, &op_close[0], 3) < 0)
		console::error("can't semop");
	// if this is the last reference to the semaphore, remove this.
	if ( (semval = semctl(id, 1, GETVAL, 0)) < 0)
		console::error("can't GETVAL");
	if (semval > BIGCOUNT)
		console::error("sem[1] > BIGCOUNT");
	else if (semval == BIGCOUNT)
		sem_rm(id);
	else
		if (semop(id, &op_unlock[0], 1) < 0)
			console::error("can't unlock"); /* unlock */	
}
void sem_op(int id, int value)
{
	if ( (op_op[0].sem_op = value) == 0)
		console::error("can't have value == 0");
	if (semop(id, &op_op[0], 1) < 0)
		console::error("sem_op error");
}
void sem_wait(int id) 
{
	sem_op(id, -1); 
}
void sem_signal(int id)
{
	sem_op(id, 1);
}
void my_lock(int semid) 
{
	if (semid < 0)
	{
		if ( (semid = sem_create(SEMKEY, 1)) < 0)
			console::error("sem_create error"); 
	}
	sem_wait(semid);
}
void my_unlock(int semid)
{
	sem_signal(semid);
}


#endif