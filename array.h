#ifndef ARRAY_H
#define ARRAY_H

#include <pthread.h>
#include <semaphore.h>

#define ARRAY_SIZE 8                     // number of elements in shared array used by requester/resolver threads
#define MAX_INPUT_FILES 100              // max number of hostname file arguments allowed
#define MAX_REQUESTER_THREADS 10         // max number of concurrent requester threads
#define MAX_RESOLVER_THREADS 10          // max number of concurrent resolver threads
#define MAX_NAME_LENGTH 17               // max size of hostname including null terminator
#define MAX_IP_LENGTH INET6_ADDRSTRLEN   // maximum size IP address string util.c will return

typedef struct {
    char **array;
    int count;                         // count of elements in array
    int head;
    int tail;
    sem_t sem_empty;
    sem_t sem_full;
    sem_t mutex;
} array;

int array_init(array *s);               // initialize the array
int array_put(array *s, char *hostname); // place element into the array, block when full
int array_get(array *s, char **hostname); // remove element from the array, block when empty
void array_free(array *s);              // free the array's resources

#endif
