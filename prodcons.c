//Cody Whited
//cjw51@pitt.edu
//CS 1550 - Project 2
//Note: need compiled with -lm for math.h functions (log and ceil)

#include <linux/unistd.h>
#include <linux/mman.h>
#include <stdlib.h>
#include <stdio.h>

//process queue structs
struct qnode
{
	struct task_struct *process;
	struct qnode *next;
};
struct queue
{
	struct qnode *head;
	struct qnode *tail;
};
//semaphore struct
struct cs1550_sem
{
   int value;
   struct queue *processQueue;
};

//wrappers for down and up syscalls

void down(struct cs1550_sem *sem);
void up(struct cs1550_sem *sem);

void down(struct cs1550_sem *sem) 
{
       syscall(__NR_cs1550_down, sem);
}
void up(struct cs1550_sem *sem) 
{
       syscall(__NR_cs1550_up, sem);
}

int main(int argc, const char * argv[])
{
	//parse command line input
	if(argc < 4)
	{
		printf("Usage: ./prodcons consumers producers buffersize\n");
		exit(1);
	}
	int consumers = atoi(argv[1]);
	int producers = atoi(argv[2]);
	int n = atoi(argv[3]);
	if(n <= 0 || producers <= 0 || consumers <= 0)
	{
		printf("Parameters must be greater than zero\n");
		exit(1);
	}
	
	//setup shared resources for all processes
	struct cs1550_sem *empty, *full, *mutex;
	empty = (struct cs1550_sem *) mmap(NULL, sizeof(struct cs1550_sem), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	empty->processQueue = (struct queue *) mmap(NULL, sizeof(struct queue), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	empty->value = n;
	full = (struct cs1550_sem *) mmap(NULL, sizeof(struct cs1550_sem), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	full->processQueue = (struct queue *) mmap(NULL, sizeof(struct queue), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	full->value = 0;
	mutex = (struct cs1550_sem *) mmap(NULL, sizeof(struct cs1550_sem), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	mutex->processQueue = (struct queue *) mmap(NULL, sizeof(struct queue), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	mutex->value = 1;
		
	int * buffer = (int *) mmap(NULL, n*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	
	//split processes into producers and consumers
	int pid = fork();
	if(pid < 0)
	{
		printf("fork() failed\n");
		exit(1);
	}
	
	//setup shared resources for producers/consumers
	int * index = (int *) mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	int * item = (int *) mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	*index = 0;
	*item = 0;
	
	if(pid == 0) //producers
	{	
		//create producer processes
		int i, j;
		j = (int) ceil(log(producers)/log(2)); //smallest number of times to fork() to create requested producers
		
		int * count = (int *) mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0); //a shared counter for the number of processes running
		*count = 0;
		
		for(i=0; i<j; i++)
			fork();
		
		if((*count) >= producers)
			exit(1); //close immediately any forks created over the requested number
		
		char base = 'A';
		char label = base + (*count); //label each process A-Z
		(*count)++;

		while(1)
		{
			//produce an item
			down(empty);
			down(mutex);
			
			printf("Producer %c Produced: %i\n", label, *item);
			buffer[*index] = *item;
			
			*index = ((*index)+1) % n; //wrap around buffer
			(*item)++; //add one to item
			
			up(mutex);
			up(full);
		}
	}
	else //consumers
	{
		//create and assign char labels to consumers processes
		int i, j;
		j = (int) ceil(log(consumers)/log(2)); //smallest number of times to fork() to create requested consumers
		int * count = (int *) mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0); //a shared counter for the number of processes running
		*count = 0;
		
		for(i=0; i<j; i++)
			fork();
		
		if((*count) >= consumers)
			exit(1); //close immediately any forks created over the requested number
			
		char base = 'A';
		char label = base + (*count); //label each process A-Z
		(*count)++;
		
		while(1)
		{
			//consume an item
			down(full);
			down(mutex);
			
			*item = buffer[*index];
			*index = ((*index)+1) % n; //wrap around buffer
			printf("Consumer %c Consumed: %i\n", label, *item);
			
			up(mutex);
			up(empty);
		}
	}
	
	//cleanup resources
	munmap(empty);
	munmap(full);
	munmap(mutex);
	munmap(buffer);
	return 0;
}