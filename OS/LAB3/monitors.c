#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_READERS 12
#define MAX_WRITERS 4
#define MAX_ERASERS 2

// Linked List Node
typedef struct Node {
    int data;
    struct Node* next;
} Node;

// Linked List
typedef struct {
    Node* head;
    Node* tail;
} LinkedList;

// Monitor for synchronization
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t can_read;
    pthread_cond_t can_write;
    pthread_cond_t can_erase;
    int readers;
    int writers;
    int erasers;
    LinkedList* list; // Add pointer to shared linked list
} ListMonitor;


// Initialize linked list
void init_list(LinkedList* list) {
    list->head = NULL;
    list->tail = NULL;
}

// Initialize monitor
void init_monitor(ListMonitor* monitor, LinkedList* list) {
    pthread_mutex_init(&monitor->mutex, NULL);
    pthread_cond_init(&monitor->can_read, NULL);
    pthread_cond_init(&monitor->can_write, NULL);
    pthread_cond_init(&monitor->can_erase, NULL);
    monitor->readers = 0;
    monitor->writers = 0;
    monitor->erasers = 0;
    monitor->list = list; // Assign the shared linked list
}

// Enter monitor for reading
void enter_read(ListMonitor* monitor) {
    pthread_mutex_lock(&monitor->mutex);
    while (monitor->erasers > 0) {
        pthread_cond_wait(&monitor->can_read, &monitor->mutex);
    }
    monitor->readers++;
    pthread_mutex_unlock(&monitor->mutex);
}

// Exit monitor after reading
void exit_read(ListMonitor* monitor) {
    pthread_mutex_lock(&monitor->mutex);
    monitor->readers--;
    pthread_cond_signal(&monitor->can_write);
    
    pthread_mutex_unlock(&monitor->mutex);
}

// Enter monitor for writing
void enter_write(ListMonitor* monitor) {
    pthread_mutex_lock(&monitor->mutex);
    while (monitor->writers > 0 || monitor->erasers > 0) {
        pthread_cond_wait(&monitor->can_write, &monitor->mutex);
    }
    monitor->writers++;
    pthread_mutex_unlock(&monitor->mutex);
}

// Exit monitor after writing
void exit_write(ListMonitor* monitor) {
    pthread_mutex_lock(&monitor->mutex);
    monitor->writers--;
    pthread_cond_broadcast(&monitor->can_read);
    pthread_cond_broadcast(&monitor->can_write);
    pthread_cond_broadcast(&monitor->can_erase);
    pthread_mutex_unlock(&monitor->mutex);
}

// Enter monitor for erasing
void enter_erase(ListMonitor* monitor) {
    pthread_mutex_lock(&monitor->mutex);
    while (monitor->readers > 0 || monitor->writers > 0 || monitor->erasers > 0) {
        pthread_cond_wait(&monitor->can_erase, &monitor->mutex);
    }
    monitor->erasers++;
    pthread_mutex_unlock(&monitor->mutex);
}

// Exit monitor after erasing
void exit_erase(ListMonitor* monitor) {
    pthread_mutex_lock(&monitor->mutex);
    monitor->erasers--;
    pthread_cond_broadcast(&monitor->can_read);
    pthread_cond_broadcast(&monitor->can_write);
    pthread_mutex_unlock(&monitor->mutex);
}

// Append data to the linked list
void append(LinkedList* list, int data) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->data = data;
    new_node->next = NULL;

    if (list->tail == NULL) {
        list->head = new_node;
        list->tail = new_node;
    } else {
        list->tail->next = new_node;
        list->tail = new_node;
    }
}

// Print the linked list
void print_list(LinkedList* list) {
    printf("List: ");
    Node* current = list->head;
    while (current != NULL) {
        printf("%d ", current->data);
        current = current->next;
    }
    printf("\n");
}
int choose_random_value(LinkedList* list) {
    if (list->head == NULL) {
        return -1; // List is empty
    }

    // Count the number of elements
    int count = 0;
    Node* current = list->head;
    while (current != NULL) {
        count++;
        current = current->next;
    }

    // Generate a random index
    int random_index = rand() % count;

    // Traverse the list to the chosen index
    current = list->head;
    for (int i = 0; i < random_index; i++) {
        current = current->next;
    }

    // Return the value at the chosen index
    return current->data;
}

