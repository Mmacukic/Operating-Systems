#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <ctype.h>

#define NUM_INPUT_THREADS 8
#define NUM_WORK_THREADS 6
#define NUM_OUTPUT_THREADS 3
#define BUFFER_SIZE 3
#define MAX_SLEEP_SECONDS 15

typedef struct {
    char buffer[BUFFER_SIZE];
    int input_index;
    int output_index;
    sem_t Bsem; // Single semaphore to control access to the buffer
} CircularBuffer;

CircularBuffer input_buffers[NUM_WORK_THREADS];
CircularBuffer output_buffers[NUM_OUTPUT_THREADS];

// Semaphores for input and output buffers
sem_t input_semaphores[NUM_WORK_THREADS];
sem_t output_semaphores[NUM_OUTPUT_THREADS];

void print_buffers() {
    printf("INBUF[]: ");
    for (int i = 0; i < NUM_WORK_THREADS; i++) {
        for (int j = 0; j < BUFFER_SIZE; j++) {
            if (input_buffers[i].buffer[j]) {
                printf("%c", input_buffers[i].buffer[j]);
            } else {
                printf("-");
            }
        }
        printf(" ");
    }
    printf("\n");
    printf("OUTBUF[]: ");
    for (int i = 0; i < NUM_OUTPUT_THREADS; i++) {
        for (int j = 0; j < BUFFER_SIZE; j++) {
            if (output_buffers[i].buffer[j]) {
                printf("%c", output_buffers[i].buffer[j]);
            } else {
                printf("-");
            }
        }
        printf(" ");
    }
    printf("\n");
}

void *input_thread(void *arg) {
    int thread_id = *((int *)arg);
    while (1) {
        char data = 'A' + rand() % 26; // Simulating input
        int index = rand() % NUM_WORK_THREADS;
        sem_wait(&input_buffers[index].Bsem);
        input_buffers[index].buffer[input_buffers[index].input_index] = data;
        input_buffers[index].input_index = (input_buffers[index].input_index + 1) % BUFFER_SIZE;
        sem_post(&input_buffers[index].Bsem);
        printf("U%d: get_input(%d)=>'%c'; process_input('%c')=>%d; '%c' => INBUF[%d]\n", thread_id, thread_id, data, data, index, data, index);
        print_buffers(); // Print buffers after each input
        sleep(rand() % MAX_SLEEP_SECONDS);
    }
    return NULL;
}
void *work_thread(void *arg) {
    int thread_id = *((int *)arg);
    while (1) {
        for (int i = 0; i < NUM_INPUT_THREADS; i++) {
            // Choose an output buffer index randomly
            int output_index = rand() % NUM_OUTPUT_THREADS;

            // Choose an input buffer index randomly
            int input_buffer_index = rand() % NUM_WORK_THREADS;

            // Wait on the semaphore for the chosen input buffer
            sem_wait(&input_buffers[input_buffer_index].Bsem);

            // Access the input buffer
            char data = input_buffers[input_buffer_index].buffer[input_buffers[input_buffer_index].output_index];

            // Check if the data is alphabetic
            if (isalpha(data)) {
                // Convert uppercase to lowercase
                if (data >= 'A' && data <= 'Z') {
                    data += 32;
                }

                // Simulate processing
                sleep(rand() % MAX_SLEEP_SECONDS);

                // Update input buffer
                input_buffers[input_buffer_index].buffer[input_buffers[input_buffer_index].output_index] = '\0';
                input_buffers[input_buffer_index].output_index = (input_buffers[input_buffer_index].output_index + 1) % BUFFER_SIZE;

                // Release the semaphore for the input buffer
                sem_post(&input_buffers[input_buffer_index].Bsem);

                // Wait on the semaphore for the chosen output buffer
                sem_wait(&output_buffers[output_index].Bsem);

                // Update the output buffer
                output_buffers[output_index].buffer[output_buffers[output_index].input_index] = data;
                output_buffers[output_index].input_index = (output_buffers[output_index].input_index + 1) % BUFFER_SIZE;

                // Print processing information
                printf("R%d: taking from INBUF[%d] => '%c' and processing\n", thread_id, input_buffer_index, data);
                print_buffers(); // Print buffers after each processing

                // Release the semaphore for the output buffer
                sem_post(&output_buffers[output_index].Bsem);
            } else {
                // Release the semaphore for the input buffer
                sem_post(&input_buffers[input_buffer_index].Bsem);

                // Release the semaphore for the output buffer
                //sem_post(&output_buffers[output_index].Bsem);
            }
        }
    }
    return NULL;
}

