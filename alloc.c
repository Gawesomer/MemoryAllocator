#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>
#include "alloc.h"

/*
 * Available chunck:
 *      Header
 *      Node
 *      Footer
 * Used Chunck:
 *      Header
 *      Data
 *      Footer
 *
 * Padding:
 *      No padding is required as headers and footers are both of a size of 16B
 *      and allocated memory is rounded up to 16B so everything is 16B alligned.
 */

// Block header
typedef struct header_t {
    int size;                   // Size available in given chunck. Does not count header and footer.
    int free;
    struct header_t *pair;      // If footer points to associated header and vice versa
} header_t;

// Free list node
typedef struct node_t {
    struct node_t *next, *prev;
} node_t;

header_t *top;          // Start of memory chunck
header_t *bot;          // End of memory chunck
node_t *head;           // Head of the free list
node_t *current;        // Current node for next fit
int init = 0;           // Has M_Init been successfully called

header_t *getHeader(void *node) {
    header_t *result = (header_t*)node;
    result--;
    return result;
}

// Returns x rounded up to nearest multiple of offset
int roundUp(int x, int offset) {
    int mod = x%offset;
    if (mod == 0) {
        return x;
    }
    return x + (offset-mod);
}

// Advance pointer through free list. Loops back to head if got to end of list.
void circularNext(node_t *ptr) {
    if (ptr == NULL) {
        return;
    }
    ptr = ptr->next;
    if (ptr == NULL) {
        ptr = head;
    }
}

// Returns 1 if big enough 0 otherwise
// size - size needed for allocation in bytes must be rounded up to nearest 
//        multiple of 16
int bigEnough(node_t *ptr, int size) {
    header_t *header = getHeader(ptr);
    if (header->size >= size) {
        return 1;
    }
    return 0;
}

// ptr - Points to start of memory chunck
// size - size of entire memory chunck
// free - 1: Available; 0: Allocated
void setHeaderFooter(void *ptr, int size, int free) {
    header_t *header, *footer;

    header = ptr;
    header->size = size - 2*sizeof(header_t);
    header->free = free;
    
    footer = (header_t*)((char*)ptr + size - sizeof(header_t));
    footer->size = header->size;
    footer->free = header->free;
    footer->pair = header;
    header->pair = footer;
}

void prependFree(node_t *node) {
    if (head == NULL) {
        node->prev = NULL;
        node->next = NULL;
        head = node;
        current = node;
        return;
    }
    node->next = head;
    head->prev = node;
    node->prev = NULL;
    if (current == head) {
        current = node;
    }
    head = node;
}

void *getAddress(header_t *header) {
    return (char *) (header+1);
}

node_t *getNode(header_t *header) {
    return (node_t*)(header+1);
}

void removeFreeBlock(header_t *header) {
    node_t *toRemove = getNode(header);
    node_t *prev = toRemove->prev;
    node_t *next = toRemove->next;
    if (head == toRemove) {
        if (current == head) {
            current = head->next;
        }
        head = head->next;
    }
    if (prev != NULL) {
        prev->next = next;
    }
    if (next != NULL) {
        next->prev = prev;
    }
}

// Get header of next chunck
header_t *getNext(header_t *header) {
    /*
    header_t *ptr = header+2;
    ptr = (header_t*)((char*)ptr + header->size);
    */

    header_t *ptr = header->pair+1;

    if (ptr >= bot) {
        return NULL;
    }
    return ptr;
}

// Get header of previous chunck
header_t *getPrev(header_t *header) {
    header_t *ptr;

    if (header <= top) {
        return NULL;
    }

    ptr = header-1;
    /*
    ptr = (header_t*)((char*)ptr - ptr->size);
    ptr -= 1;
    */
    ptr = ptr->pair;

    return ptr;
}

int M_Init(int size) {
    void *ptr;

    // Verbose
    printf("M_Init(%d)\n", size);

    if (init != 0) {
        // M_Init has already been called
        return -1;
    }
    if (size <= 0) {
        // Can't allocate negative memory space
        return -1;
    }

    // Request chunck of memory
    ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    // Verbose
    printf("sizeof(header_t) = %ld,\t sizeof(node_t) = %ld\n", sizeof(header_t), sizeof(node_t));

    setHeaderFooter(ptr, size, 1);

    top = ptr;
    bot = (header_t*)((char*)top + size);

    head = (node_t*) (((header_t*)ptr)+1);
    head->next = NULL;
    head->prev = NULL;
    current = head;
    init = 1;

    // Verbose
    M_DisplayAll();

    return 0;
}

void displayFreeList() {
    node_t *ptr;

    printf("---displayFreeList\n");
    printf("\tcurrent = %p\n", (void*)current);

    printf("\tFree List:\n\t\t");
    ptr = head;
    while (ptr != NULL) {
        printf("%p - ", (void*)ptr);
        ptr = ptr->next;
    }
    printf("\n\n");
    
}