// Remove a random element from the linked list
int remove_random(LinkedList* list) {
    if (list->head == NULL) {
        return -1;
    }

    Node* current = list->head;
    Node* previous = NULL;
    int random_index = rand() % (MAX_READERS + 1);
    for (int i = 0; i < random_index && current != NULL; i++) {
        previous = current;
        current = current->next;
    }
    int deleted_value = -1;
    if (current != NULL) {
        deleted_value = current->data;
        if (previous != NULL) {
            previous->next = current->next;
            if (current == list->tail) {
                list->tail = previous;
            }
            free(current);
        } else {
            list->head = current->next;
            if (current == list->tail) {
                list->tail = NULL;
            }
            free(current);
        }
    }
    return deleted_value;
}


// Reader thread function
void* reader(void* arg) {
    ListMonitor* monitor = (ListMonitor*)arg;
    LinkedList* list = monitor->list; // Get shared list
    while (1) {
        enter_read(monitor);

        printf("Reader %lu starts using the list\n", pthread_self());
        printf("Active: readers=%d, writers=%d, erasers=%d\n", monitor->readers, monitor->writers, monitor->erasers);

        // Simulate reading from the list
        int value = choose_random_value(list);
        sleep(4);

        printf("Reader %lu reads value %d from the list\n", pthread_self(), value);
        printf("Reader %lu stops using the list\n", pthread_self());
        printf("Active: readers=%d, writers=%d, erasers=%d\n", monitor->readers, monitor->writers, monitor->erasers);
        print_list(list);
        exit_read(monitor);

        sleep(rand()%5 + 5);
    }
    return NULL;
}

// Writer thread function
void* writer(void* arg) {
    ListMonitor* monitor = (ListMonitor*)arg;
    LinkedList* list = monitor->list; // Get shared list
    while (1) {
        enter_write(monitor);

        printf("Writer %lu starts using the list\n", pthread_self());
        printf("Active: readers=%d, writers=%d, erasers=%d\n", monitor->readers, monitor->writers, monitor->erasers);

        // Simulate writing to the list
        int data = rand() % 100;
        printf("Writer %lu adds %d to the end of the list\n", pthread_self(), data);
        append(list, data);
        print_list(list);

        printf("Writer %lu stops using the list\n", pthread_self());
        printf("Active: readers=%d, writers=%d, erasers=%d\n", monitor->readers, monitor->writers, monitor->erasers);

        exit_write(monitor);

        sleep(rand()%5 + 5);
    }
    return NULL;
}

// Eraser thread function
void* eraser(void* arg) {
    ListMonitor* monitor = (ListMonitor*)arg;
    LinkedList* list = monitor->list; // Get shared list
    while (1) {
        enter_erase(monitor);

        printf("Eraser %lu starts using the list\n", pthread_self());
        printf("Active: readers=%d, writers=%d, erasers=%d\n", monitor->readers, monitor->writers, monitor->erasers);
        print_list(list);

        // Simulate erasing from the list
        int deleted = remove_random(list);


        printf("Eraser %lu stops using the list, deleted: %d\n", pthread_self(), deleted);
        printf("Active: readers=%d, writers=%d, erasers=%d\n", monitor->readers, monitor->writers, monitor->erasers);
        print_list(list);
        exit_erase(monitor);

        sleep(rand()%5 + 5);
    }
    return NULL;
}

int main() {
    srand(time(NULL));
    LinkedList list;
    init_list(&list);

    ListMonitor monitor;
    init_monitor(&monitor, &list);

    pthread_t readers[MAX_READERS];
    pthread_t writers[MAX_WRITERS];
    pthread_t erasers[MAX_ERASERS];

    // Create writer threads
    for (int i = 0; i < MAX_WRITERS; i++) {
        pthread_create(&writers[i], NULL, writer, (void*)&monitor);
    }

    // Create reader threads
    for (int i = 0; i < MAX_READERS; i++) {
        pthread_create(&readers[i], NULL, reader, (void*)&monitor);
    }

    // Create eraser threads
    for (int i = 0; i < MAX_ERASERS; i++) {
        pthread_create(&erasers[i], NULL, eraser, (void*)&monitor);
    }

    // Wait for threads to finish
    for (int i = 0; i < MAX_WRITERS; i++) {
        pthread_join(writers[i], NULL);
    }
    for (int i = 0; i < MAX_READERS; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < MAX_ERASERS; i++) {
        pthread_join(erasers[i], NULL);
    }

    return 0;
}