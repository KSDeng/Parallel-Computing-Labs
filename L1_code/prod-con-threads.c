/*******************************************************************
 * prod-con-threads.c
 * Producer-consumer synchronisation problem in C
 * Compile: gcc -pthread -o prodcont prod-con-threads.c
 * Run: ./prodcont
 *******************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define PRODUCERS 2
#define CONSUMERS 1
#define BUFFER_SIZE 10
#define RANDOM_MIN 1
#define RANDOM_MAX 10
#define EMPTY_FLAG -1

int consumed_sum = 0;
int buffer[BUFFER_SIZE];
int pos_prod = 0, pos_con = 0, len_buf = 0;

pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;	// mutex lock for buffer
pthread_cond_t consumer_cv = PTHREAD_COND_INITIALIZER;	// consumer condition value
pthread_cond_t producer_cv = PTHREAD_COND_INITIALIZER;	// producer condition value

void printBufferStatus(int pos_hl) {
	int j;
	for (j = 0; j < BUFFER_SIZE; j++) {
		if (j == pos_hl)
			printf(" *%d ", buffer[j]);
		else {
			if (buffer[j] == EMPTY_FLAG) printf(" __ ");
			else printf(" %d ", buffer[j]);
		}
	}
}

void* producer(void* threadid) {
	int tid = *(int*)threadid;
	while (1) {
		pthread_mutex_lock(&buffer_lock);

		if (len_buf > BUFFER_SIZE) exit(1);	// should never reach here!
		while (len_buf == BUFFER_SIZE) {
			printf("[producer #%d] buffer full, waiting...\n", tid);
			pthread_cond_wait(&producer_cv, &buffer_lock);
		}
		srand(time(NULL));
		int random_num = rand() % RANDOM_MAX + RANDOM_MIN;
		buffer[pos_prod] = random_num;
		int temp_pos = pos_prod;
		pos_prod = (pos_prod + 1) % BUFFER_SIZE;
		len_buf++;
		// visualize
		printf("[producer #%d] insert %d, current buffer: ", tid, random_num);
		printBufferStatus(temp_pos);
		printf("\n"); 
		sleep(rand() % 2);

		pthread_mutex_unlock(&buffer_lock);
		pthread_cond_signal(&consumer_cv);
	}
}

void* consumer(void* threadid) {
	int tid = *(int*)threadid;
	int element;
	while (1) {
		pthread_mutex_lock(&buffer_lock);

		if (len_buf < 0) exit(1);	// should never reach here!
		while (len_buf == 0) {
			printf("[consumer #%d] buffer empty, waiting...\n", tid);
			pthread_cond_wait(&consumer_cv, &buffer_lock);	
		}
		// visualize
		printf("[consumer #%d] current buffer: ", tid);
		printBufferStatus(pos_con);
		element = buffer[pos_con];
		buffer[pos_con] = EMPTY_FLAG;
		pos_con = (pos_con + 1) % BUFFER_SIZE;
		len_buf--;
		consumed_sum += element;
		printf(", consume %d, sum = %d\n", element, consumed_sum); 
		sleep(rand() % 2);

		pthread_mutex_unlock(&buffer_lock);
		pthread_cond_signal(&producer_cv);
	}
}

int main(int argc, char* argv[]) {
    pthread_t producer_threads[PRODUCERS];
    pthread_t consumer_threads[CONSUMERS];
    int producer_threadid[PRODUCERS];
    int consumer_threadid[CONSUMERS];

	int i;
	for (i = 0; i < BUFFER_SIZE; ++i) buffer[i] = EMPTY_FLAG;

    int rc;
    int t1, t2;
    for (t1 = 0; t1 < PRODUCERS; t1++) {
        int tid = t1;
        producer_threadid[tid] = tid;
        printf("Main: creating producer %d\n", tid);
        rc = pthread_create(&producer_threads[tid], NULL, producer,
                (void*) &t1);
        if (rc) {
            printf("Error: Return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for (t2 = 0; t2 < CONSUMERS; t2++) {
        int tid = t2;
        consumer_threadid[tid] = tid;
        printf("Main: creating consumer %d\n", tid);
        rc = pthread_create(&consumer_threads[tid], NULL, consumer,
                (void*) &t2);
        if (rc) {
            printf("Error: Return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

	for (t1 = 0; t1 < PRODUCERS; t1++) pthread_join(producer_threads[t1], NULL);
	for (t2 = 0; t2 < CONSUMERS; t2++) pthread_join(consumer_threads[t2], NULL);

    pthread_exit(NULL);
}
