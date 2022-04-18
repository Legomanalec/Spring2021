/*
Author: Alec Meyer
This is my implementation of a multi-threaded
encryptor. It uses 10 semaphores to ensure stability 
when performing atomic, data-sharing-tasks. There is 
also a single mutex lock for when the encryptor
reset takes place to ensure a safe reset. This 
program as max concurrency as the only non-concurrent
actions are when one of the shared buffers are being 
accessed. 

I used a ring buffer for both buffers as it was the 
easiest to implement and made the most sense to me. 
*/
#include "encrypt-module.h"
#include "encrypt-module.c"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

//input buffer
char *inputBuffer;
int inSize;
int inItems = 0;

//output Buffer
char *outputBuffer;
int outSize;
int outItems = 0;


//used for when buffers are empty/full
sem_t inBufferFull; 
sem_t inBufferOpen; 
sem_t outBufferFull; 
sem_t outBufferOpen; 

//used for critical section for input/output buffers
sem_t inBufferMutex; 
sem_t outBufferMutex; 

//used for critical section to wait for encrypted 
//characters in the buffers
sem_t forceEncrypted; 
sem_t forceNotEncrypted; 

//used to wait if no characters to count
sem_t outCount; 
sem_t outToCount;


//buffer indexes
int outWriterPtr = 0;
int inReaderPtr = 0;
int inCaesarPtr = 0;
int outCaesarPtr = 0;
int inCountPtr = 0;
int outCountPtr = 0;

//lock for reset request and finish
pthread_mutex_t reset_lock;

/* This thread will write characters from a specified 
file into the input buffer */
void *reader_thread(void *n) {
    char x;
    int localInSize = inSize;
    while(x != EOF) {

        x = read_input();
        sem_wait(&forceEncrypted);
        sem_wait(&inBufferOpen);
        sem_wait(&inBufferMutex);

            //critical section for added elements to the input buffer
            inputBuffer[inReaderPtr] =  x;
            inReaderPtr = (inReaderPtr + 1) % localInSize;

        sem_post(&inBufferMutex);
        sem_post(&inBufferFull);
        sem_post(&forceNotEncrypted);
    } 


} 

/* This thread will count the occurrences of each 
character from the input buffer */
void *input_counter_thread(void *n) {
    char x;
    while(x != EOF){

        sem_wait(&inBufferFull);
        sem_wait(&inBufferMutex);

            //critical section for counting an element from the input buffer
            x = inputBuffer[inCountPtr];
            inCountPtr = (inCountPtr + 1) % inSize;

        sem_post(&inBufferMutex);
        sem_post(&inBufferOpen);

        if(x != EOF)
            count_input(x);

    }
}

/* This thread will take characters from the input buffer, 
encrypt them, and then send them to the output buffer */
void *encryption_thread(void *n) {
    char x;
    int localInSize = inSize;
    int localOutSize = outSize;

    while(x != EOF) 
    {
        sem_wait(&forceNotEncrypted);
        sem_wait(&inBufferMutex);
        
            //critical section for preparing a element for encryption
            x = inputBuffer[inCaesarPtr];
            inCaesarPtr = (inCaesarPtr+1) % localInSize;

        sem_post(&inBufferMutex);
        sem_post(&forceEncrypted);

        //locks so that the reset cannot happen while encryption
        pthread_mutex_lock(&reset_lock);

            //critical section to safely encrypt with no resets
            char y;
            y = caesar_encrypt(x);

        pthread_mutex_unlock(&reset_lock);


        
        sem_wait(&outBufferOpen);
        sem_wait(&outBufferMutex);

            //critical section for adding encrypted element to the output buffer
            outputBuffer[outCaesarPtr] = y;
            outCaesarPtr = (outCaesarPtr+1) % localOutSize;
          
        sem_post(&outBufferMutex);
        sem_post(&outBufferFull);
    }
}

/* This thread will count the occurrences of each encrypted
character from the output buffer */
void *output_counter_thread(void *n) {
    char x;
    int bufferInitCounter = 0;
    while(x != EOF){
        sem_wait(&outToCount);
        sem_wait(&outBufferMutex);

            //critical section for counting an element from the output buffer
            x = outputBuffer[outCountPtr];
            outCountPtr = (outCountPtr + 1) % outSize;

        sem_post(&outBufferMutex);
        sem_post(&outCount);

        //need to remove first N (Buffer size) counts 
        //because we need to init the writer semaphores
        if(bufferInitCounter++ == outSize & x != EOF)
            count_output(x);
        if(bufferInitCounter > outSize)
            bufferInitCounter = outSize;
    }
}

