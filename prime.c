

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#define READ_END 0
#define WRITE_END 1
#define END_MARKER -1
#define KILL_MARKER -2


struct Node
{
	int data;
	struct Node *next;
};

struct Queue
{
	struct Node *head;
	struct Node *tail;
};

int getHead( struct Queue *queue){
	return queue->head->data;
}

void pop( struct Queue *queue){
	struct Node *temp = queue->head;
	queue->head = queue->head->next;
	free(temp);
}

void initQ( struct Queue *queue){
	queue->head = NULL;
	queue->tail = NULL;
}

int isEmpty( struct Queue *queue){
	return( queue->head == NULL);
}

void push( struct Queue *queue, int data){
	if( queue->head == NULL){
		queue->head = ( struct Node *)malloc( sizeof(struct Node));
		queue->head->data = data;
		queue->head->next = NULL;
		queue->tail = queue->head;
	}else{
		queue->tail->next = ( struct Node *)malloc( sizeof( struct Node));
		queue->tail->next->data = data;
		queue->tail->next->next = NULL;
		queue->tail = queue->tail->next;
	}
}

int main( int argc, char *argv[]){
	int n, m;
	//Linked list structure for buffering of the sequence
	struct Queue cycleQ;
	initQ( &cycleQ);

	//Read arguments
	n = atoi( argv[1]);
	m = atoi( argv[2]);
	int pids[m+1];

	//Check arguments whether are valid
	//check largest integer in sequence
	if ( n < 1000 || n > 1000000)
	{
		/* code */
		printf("Invalid argument value for largest integer\nLargest integer must be between 1K(1000) and 1M(1000000)\n");
		exit(1);
	}
	//check number of children for process
	if( m < 1 || m > 50){
		printf("Invalid argument value for number of child processes\nNumber of processes must be between 1 and 50\n");
		exit(1);
	}

	//pipes connect children processes
	//for m children there are m+1 pipes( +1 for main process)
	int pipes[m+1][2];
	for( int i = 0; i < m+1; i++){
		if( pipe(pipes[i]) == -1){
			fprintf(stderr, "%s\n", "Pipe failed!");
		}
	}


	//only one pipe is needed for PR
	int pipe_pr [2];
	fcntl(pipes[m][READ_END], F_SETFL, O_NONBLOCK);

	//keep the main process id
	int main_pid = getpid();

	if( pipe( pipe_pr) == -1){
		fprintf(stderr, "%s\n", "Pipe failed!");
	}
	
	int i;
	for (i = 0; i < m + 1; ++i) // +1 for PR
	{
		//create m +1 child processes ( + 1 for pr)
		pids[i] = fork();
		if( pids[i] < 0){
			printf("%s\n","Error during fork");
		}else if( pids[i] == 0){
			int childIndex = i;
			int read_number;
			int taken_prime = END_MARKER;
			while(1){
				if( childIndex == m){
					if( read(pipe_pr[READ_END], &read_number, sizeof(read_number)) > 0){
						if( read_number == KILL_MARKER){
							exit(1);
						}
						printf("%d\n", read_number);
					}
				}else{
					if( read( pipes[i][READ_END], &read_number, sizeof(read_number)) > 0){
						if( read_number == KILL_MARKER){
							write( pipes[i+1][WRITE_END], &read_number, sizeof( read_number));
							exit(1);
						}
						if( read_number == END_MARKER){
							taken_prime = END_MARKER;
							write( pipes[i+1][WRITE_END], &read_number, sizeof( read_number));
						}else{
							if( taken_prime == END_MARKER){
								taken_prime = read_number;
								write( pipe_pr[WRITE_END], &taken_prime, sizeof(taken_prime));
							}else if( read_number % taken_prime != 0){
								write( pipes[i+1][WRITE_END], &read_number, sizeof( read_number));
							}
						}
					}
				}
			}
			break;
		}
	}

	if( getpid() == main_pid){
		//Check main Process
		int number = 2;
		int queueStartFlag = 0;
		int read_number;
		int end = END_MARKER;
		while(1){
			if( number <= n + 1){
				if( number == n + 1){
					write( pipes[0][WRITE_END], &end, sizeof(end));
				}else
					write( pipes[0][WRITE_END], &number, sizeof(number));
				number++;
			}
			if(queueStartFlag){
					int push_number = getHead(&cycleQ);
					//printf("in queue %d\n", push_number);
					pop( &cycleQ);
					write( pipes[0][WRITE_END], &push_number, sizeof(push_number));
					if( isEmpty(&cycleQ)){
						queueStartFlag = 0;
					}
			}			
			if( read( pipes[m][READ_END], &read_number, sizeof(read_number)) > 0){
				if( read_number == END_MARKER && !isEmpty(&cycleQ)){
					push( &cycleQ, read_number);
					queueStartFlag = 1;
				}else if( read_number == END_MARKER && isEmpty(&cycleQ))
				{
					int push_number = KILL_MARKER;
					write( pipes[0][WRITE_END], &push_number, sizeof(push_number));
					write( pipe_pr[WRITE_END], &push_number, sizeof(push_number));
					break;
				}
				else
					push( &cycleQ, read_number);
			}

		}
		
		for (int k = 0; k < m+1; ++k)
		{
			/* code */
			wait( NULL);
		}
		exit(1);
	}
}
