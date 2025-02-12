#ifndef MULTILOOKUP_H
#define MULTILOOKUP_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "array.h"

#define ARRAY_SIZE 8                // Number of elements in shared array used by requester/resolver threads
#define MAX_INPUT_FILES 100         // Max number of hostname file arguments allowed
#define MAX_REQUESTER_THREADS 10    // Max number of concurrent requester threads
#define MAX_RESOLVER_THREADS 10     // Max number of concurrent resolver threads
#define MAX_NAME_LENGTH 17          // Max size of hostname including null terminator
#define MAX_IP_LENGTH INET6_ADDRSTRLEN // Maximum size IP address string util.c will return
#define POISON_PILL "POISON PILL"

typedef struct {
    int num_files;
    int files_serviced;
    int thread_specific_files_serviced;
    int files_looked_at;
    array* sharedArray;
    char** input_log_file;
    FILE* output_log_file;
    pthread_mutex_t* mutex_write_logfile_req;
    pthread_mutex_t* mutex_count;
} requester_thread_struct;

typedef struct {
    int num_files;
    int hostnames_resolved;
    int thread_specific_files_serviced;
    array* sharedArray;
    FILE* output_log_file;
    pthread_mutex_t* mutex_write_logfile_res;
} resolver_thread_struct;

typedef struct {
    int requester_check;
} sharedPoisonPill;

void *requester_threads(void *args);
void *resolver_threads(void *args);
int main(int argc, char* argv[]);

#endif // MULTILOOKUP_H