/* This thread will write each character from the output
buffer to a specified file */
void *writer_thread(void *n) {
    int localInSize = inSize;
    char x;
    while(x != EOF) 
    {


        sem_wait(&outBufferFull);
        sem_wait(&outCount);

            //critical section for writing to a file from the output buffer
            x = outputBuffer[outWriterPtr];
            outWriterPtr = (outWriterPtr+1) % localInSize;

        sem_post(&outToCount);
        sem_post(&outBufferOpen);
        
        write_output(x);  
    }

}

/* This functions will notify the rest of the program
that the encryptor has reset and that it is safe to 
continue */
void reset_finished(){
    //once reset is complete, unlock the encryptor
    pthread_mutex_unlock(&reset_lock);


}

/* This functions will block the rest of the program
to allow the encryptor to reset safely */
void reset_requested(){
    //signal lock to block the encryptor for safe reset
    pthread_mutex_lock(&reset_lock);

    printf("Reset requested.\n");
    printf("Total input count with current key is %d.\n", get_input_total_count());
    int i;
    for(i = 65; i < 91; i++) //ASCII
    {
        printf("%c:%d ", i, get_input_count(i));
    }
    printf("\nTotal output count with current key is %d.\n", get_output_total_count());
    for(i = 65; i < 91; i++) //ASCII
    {
        printf("%c:%d ", i, get_output_count(i));
    }
    printf("\nReset finished.\n\n");
}

int main(int argc, char *argv[]){
	char *inputFileName = argv[1];
	char *outputFileName = argv[2];
	init(inputFileName, outputFileName);
	int input, output;
	while(1){
		printf("Input size: ");
		scanf("%d", &input);
		if(input <= 1){
				printf("\nInput must be greater than 1\n");
				continue;
			}else{break;}
	}
	while(1){
		printf("Output size: ");
		scanf("%d", &output);
		if(output <= 1){
				printf("\nOutput must be greater than 1\n");
				continue;
			}else{break;}
	}

    //init buffers
    inSize = input;
    outSize = output;
	inputBuffer = malloc(inSize);
	outputBuffer = malloc(outSize);


    //set to allow them to pass
    sem_init(&inBufferMutex, 0, 1);
    sem_init(&outBufferOpen, 0, 1);

    //set to 0 because need to wait for other threads
    sem_init(&inBufferFull, 0, 0);
    sem_init(&forceNotEncrypted, 0, 0);
    sem_init(&outBufferFull, 0, 0);
    sem_init(&outCount, 0, 0);

    //set to outSize so outBuffer can fill up
    sem_init(&outToCount, 0, outSize);
    sem_init(&outBufferMutex, 0, outSize);

    //set to inSize so inBuffer can fill up
    sem_init(&inBufferOpen, 0, inSize);
    sem_init(&forceEncrypted, 0, inSize);

    //create threads
	pthread_t td[5];
	pthread_create(&td[0], NULL, reader_thread, NULL);
	pthread_create(&td[1], NULL, input_counter_thread, NULL);
    pthread_create(&td[2], NULL, encryption_thread, NULL);
    pthread_create(&td[3], NULL, output_counter_thread, NULL);
    pthread_create(&td[4], NULL, writer_thread, NULL);

    int t;
    for(t = 0; t < 5; t++)
    {
        pthread_join(td[t], NULL);
    }

    //destroy sems
    sem_destroy(&inBufferMutex);
    sem_destroy(&inBufferFull);
    sem_destroy(&forceNotEncrypted);
    sem_destroy(&inBufferOpen);
    sem_destroy(&forceEncrypted);
    sem_destroy(&outBufferMutex);
    sem_destroy(&outBufferFull);
    sem_destroy(&outBufferOpen);
    sem_destroy(&outCount);
    sem_destroy(&outToCount);

    //end of program
    printf("End of file reached.\n");
    printf("Total input count with current key is %d.\n", get_input_total_count());
    int i;
    for(i = 65; i < 91; i++) //ASCII
    {
        printf("%c:%d ", i, get_input_count(i));
    }
    printf("\nTotal output count with current key is %d.\n", get_output_total_count());
    for(i = 65; i < 91; i++) //ASCII
    {
        printf("%c:%d ", i, get_output_count(i));
    }

	return 0;
}
