 #include <stdlib.h>
#include <stdio.h>
#include "device-controller.h"

int buffer_size;
int queue_size = 0;
int* queue; 
int tail = -1;
int head = 0;

int is_full() {
	if(queue_size >= buffer_size)
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
	enqueue(c);	
	write_device(queue[head]);
}

void write_done_interrupt() {
	int c = dequeue();
	write_device(queue[head]);
}


int main(int argc, char* argv[]) {
	buffer_size = atoi(argv[1]) + 1;            	//need "+ 1" to produce correct solution ¯\_(ツ)_/¯
	queue = malloc(buffer_size * (sizeof(int)));
	start();
	return 0;
}



