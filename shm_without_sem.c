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

int main()
{
    key_t key;
	pid_t pid;
    int shmid;
    char *data;
    int mode;
	char *str;
    int semid;
    struct sembuf sb;
    
	printf("enter string: ");
	scanf("%s",str);
    /* make the key: */
    if ((key = ftok("shm_without_sem.c", 'R')) == -1) {
        perror("ftok");
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
	}
	else		//parent process writes data to the shared mermory
	{
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

	}
    return 0;
}
