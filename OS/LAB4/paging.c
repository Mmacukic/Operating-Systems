#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE 64
#define NUM_PAGES 256
#define NUM_FRAMES 4

typedef struct {
    int frame_number;
    int valid;
} PageTableEntry;

typedef struct {
    PageTableEntry entries[NUM_PAGES];
} PageTable;

typedef struct {
    char data[PAGE_SIZE];
} Frame;

typedef struct {
    Frame frames[NUM_FRAMES];
} Memory;

typedef struct {
    PageTable page_table;
    Memory memory;
} OnDemandPagingSimulator;

void access_memory(OnDemandPagingSimulator *simulator, int logical_address) {
    int page_number = logical_address / PAGE_SIZE;
    int offset = logical_address % PAGE_SIZE;

    int frame_number = simulator->page_table.entries[page_number].frame_number;
    if (!simulator->page_table.entries[page_number].valid) {
        printf("Page fault occurred for page: %d\n", page_number);
        // Simulate loading page from disk
        for (int i = 0; i < NUM_FRAMES; i++) {
            if (!simulator->page_table.entries[i].valid) {
                frame_number = i;
                simulator->page_table.entries[page_number].frame_number = i;
                simulator->page_table.entries[page_number].valid = 1;
                printf("Page table entry %d is invalid, loading page %d into frame %d\n", page_number, page_number, frame_number);
                break;
            }
        }
    }

    int physical_address = frame_number * PAGE_SIZE + offset;
    printf("Accessing logical address: %d\n", logical_address);
    printf("Page number: %d\n", page_number);
    printf("Frame number: %d\n", frame_number);
    printf("Offset: %d\n", offset);
    printf("Physical address: %d\n", physical_address);
    printf("Content of memory at physical address: %s\n", simulator->memory.frames[frame_number].data);
}

int main() {
    OnDemandPagingSimulator simulator;
    
    // Initialize page table entries as invalid
    for (int i = 0; i < NUM_PAGES; i++) {
        simulator.page_table.entries[i].frame_number = -1;
        simulator.page_table.entries[i].valid = 0;
    }

    // Access some memory addresses
    access_memory(&simulator, 0);
    access_memory(&simulator, 128);
    access_memory(&simulator, 256);
    access_memory(&simulator, 192);

    return 0;
}