void *output_thread(void *arg) {
    int thread_id = *((int *)arg);
    while (1) {
        sem_wait(&output_buffers[thread_id].Bsem);
        char data = output_buffers[thread_id].buffer[output_buffers[thread_id].output_index];
        if (isalpha(data)) {
            output_buffers[thread_id].buffer[output_buffers[thread_id].output_index] = '\0';
            output_buffers[thread_id].output_index = (output_buffers[thread_id].output_index + 1) % BUFFER_SIZE;
            sem_post(&output_buffers[thread_id].Bsem);
            printf("O%d: output from OUBUF[%d]=>'%c'printing %c\n", thread_id, thread_id, data, data);
            print_buffers();
            sleep(rand() % MAX_SLEEP_SECONDS);
        } else {
            sem_post(&output_buffers[thread_id].Bsem);
        }
    }
    return NULL;
}

int main() {
    pthread_t input_threads[NUM_INPUT_THREADS];
    pthread_t work_threads[NUM_WORK_THREADS];
    pthread_t output_threads[NUM_OUTPUT_THREADS];
    int input_thread_ids[NUM_INPUT_THREADS];
    int work_thread_ids[NUM_WORK_THREADS];
    int output_thread_ids[NUM_OUTPUT_THREADS];

    // Initialize circular buffers and semaphores
    for (int i = 0; i < NUM_WORK_THREADS; i++) {
        sem_init(&input_buffers[i].Bsem, 0, 1); // Initialize Bsem to 1
        input_buffers[i].input_index = 0;
        input_buffers[i].output_index = 0;
        sem_init(&input_semaphores[i], 0, 1); // Initialize semaphore for input buffer
    }
    for (int i = 0; i < NUM_OUTPUT_THREADS; i++) {
        sem_init(&output_buffers[i].Bsem, 0, 1); // Initialize Bsem to 1
        output_buffers[i].input_index = 0;
        output_buffers[i].output_index = 0;
        sem_init(&output_semaphores[i], 0, 1); // Initialize semaphore for output buffer
    }

    // Create input threads
    for (int i = 0; i < NUM_INPUT_THREADS; i++) {
        input_thread_ids[i] = i;
        pthread_create(&input_threads[i], NULL, input_thread, &input_thread_ids[i]);
    }
    sleep(4);

    // Create work threads
    for (int i = 0; i < NUM_WORK_THREADS; i++) {
        work_thread_ids[i] = i;
        pthread_create(&work_threads[i], NULL, work_thread, &work_thread_ids[i]);
        sleep(1);
    }

    // Create output threads
    for (int i = 0; i < NUM_OUTPUT_THREADS; i++) {
        output_thread_ids[i] = i;
        pthread_create(&output_threads[i], NULL, output_thread, &output_thread_ids[i]);
    }

    // Join threads
    for (int i = 0; i < NUM_INPUT_THREADS; i++) {
        pthread_join(input_threads[i], NULL);
    }
    for (int i = 0; i < NUM_WORK_THREADS; i++) {
        pthread_join(work_threads[i], NULL);
    }
    for (int i = 0; i < NUM_OUTPUT_THREADS; i++) {
        pthread_join(output_threads[i], NULL);
    }

    // Cleanup
    for (int i = 0; i < NUM_WORK_THREADS; i++) {
        sem_destroy(&input_buffers[i].Bsem);
        sem_destroy(&input_semaphores[i]); // Cleanup semaphore for input buffer
    }
    for (int i = 0; i < NUM_OUTPUT_THREADS; i++) {
        sem_destroy(&output_buffers[i].Bsem);
        sem_destroy(&output_semaphores[i]); // Cleanup semaphore for output buffer
    }

    return 0;
}
