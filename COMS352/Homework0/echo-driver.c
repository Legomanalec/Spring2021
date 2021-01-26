 #include <stdlib.h>
#include <stdio.h>
#include "device-controller.h"

int buffer_size;
int queue_size = 0;
int* queue; 
int tail = -1;
int head = 0;
int writer_ready = 1;

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
	if(writer_ready == 1){
		write_device(c);
		writer_ready = 0;
	}		
	else
		enqueue(c);	
}

void write_done_interrupt() {
	writer_ready = 1;
}

int main(int argc, char* argv[]) {
	buffer_size = atoi(argv[1]);
	queue = malloc(buffer_size * (sizeof(int)));
	start();
	for(int i = 0; i < 10; i++)
		printf("%d\n", queue[i]);
	return 0;
}

