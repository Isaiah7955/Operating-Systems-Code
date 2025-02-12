#include "array.h"
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int array_init(array *s) { // Initialize the stack
    s->head = 0;
    s->tail = 0;
    s->count = 0;
    
    s->array = (char **)malloc(ARRAY_SIZE * sizeof(char *)); // Allocate memory
    if (s->array == NULL) {
        printf("Error: Memory allocation for s->array failed\n");
        return -1;
    }
    
    for (int i = 0; i < ARRAY_SIZE; i++) {
        s->array[i] = (char *)malloc(MAX_NAME_LENGTH * sizeof(char));
        if (s->array[i] == NULL) {
            printf("Error: Memory allocation for s->array[%d] failed\n", i);
            return -1;
        }
    }
    
    sem_init(&s->sem_empty, 0, ARRAY_SIZE); // All slots available at init
    sem_init(&s->sem_full, 0, 0);          // No items to pull out at init
    sem_init(&s->mutex, 0, 1);             // Mutex starts as available
    
    return 0;
}

int array_put(array *s, char *hostname) { // Producer - place element on the stack
    sem_wait(&s->sem_empty);
    sem_wait(&s->mutex);
    
    strncpy(s->array[s->tail], hostname, MAX_NAME_LENGTH);
    s->count++;
    s->tail = (s->tail + 1) % ARRAY_SIZE;
    
    sem_post(&s->mutex);
    sem_post(&s->sem_full);
    
    return 0;
}

int array_get(array *s, char **hostname) { // Consumer - remove element from the stack
    sem_wait(&s->sem_full);
    sem_wait(&s->mutex);
    
    s->count--;
    strncpy(*hostname, s->array[s->head], MAX_NAME_LENGTH);
    s->head = (s->head + 1) % ARRAY_SIZE;
    
    sem_post(&s->mutex);
    sem_post(&s->sem_empty);
    
    return 0;
}

void array_free(array *s) { // Free the stack's resources
    for (int i = 0; i < ARRAY_SIZE; i++) {
        free(s->array[i]);
    }
    free(s->array);
    
    sem_destroy(&s->sem_empty);
    sem_destroy(&s->sem_full);
    sem_destroy(&s->mutex);
}
