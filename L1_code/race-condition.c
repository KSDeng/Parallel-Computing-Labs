/*******************************************************************
 * race-condition.c
 * Demonstrates a race condition.
 * Compile: gcc -pthread -o race race-condition.c
 * Run: ./race
 *******************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ADD_THREADS 10
#define SUB_THREADS 10

int global_counter;
int add_finished = 0;
pthread_mutex_t lock;
pthread_cond_t count_threshold;

void* add(void* threadid)
{
	pthread_mutex_lock(&lock);
    long tid = *(long*) threadid;
    global_counter++;
    sleep(rand() % 2);
    printf("add thread #%ld incremented global_counter! \n", tid);
	if (global_counter == ADD_THREADS) {
		printf("global_counter = %d, meet wait condition\n", global_counter);
		add_finished = 1;
		pthread_cond_broadcast(&count_threshold);
		//pthread_cond_signal(&count_threshold);
	}
	pthread_mutex_unlock(&lock);
	pthread_exit(NULL);
}

void* sub(void* threadid)
{
	pthread_mutex_lock(&lock);	
    long tid = *(long*) threadid;
	while (add_finished == 0) {
		printf("thread #%ld waiting\n", tid);
		pthread_cond_wait(&count_threshold, &lock);
		printf("thread #%ld receive signal\n", tid);
	}
    global_counter--;
    sleep(rand() % 2);
    printf("sub thread #%ld decremented global_counter! \n", tid);
	pthread_mutex_unlock(&lock);
	pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
    global_counter = 0;
	pthread_mutex_init(&lock, NULL);		// difference between this and PTHREAD_MUTEX_INITIALIZER?
	pthread_cond_init(&count_threshold, NULL);

    pthread_t add_threads[ADD_THREADS];
    pthread_t sub_threads[SUB_THREADS];
    long add_threadid[ADD_THREADS];
    long sub_threadid[SUB_THREADS];

    int rc;
    long t1, t2;
    for (t1 = 0; t1 < ADD_THREADS; t1++) {
        int tid = t1;
        add_threadid[tid] = tid;
        printf("main thread: creating add thread %d\n", tid);
        rc = pthread_create(&add_threads[tid], NULL, add,
                (void*) &add_threadid[tid]);
        if (rc) {
            printf("Return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for (t2 = 0; t2 < SUB_THREADS; t2++) {
        int tid = t2;
        sub_threadid[tid] = tid;
        printf("main thread: creating sub thread %d\n", tid);
        rc = pthread_create(&sub_threads[tid], NULL, sub,
                (void*) &sub_threadid[tid]);
        if (rc) {
            printf("Return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

	// wait for all the threads to terminate
	for (t1 = 0; t1 < ADD_THREADS; t1++) {
		pthread_join(add_threads[t1], NULL);
	}
	for (t2 = 0; t2 < SUB_THREADS; t2++) {
		pthread_join(sub_threads[t2], NULL);
	}

    printf("### global_counter final value = %d ###\n",
            global_counter);
    pthread_exit(NULL);
}
