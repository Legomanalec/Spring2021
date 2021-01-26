#include <stdlib.h>
#include <stdio.h>
#include "device-controller.h"

int buffer_size;
int queue_size = 0;
int* queue; 
int tail = -1;
int head = 0;

int is_full() {
	if(queue_size == buffer_size)
		return 1;
	return 0;
}

int is_empty() {
	if(queue_size == 0)
		return 1;
	return 0;
}

void enqueue(int val) {
	if(is_full() == 0){
		tail = (tail + 1) % buffer_size;
		queue[tail] = val;
		queue_size++;
	}
}

int dequeue() {
	if(is_empty() == 0){
		int tmp = queue[head];
		head = (head + 1) % buffer_size;
		queue_size--;
		return tmp;
	}
	return 0;
}

void read_interrupt(int c) {
	//TODO
}

void write_done_interrupt() {
	//TODO
}

int main(int argc, char* argv[]) {
	buffer_size = atoi(argv[1]);
	queue = malloc(buffer_size * (sizeof(int)));
	enqueue(5);
	enqueue(10);
	enqueue(15);
	enqueue(20);
	enqueue(25);
	enqueue(30);
	dequeue();
	dequeue();
	enqueue(35);
	enqueue(40);

	for(int i = 0; i<buffer_size; i++)
	{
		if(tail == i)
			printf("tail ");
		if(head == i)
			printf(" head ");
		printf("%d\n", queue[i]);
	}
	

	//start();
	return 0;
}

