#include <stdio.h>
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define N_CONSUMER 1
#define N_PRODUCER 2
#define BUFFER_SIZE 10
#define EMPTY_FLAG -1
#define RANDOM_MAX 10
#define RANDOM_MIN 1

struct shm_data {
    int buffer[BUFFER_SIZE];
    sem_t buffer_sem;   // used as lock for shared memory
    sem_t used_sem;     // buffer used space
    sem_t free_sem;     // buffer free space

    int consumed_sum;
    int pos_prod;
    int pos_con;
};

struct shm_data* p;     // shared variable

void printBufferStatus(int pos_hl) {
    int j;
    for (j = 0; j < BUFFER_SIZE; j++) {
        if (j == pos_hl)
            printf(" *%d ", p->buffer[j]);
        else {
            if (p->buffer[j] == EMPTY_FLAG) printf(" __ ");
            else printf(" %d ", p->buffer[j]);
        }
    }
}

void producer(int producer_id) {
    while (1) {
        sem_wait(&(p->free_sem));       // apply for a free space
        sem_wait(&(p->buffer_sem));     // apply for shared memory lock

        // produce
        srand(time(NULL));
        int random_num = rand() % RANDOM_MAX + RANDOM_MIN;
        p->buffer[p->pos_prod] = random_num;
        int temp_pos = p->pos_prod;
        p->pos_prod = (p->pos_prod + 1) % BUFFER_SIZE;

        // visualize
        printf("[producer #%d] insert %d, current buffer: ", producer_id, random_num);
        printBufferStatus(temp_pos);
        printf("\n");
        sleep(rand() % 2);

        sem_post(&(p->buffer_sem));     // release the lock for shared memory
        sem_post(&(p->used_sem));       // signal for a new production
    }
}

void consumer(int consumer_id) {
    int element;
    while (1) {
        sem_wait(&(p->used_sem));   // apply for a production
        sem_wait(&(p->buffer_sem)); // apply for the shared memory lock

        // visualize
        printf("[consumer #%d] current buffer: ", consumer_id);
        printBufferStatus(p->pos_con);

        // consume
        element = p->buffer[p->pos_con];
        p->buffer[p->pos_con] = EMPTY_FLAG;
        p->pos_con = (p->pos_con + 1) % BUFFER_SIZE;
        p->consumed_sum += element;
        printf(", consume %d, sum = %d\n", element, p->consumed_sum);
        sleep(rand() % 2);

        sem_post(&(p->buffer_sem));     // release the lock for the shared memory
        sem_post(&(p->free_sem));       // signal for a new free space
    }
}

int main() {
    int i;          // loop variable
    key_t shmkey;   // shared memory key
    int shmid;      // shared memory id
    pid_t pid;      // fork pid

    shmkey = ftok("/dev/null", 5); // valid directory name and a number
    shmid = shmget(shmkey, sizeof(struct shm_data), 0644 | IPC_CREAT);      // allocate enough space for our struct (data + sem)
    if (shmid < 0) {    // error check
        perror("shmget\n");
        exit(1);
    }

    // initialize content of shared memory
    p = (struct shm_data*)shmat(shmid, NULL, 0); // attach p to shared memory
    for (i = 0; i < BUFFER_SIZE; i++) p->buffer[i] = EMPTY_FLAG;

    sem_init(&p->buffer_sem, 1, 1);
    sem_init(&p->used_sem, 1, 0);
    sem_init(&p->free_sem, 1, BUFFER_SIZE);
    p->consumed_sum = 0;
    p->pos_prod = 0;
    p->pos_con = 0;

    // fork consumer processes
    for (i = 0; i < N_CONSUMER; i++) {
        pid = fork();
        if (pid < 0) {
            // check for error
            printf("Fork error.\n");
            exit(1);
        } else if (pid == 0) {
            printf("consumer %d created\n", i);
            consumer(i);
        }
    }

    // fork producer processes
    for (i = 0; i < N_PRODUCER; i++) {
        pid = fork();
        if (pid < 0) {
            // check for error
            printf("Fork error.\n");
            exit(1);
        } else if (pid == 0) {
            printf("producer %d created\n", i);
            producer(i);
        }
    }

    // parent process
    if (pid != 0) {
        // wait for all children to exit
        while (waitpid(-1, NULL, 0)) {
            if (errno == ECHILD)
                break;
        }

        // shared memory detach
        shmdt(p);
        shmctl(shmid, IPC_RMID, 0);

        // cleanup semaphores
        sem_destroy(&p->buffer_sem);
        sem_destroy(&p->used_sem);
        sem_destroy(&p->free_sem);
        exit(0);
    }
}
