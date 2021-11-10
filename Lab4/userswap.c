#include "userswap.h"

/*
Assumption
1. Size of page is 4096 (4kb) -> can ignore the 16 bits of virtual address when designing data struc

Limitation
1. no more than 128 bytes of overhead per page of mem in a single alloc
2. must no use mmap syscall to map a file into memory to perform file I/O
3. when using mmap -> fd argument must always be -1 -> even for userswap_map
*/


/*
1. sets the LORM to size
2. Rounding up too
3. total size of resident mem in all controlled regions is above the new LORM, do the minimum eviction using FIFO
*/
void userswap_set_size(size_t size) {

}

/*
1. allocate siez bytes of memory that is controlled by the swap scheme
2. return a pointer to the start of the memory
3. If size not a multiple of the page size, size should be rounded up to the next multiple of page size (quick maths)
4. This function can be called many times but cannot intervening userswap_free -> possible semaphore?
5. If SIGSEGV handler has not yet been installed when this function is called -> call internally for the handler
*/
void *userswap_alloc(size_t size) {
  // 
  // 1. allocate the requested amount of memory (rounded up as needed) using mmap
  // 2. Memory should be initially non-resident and allocated as PROT_NONE -> in order for any accesses to the memory to cause a page fault
  // 3. Should also install the SIGSEGV handler
  return NULL;
}

/*
1. Free from the start of the mem for each block
2. takes in a pointer that is previously returned by alloc or map (not freed)
3. if the mem region was allocated by map -> any changes made to the mem must be written to the file accordingly
4. fd should not be closed
*/
void userswap_free(void *mem) {
  //  1. Free the entire allocation starting at the provided address using munmap
  //  2. Need to track the size of each allocation in userswap_alloc.
  //  3. A simple linked list of allocations, storing the start address (from mmap) and size will be sufficient
}


/*
1. Should map the first size bytes of the file open in the file descriptior fd, using the swap scheme described above in the CMR 
2. if Size is not a multiple of the page size, size should be rounded up to the next multiple of the page size
3. file shall be known as backing file.
4. fd can be assumed to be a valid file decriptor open in read-write mode using the open syscall
5. no assumption should be made as to the current offset of the file descriptor
6. the fd, once handed to userswap_map, can be assumed to tbe fully controlled by our lib -> no other code will touch this
7. If the file is shorter than size bytes, then this function should also cause the file to be zero filled to size btyes (bit shifting)
8. same as userswap_alloc -> cannot intervening userswap_free
9. Call internally for SIGSEGV handler if not done so
*/
void *userswap_map(int fd, size_t size) {
  return NULL;
}

// Write a SIGSEGV handler.
// 1. Does not need to check whether the faulting memory address is within a controled memory region;
// 2. It can simply call the page fault handler.
// 3. The page fault handler will need the address to perform mprotect
// 4. Faulting memory address can be found in the signinfo t strcut passed to the signal handler

// Write a function to handle page faults (Where the logic bulk for the CMR will reside)
// 1. Page fault handler only needs to user mprotect to make the page containing the access memory PROT_READ, thereby making the page resident
// 2. Memory newly allocated by userswap_alloc should be initialized to zero.
// 3. Nothing needs to be doen for this, as memory allocated by mmap is always initialised to zero