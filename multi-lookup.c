#include "multi-lookup.h"
#include "array.h"
#include "util.h"
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

void *requester_threads(void *args) {
    requester_thread_struct *actual_args = (requester_thread_struct *)args;
    FILE *output_file = actual_args->output_log_file;
    char hostname[MAX_NAME_LENGTH];
    actual_args->thread_specific_files_serviced = 0;

    while (1) {
        pthread_mutex_lock(actual_args->mutex_count);
        if (actual_args->files_looked_at == actual_args->num_files) {
            pthread_mutex_unlock(actual_args->mutex_count);
            pthread_t tid = pthread_self();
            fprintf(stdout, "thread %lu serviced %d files\n", (unsigned long)tid, actual_args->thread_specific_files_serviced);
            break;
        }

        FILE *input_file = fopen(actual_args->input_log_file[actual_args->files_looked_at], "r");
        if (input_file == NULL) {
            fprintf(stderr, "invalid file %s\n", actual_args->input_log_file[actual_args->files_looked_at]);
            actual_args->files_looked_at++;
            pthread_mutex_unlock(actual_args->mutex_count);
            continue;
        }

        actual_args->files_serviced++;
        actual_args->files_looked_at++;
        actual_args->thread_specific_files_serviced++;
        pthread_mutex_unlock(actual_args->mutex_count);

        while (fgets(hostname, sizeof(hostname), input_file) != NULL) {
            if (strcmp(hostname, "\n") == 0) {
                continue;
            }
            hostname[strlen(hostname) - 1] = '\0';
            array_put(actual_args->sharedArray, hostname);
            pthread_mutex_lock(actual_args->mutex_write_logfile_req);
            fprintf(output_file, "%s\n", hostname);
            fflush(output_file);
            pthread_mutex_unlock(actual_args->mutex_write_logfile_req);
        }
        fclose(input_file);
    }
    return NULL;
}

void *resolver_threads(void *args) {
    resolver_thread_struct *actual_args = (resolver_thread_struct *)args;
    FILE *output_file = actual_args->output_log_file;
    char *hostname = malloc(MAX_NAME_LENGTH * sizeof(char));
    char IPstrtopass[MAX_IP_LENGTH];
    actual_args->thread_specific_files_serviced = 0;

    while (1) {
        array_get(actual_args->sharedArray, &hostname);
        if (strcmp(hostname, POISON_PILL) == 0) {
            break;
        }
        if (dnslookup(hostname, IPstrtopass, MAX_IP_LENGTH) == UTIL_SUCCESS) {
            pthread_mutex_lock(actual_args->mutex_write_logfile_res);
            fprintf(output_file, "%s, %s\n", hostname, IPstrtopass);
            fflush(output_file);
            pthread_mutex_unlock(actual_args->mutex_write_logfile_res);
        } else {
            pthread_mutex_lock(actual_args->mutex_write_logfile_res);
            fprintf(output_file, "%s, NOT_RESOLVED\n", hostname);
            fflush(output_file);
            pthread_mutex_unlock(actual_args->mutex_write_logfile_res);
        }
        actual_args->thread_specific_files_serviced++;
    }

    pthread_t tid = pthread_self();
    fprintf(stdout, "thread %lu resolved %d hostnames\n", (unsigned long)tid, actual_args->thread_specific_files_serviced);
    free(hostname);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 6) {
        fprintf(stdout, "Too few arguments provided.\n");
        return EXIT_FAILURE;
    }
    if (argc > 105) {
        fprintf(stdout, "Too many arguments provided.\n");
        return EXIT_FAILURE;
    }

    struct timeval start_time, end_time;
    array reqResArray;
    if (array_init(&reqResArray) != 0) {
        printf("Failed to init shared array\n");
    }

    int num_requesters = atoi(argv[1]);
    int num_resolvers = atoi(argv[2]);
    if (num_requesters > MAX_REQUESTER_THREADS) {
        fprintf(stderr, "Too many requester threads.\n");
        return EXIT_FAILURE;
    }
    if (num_resolvers > MAX_RESOLVER_THREADS) {
        fprintf(stderr, "Too many resolver threads.\n");
        return EXIT_FAILURE;
    }

    int num_files = argc - 5;
    pthread_t requesterThreads[num_requesters];
    pthread_t resolverThreads[num_resolvers];
    requester_thread_struct req_args;
    resolver_thread_struct res_args;
    pthread_mutex_t mutex_write_logfile_req = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex_write_logfile_res = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex_count = PTHREAD_MUTEX_INITIALIZER;

    req_args.input_log_file = &argv[5];
    req_args.sharedArray = &reqResArray;
    req_args.output_log_file = fopen(argv[3], "w");
    req_args.num_files = num_files;
    req_args.files_serviced = 0;
    req_args.thread_specific_files_serviced = 0;
    req_args.files_looked_at = 0;
    req_args.mutex_write_logfile_req = &mutex_write_logfile_req;
    req_args.mutex_count = &mutex_count;

    res_args.output_log_file = fopen(argv[4], "w");
    res_args.sharedArray = &reqResArray;
    res_args.num_files = num_files;
    res_args.hostnames_resolved = 0;
    res_args.thread_specific_files_serviced = 0;
    res_args.mutex_write_logfile_res = &mutex_write_logfile_res;

    gettimeofday(&start_time, NULL);

    for (int i = 0; i < num_requesters; i++) {
        if (pthread_create(&requesterThreads[i], NULL, requester_threads, &req_args)) {
            fprintf(stderr, "Error during index %d\n", i);
            return 1;
        }
    }
    for (int i = 0; i < num_resolvers; i++) {
        if (pthread_create(&resolverThreads[i], NULL, resolver_threads, &res_args)) {
            fprintf(stderr, "Error during index %d\n", i);
            return 1;
        }
    }

    for (int i = 0; i < num_requesters; i++) {
        pthread_join(requesterThreads[i], NULL);
    }

    for (int i = 0; i < num_resolvers; i++) {
        array_put(&reqResArray, POISON_PILL);
    }

    for (int i = 0; i < num_resolvers; i++) {
        pthread_join(resolverThreads[i], NULL);
    }

    gettimeofday(&end_time, NULL);
    pthread_mutex_destroy(req_args.mutex_write_logfile_req);
    pthread_mutex_destroy(res_args.mutex_write_logfile_res);
    pthread_mutex_destroy(req_args.mutex_count);
    fclose(res_args.output_log_file);
    fclose(req_args.output_log_file);
    array_free(&reqResArray);

    float duration = (end_time.tv_sec - start_time.tv_sec) + 1e-6 * (end_time.tv_usec - start_time.tv_usec);
    printf("./multi-lookup: total time is %f seconds\n", duration);
    return 0;
}
