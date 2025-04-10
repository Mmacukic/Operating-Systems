#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <execinfo.h>

#define NUM_PRIORITY_LEVELS 4

/* signal handlers declarations */
void proces_event(int sig);
void process_sigterm(int sig);
void process_sigint(int sig);

int run = 1;
int interrupt_occurred = 0; // Flag to track interrupt occurrence
int control_flag = 0; // Control flag
int current_priority = 0; // Current priority

// Function to simulate hardware interrupt
void simulate_interrupt() {
    srand(time(NULL));
    int interval = rand() % 5 + 1; // Random interval between 1 and 5 seconds
    sleep(interval);
    kill(getpid(), SIGUSR1); // Raise SIGUSR1 to simulate interrupt
}

void print_system_state(const char* state) {
    printf("System State: %s\n", state);
    printf("Control Flag: %d\n", control_flag);
    printf("Current Priority: %d\n", current_priority);
    // Print other relevant data structures here
}

void print_stack() {
    void *array[10];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);

    printf("Stack trace:\n");
    for (i = 0; i < size; i++)
        printf("%s\n", strings[i]);

    free(strings);
}

int main()
{
    struct sigaction act;

    /* 1. masking signal SIGUSR1 */
    /* signal handler function */
    act.sa_handler = proces_event;
    /* additionally block SIGTERM in handler function */
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGTERM);
    act.sa_flags = 0; /* advanced features not used */
    /* mask the signal SIGUSR1 as described above */
    sigaction(SIGUSR1, &act, NULL);

    /* 2. masking signal SIGTERM */
    act.sa_handler = process_sigterm;
    sigemptyset(&act.sa_mask);
    sigaction(SIGTERM, &act, NULL);

    /* 3. masking signal SIGINT */
    act.sa_handler = process_sigint;
    sigaction(SIGINT, &act, NULL);

    printf("Process with PID=%ld started\n", (long)getpid());

    /* processing simulation */
    int i = 1;
    while (run) {
        printf("Process: iteration %d\n", i++);
        simulate_interrupt(); // Simulate hardware interrupt
        if (interrupt_occurred) {
            print_system_state("Interrupt Handling");
            interrupt_occurred = 0; // Reset interrupt flag
        }
    }

    printf("Process with PID=%ld finished\n", (long)getpid());
    return 0;
}

void proces_event(int sig)
{
    int i;
    printf("Event processing started for signal %d (SIGUSR1)\n", sig);
    control_flag = 1;
    current_priority = NUM_PRIORITY_LEVELS - 1; // Set current priority to highest
    print_system_state("Handling Interrupt");
    for (i = 1; i <= 5; i++) {
        printf("Processing signal %d: %d/5\n", sig, i);
        sleep(1);
    }
    printf("Event processing completed for signal %d (SIGUSR1)\n", sig);
    interrupt_occurred = 1; // Set interrupt flag
    control_flag = 0; // Reset control flag
    current_priority = 0; // Reset current priority
}

void process_sigterm(int sig)
{
    printf("Received SIGTERM, saving data before exit\n");
    run = 0;
}

void process_sigint(int sig)
{
    printf("Received SIGINT, canceling process\n");
    exit(1);
}
