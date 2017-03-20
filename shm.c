#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SHM_SIZE 1024  /* make it a 1K shared memory segment */
#define MAX_RETRIES 10

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};

/*
** initsem() -- more-than-inspired by W. Richard Stevens' UNIX Network
** Programming 2nd edition, volume 2, lockvsem.c, page 295.
*/
int initsem(key_t key, int nsems)  /* key from ftok() */
{
    int i;
    union semun arg;
    struct semid_ds buf;
    struct sembuf sb;
    int semid;

    semid = semget(key, nsems, IPC_CREAT | IPC_EXCL | 0666);

    printf("Semget() returned %d \n",semid);

    if (semid >= 0) 
    { /* we got it first */
        sb.sem_op = 1; sb.sem_flg = 0;
        arg.val = 1;
        for(sb.sem_num = 0; sb.sem_num < nsems; sb.sem_num++) 
        { 
            /* do a semop() to "free" the semaphores. */
            /* this sets the sem_otime field, as needed below. */
            if (semop(semid, &sb, 1) == -1) 
            {
                int e = errno;
                semctl(semid, 0, IPC_RMID); /* clean up */
                errno = e;
                return -1; /* error, check errno */
            }
        }

    } 
    else if (errno == EEXIST) 
    { /* someone else got it first */
        int ready = 0;

        semid = semget(key, nsems, 0); /* get the id */
        if (semid < 0) return semid; /* error, check errno */

        /* wait for other process to initialize the semaphore: */
        arg.buf = &buf;
        for(i = 0; i < MAX_RETRIES && !ready; i++) 
        {
            semctl(semid, nsems-1, IPC_STAT, arg);
            if (arg.buf->sem_otime != 0) 
            {
                ready = 1;
            } 
            else 
            {
                sleep(1);
            }
        }
        if (!ready) {
            errno = ETIME;
            return -1;
        }
    } else {
        return semid; /* error, check errno */
    }

    return semid;
}
int main(int argc, char *argv[])
{
    key_t key;
	pid_t pid;
    int shmid;
    char *data;
    int mode;
	char *str;
    int semid;
    struct sembuf sb;
    
    sb.sem_num = 0;
    sb.sem_op = -1;  /* set to allocate resource */
    sb.sem_flg = SEM_UNDO;
	printf("enter string: ");
	scanf("%s",str);
    /* make the key: */
    if ((key = ftok("shm.c", 'R')) == -1) {
        perror("ftok");
        exit(1);
    }
	if ((semid = initsem(key, 1)) == -1) {
        perror("initsem");
        exit(1);
    }
    /* connect to (and possibly create) the segment: */
    if ((shmid = shmget(key, SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }
	if((pid=fork())==-1)
	{
		perror("fork");
		exit(1);
	}

	if(pid==0)		//child process collects data from the shared memory
	{	
		/*lock the resource*/
		if (semop(semid, &sb, 1) == -1) 
		{
		    perror("semop");
		    exit(1);
    	}
    	/* attach to the segment to get a pointer to it: */
		data = shmat(shmid, (void *)0, 0);
	
		if (data == (char *)(-1)) 
		{
		    perror("shmat");
		    exit(1);
		}

		/* read the segment, */
			printf("****Child Process****\n");
		    printf("segment contains: \"%s\"\n", data);

		/* detach from the segment: */
		if (shmdt(data) == -1) 
		{
		    perror("shmdt");
		    exit(1);
		}
		sb.sem_op = 1; /* free resource */
		if (semop(semid, &sb, 1) == -1) 
		{
		    perror("semop");
		    exit(1);
		}
	}
	else		//parent process writes data to the shared mermory
	{
		/*lock the resource*/
		if (semop(semid, &sb, 1) == -1) 
		{
		    perror("semop");
		    exit(1);
    	}
    	/* attach to the segment to get a pointer to it: */
		data = shmat(shmid, (void *)0, 0);
	
		if (data == (char *)(-1)) 
		{
		    perror("shmat");
		    exit(1);
		}

		/* modify the segment*/
		printf("****Parent Process****\n");
	    printf("writing to segment: \"%s\"\n", str);
	    strncpy(data, str, SHM_SIZE);
		/* detach from the segment: */
		if (shmdt(data) == -1) 
		{
		    perror("shmdt");
		    exit(1);
		}
			sb.sem_op = 1; /* free resource */
		if (semop(semid, &sb, 1) == -1) 
		{
		    perror("semop");
		    exit(1);
		}
	}
    return 0;
}
