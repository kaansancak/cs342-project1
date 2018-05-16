#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <mqueue.h>
#include <signal.h>
#define END_MARKER -1
#define KILL_MARKER -2


struct item
{
	int value;
};

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

	//read arguments
	n = atoi( argv[1]); //largest number
	m = atoi( argv[2]);	//number of children

	//Linked list structure for buffering of the sequence
	struct Queue cycleQ;
	initQ( &cycleQ);

	//check whether arguments are valid
	//check largest integer in the sequence
	
	if ( n < 1000 || n > 1000000)
	{
	
		printf("Invalid argument value for largest integer\nLargest integer must be between 1K(1000) and 1M(1000000)\n");
		exit(1);
	}

	//check number of children for process
	if( m < 1 || m > 5){
		printf("Invalid argument value for number of child processes\nNumber of processes must be between 1 and 5\n");
		exit(1);
	}

	//keep the main process id
	int main_pid = getpid();

	//Create message queues
	mqd_t mqs[m+1];
	int pids[m+1];

	for (int i = 0; i < m + 1 ; ++i)
	{
		/* code */
		char mqName[20];
		snprintf( mqName, 20, "/MQ_%d", i);

		if( i == m ){
			mqs[i] = mq_open(mqName, O_RDWR | O_CREAT|O_NONBLOCK, 0666, NULL);
			if( mqs[i] == -1 ){
				perror("can not open msg queue!");
				exit(1);
			}
		}else {

		mqs[i] = mq_open(mqName, O_RDWR | O_CREAT , 0666, NULL);
		if( mqs[i] == -1 ){
			perror("can not open msg queue!");
			exit(1);
		}
	}
	}



	/*
	for (int i = 0; i < m + 1; ++i)
	{
	
		char mqName[100];
		snprintf( mqName, 100, "/MQ_%d", i);
		//mq_unlink( mqName);
		close(mqs[i]);
	}
	exit(1);*/

	mqd_t mq_pr = mq_open( "/MQ_PR", O_RDWR| O_CREAT, 0666, NULL);
	if (mq_pr == -1)
	{
		/* code */
		perror("can not open msq queue!");
		exit(1);
	}


	int i;
	for( i = 0; i < m + 1; i++){
		//create child processes
		pids[i] = fork();
		if( pids[i] < 0){
			printf("%s\n", "Error during fork" );
		}else if( pids[i] == 0){
			//In child processes
			int childIndex = i;
			int read_number;
			int taken_prime = END_MARKER;
			struct mq_attr mq_attr;
			struct item *itemptr;
			struct item send_item;
			int send_code;
			char *bufptr;
			int buflen;

			while(1){
				if( childIndex == m){
					mq_getattr( mq_pr, &mq_attr);

					buflen = mq_attr.mq_msgsize;
					bufptr = ( char *)malloc( buflen);
					while(1) {
						send_code = mq_receive( mq_pr, (char *)bufptr, buflen, NULL);
						if( send_code == -1){
							perror("mq_receive pr failed\n");
							exit(1);
						}


						if( send_code > 0){
						itemptr = ( struct item *) bufptr;
						if( itemptr ->value == KILL_MARKER)
						{	
							break;
						}
						printf("%d\n",itemptr->value );
						}
					}	
					free( bufptr);
					exit(1);
				}else{
					mq_getattr( mqs[i], &mq_attr);

					buflen = mq_attr.mq_msgsize;
					bufptr = ( char *)malloc( buflen);
					while(1) {
						send_code = mq_receive( mqs[childIndex], (char *)bufptr, buflen, NULL);
						if( send_code == -1){
							continue;
							perror("mq_receive failed\n");
						}else if( send_code > 0){
							itemptr = ( struct item *)bufptr;
							read_number = itemptr->value;
							send_item.value = read_number;
							//printf("child %d received number: %d\n",childIndex, read_number );
							//printf("received: %d\n", read_number);
							if( read_number == -2){
								send_item.value = -2;
								send_code = mq_send( mqs[childIndex+1], (char *)&send_item, sizeof( struct item), 0);
								if( send_code == -1)
									while( mq_send( mqs[childIndex+1], (char *)&send_item, sizeof( struct item), 0) == -1 );
								break;
							}
							if( read_number == -1){
								taken_prime = END_MARKER;
								send_item.value = -1;
								send_code = mq_send( mqs[childIndex+1], (char *)&send_item, sizeof( struct item), 0);
								if( send_code == -1)
									while( mq_send( mqs[childIndex+1], (char *)&send_item, sizeof( struct item), 0) == -1 );
							}else{
								if( taken_prime == -1){
									taken_prime = read_number;
									mq_send( mq_pr, (char *)&send_item, sizeof( struct item), 0);
								}else if( read_number % taken_prime != 0){
									send_code = mq_send( mqs[childIndex+1], (char *)&send_item, sizeof( struct item), 0);
									if( send_code == -1)
										while( mq_send( mqs[childIndex+1], (char *)&send_item, sizeof( struct item), 0) == -1);
								}
							}
							
						}
					}
					free( bufptr);
					exit(1);
				}
			}

			//interrupt child proccesses create children
			break;
		}
	}

	if( getpid() == main_pid){
		//Manage main process
		int number = 2;
		int queueStartFlag = 0;


		while(1){
		int read_number;
		struct item send_item;
		int send_code = -1;
		struct mq_attr mq_attr;
		struct item *itemptr;
		char *bufptr;
		int buflen;
			if( number <= n + 1){
				if( number == n + 1){
					send_item.value = END_MARKER;
				}else{
					send_item.value = number;
				}	
				send_code = mq_send( mqs[0], (char *)&send_item, sizeof( struct item), 0);
				if( send_code == -1 ){
					while( mq_send( mqs[0], (char *)&send_item, sizeof( struct item), 0) == -1);
				}
				//printf("sending %d\n", send_item.value );
				number++;
			}
			if( queueStartFlag){
				int push_number = getHead(&cycleQ);
				send_item.value = push_number;
				send_code = mq_send( mqs[0], (char *)&send_item, sizeof( struct item), 0);
				if( send_code == -1 ){
					while( mq_send( mqs[0], (char *)&send_item, sizeof( struct item), 0) == -1);
				}
				pop( &cycleQ);
				if( isEmpty(&cycleQ)){
					queueStartFlag = 0;
				}
			}

			
			mq_getattr( mq_pr, &mq_attr);

			
			buflen = mq_attr.mq_msgsize;
			bufptr = ( char *)malloc( buflen);
		
			send_code = mq_receive( mqs[m], (char *)bufptr, buflen, NULL);
			
			if( send_code == -1 ){
				free(bufptr);
				continue;
			}
			else if( send_code > 0){
				itemptr = ( struct item *) bufptr;
				read_number = itemptr->value;
				
				//printf("main read %d\n",read_number );
				if( read_number == END_MARKER && !isEmpty(&cycleQ)){
					//printf("sequence ended\n");
					push( &cycleQ, read_number);
					queueStartFlag = 1;
					free(bufptr);
				}else if( read_number == END_MARKER && isEmpty(&cycleQ))
				{	
					send_item.value = KILL_MARKER;
					send_code = mq_send( mqs[0], (char *)&send_item, sizeof( struct item), 0);
					send_code = mq_send( mq_pr, (char *)&send_item, sizeof( struct item), 0);
					free(bufptr);
					break;
				}
				else
				{	
					free(bufptr);
					push( &cycleQ, read_number);
				}	
			}
		}
		for (int k = 0; k < m+1; ++k)
		{
			wait(NULL);
		}

		for( int k = 0; k < m+1; k++){
			char mqName[20];
			snprintf( mqName, 20, "/MQ_%d", k);
			mq_close(mqs[k]);
			mq_unlink(mqName);
		}

		mq_close(mq_pr);
		mq_unlink("/MQ_PR");
		
		exit(1);
	}

}
