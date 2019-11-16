/**
 * Initializes memory allocator.
 * The calling program should only invoke this once.
 * Input:
 *      size    - Size of memory space in bytes that the allocator will manage.
 *                This is final. The size cannot be changed later on.
 * Output:
 *      0       - Success
 *      -1      - Otherwise
 */
int M_Init(int size);

/**
 * Allocates a chunck of memory.
 * The actual size of the allocated chunck is rounded up to the nearest multiple
 * of 16 bytes.
 * The allocated chunck will always be aligned on a 16 byte boundary.
 * (i.e. the returned address will always be divisible by 16)
 * Allocation uses the next-fit policy.
 * Input:
 *      size    - Requested size in bytes.
 * Output:
 *      Pointer to the start of the allocated chunck or NULL if the request 
 *      failed.
 */
void *M_Alloc(int size);

/**
 * Frees previously allocated chunck at address p.
 * Free space if coallesced.
 * Input:
 *      p       - Address of a previously allocated chunck that should be freed.
 * Output:
 *      0       - Success
 *      -1      - Otherwise
 */
int M_Free(void *p);

/**
 * Prints the addresses and sizes of the free chuncks to STDOUT.
 */
void M_Display();

/**
 * Prints layout of all memory managed by the allocator.
 */
void M_DisplayAll();