void *M_Alloc(int size) {
    node_t *start;
    node_t *freeNode;
    header_t *allocated, *free;

    // Verbose
    printf("M_Alloc(%d)\n", size);

    if (size <= 0) {
        return NULL;
    }
    if (current == NULL) {
        if (head != NULL) {
            fprintf(stderr, "ERROR: current is NULL but free list non-empty\n");
        }
        return NULL;
    }

    size = roundUp(size, 16);
    start = current;
    do {
        if (bigEnough(current, size)) {
            allocated = getHeader(current);
            if (allocated->size > size + 2*sizeof(header_t) + sizeof(node_t)) {
                // We have enough space to split and make another free chunck
                free = (header_t*) ((char*)allocated + ((size + 2*sizeof(header_t))/sizeof(char)) );   
                setHeaderFooter(free, allocated->size-size, 1);
                freeNode = getNode(free);
                prependFree(freeNode);
            }
            circularNext(current);
            // Allocate chunck
            size += 2*sizeof(header_t);
            setHeaderFooter(allocated, size, 0);
            removeFreeBlock(allocated);

            // Verbose
            printf("M_Alloc: SUCCESS///\n");
            M_DisplayAll();

            return getAddress(allocated);
        } else {
            circularNext(current);
        }
    } while (current != start);

    // Verbose
    M_DisplayAll();

    return NULL;
}

void coallesce(header_t *header) {
    header_t *next, *prev;
    node_t *node;
    int nextFree, prevFree;

    next = getNext(header);
    prev = getPrev(header);

    if (next == NULL) {
        nextFree = 0;
    } else {
        nextFree = next->free;
    }
    if (prev == NULL) {
        prevFree = 0;
    } else {
        prevFree = prev->free;
    }

    if (!prevFree && !nextFree) {
        setHeaderFooter(header, header->size + 2*sizeof(header_t), 1);
        node = getNode(header);
        prependFree(node);
        return;
    }

    if (prevFree && nextFree) {
        setHeaderFooter(prev, prev->size + header->size + next->size + 6*sizeof(header_t), 1);
        if (getNode(next)->prev != NULL) {
            getNode(next)->prev->next = getNode(next)->next;
        }
        if (getNode(next)->next != NULL) {
            getNode(next)->next->prev = getNode(next)->prev;
        }
        return;
    }

    if (prevFree) {
        setHeaderFooter(prev, prev->size + header->size + 4*sizeof(header_t), 1);
        return;
    }
    if (nextFree) {
        setHeaderFooter(header, next->size + header->size + 4*sizeof(header_t), 1);
        node = getNode(header);
        node->prev = getNode(next)->prev;
        if (node->prev != NULL) {
            node->prev->next = node;
        }
        node->next = getNode(next)->next;
        if (node->next != NULL) {
            node->next->prev = node;
        }
        return;
    }
}

// Returns 0 on success -1 on failure
int M_Free(void *p) {
    header_t *header;
    
    // Verbose
    printf("M_Free(%p)\n", p);

    if (p == NULL) {
        return -1;
    }

    if (p < (void*)top || p >= (void*)bot) {
        return -1;
    }

    header = getHeader(p);
    coallesce(header);

    // Verbose
    M_DisplayAll();
 
    return 0;
}

void M_Display() {
    node_t *ptr = head;

    printf("---Free chuncks:\n");
    while (ptr != NULL) {
        printf("\tAddress: %p\t Size: %d\n", (void *)(getHeader(ptr)), getHeader(ptr)->size);
        ptr = ptr->next;
    }
}

void M_DisplayAll() {
    header_t *ptr = top;
    header_t *footer;
    node_t *node;

    M_Display();

    printf("---DisplayAll\n");
    while (ptr != NULL) {
        printf("Header: (%p)\n", (void*)ptr);
        printf("\tsize = %d,\t free = %d,\t pair = %p\n", ptr->size, ptr->free, (void*)ptr->pair);
        if (ptr->free) {
            node = getNode(ptr);
            printf("Node: (%p)\n", (void*)node);
            printf("\tnext = %p,\t prev = %p\n", (void*)node->next, (void*)node->prev);
        }
        footer = ptr->pair;
        printf("Footer: (%p)\n", (void*)footer);
        printf("\tsize = %d,\t free = %d,\t pair = %p\n", footer->size, footer->free, (void*)footer->pair);
        ptr = getNext(ptr);
    }

    printf("\n\t");
    ptr = top;
    while (ptr != NULL) {
        if (ptr->free) {
            printf("|    |");
        } else {
            printf("|####|");
        }
        ptr = getNext(ptr);
    }
    printf("\n\n");
}
