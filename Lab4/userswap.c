/*
Assumption
1. Size of page is 4096 (4kb) -> can ignore the 16 bits of virtual address when designing data struc

Limitation
1. no more than 128 bytes of overhead per page of mem in a single alloc
2. must no use mmap syscall to map a file into memory to perform file I/O
3. when using mmap -> fd argument must always be -1 -> even for userswap_map
*/

#include "userswap.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>   // stdio includes printf
#include <stdlib.h>  // stdlib includes malloc() and free()
#include <stddef.h>
#include <unistd.h>
#include <sys/queue.h>
#include <signal.h>

#define TRUE 1
#define FALSE 0
#define STANDARD_PAGE_SIZE 4096
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


//----------------------------Data Structure----------------------------
/*  
    Each Node, we store:
        1) starting address of page
        2) size of page
*/  
struct entry {
  char* address;
  size_t size;
  STAILQ_ENTRY(entry) entries;  
};


//----------------------------Global----------------------------
STAILQ_HEAD(stailhead, entry);    //  Declare STailQ
struct stailhead STQ_head;  //  Pointer to the stailq
int STQ_entry_no = 0;
size_t standard_page_size = STANDARD_PAGE_SIZE;


//----------------------------Prototypes----------------------------
size_t roundUp(size_t numToRound, size_t multiple);


// ----------------------------Internal Functions----------------------------

size_t roundUp(size_t numToRound, size_t multiple) {
  if (multiple == 0)
    return numToRound;

  size_t remainder = numToRound % multiple;
  if (remainder == 0)
    return numToRound;

  return numToRound + multiple - remainder;
}


// ----------------------------Assignment Functions----------------------------

/*
1. sets the LORM to size
2. Rounding up too
3. total size of resident mem in all controlled regions is above the new LORM, do the minimum eviction using FIFO
*/
void userswap_set_size(size_t size) {

}

/*
1. allocate siez bytes of memory that is controlled by the swap scheme (ex0)
2. return a pointer to the start of the memory
3. If size not a multiple of the page size, size should be rounded up to the next multiple of page size (quick maths)
4. This function can be called many times but cannot intervening userswap_free -> possible semaphore?
5. If SIGSEGV handler has not yet been installed when this function is called -> call internally for the handler
*/
void *userswap_alloc(size_t size) {
  // 
  // 1. allocate the requested amount of memory (rounded up as needed) using mmap (todo)
  // 2. Memory should be initially non-resident and allocated as PROT_NONE -> in order for any accesses to the memory to cause a page fault 
  // 3. Should also install the SIGSEGV handler

  //  Checks whether this is the first node -> This will only be performed once
  if (STQ_entry_no == 0) {
      STAILQ_INIT(&STQ_head);  
  }
  
  //  Increment the size of the STQ_entry_no
  STQ_entry_no++;

  //  Round up the size
  size_t correctSize = roundUp(size, standard_page_size);

  //  mmap
  char* mmapAddress = mmap(NULL, correctSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mmapAddress == MAP_FAILED) {
      handle_error("mmap");
  }

  //  Data Recording
  struct entry *newEntry;
  newEntry = malloc(sizeof(struct entry));
  newEntry->address = mmapAddress;
  newEntry->size = correctSize;

  //  Insert into the STQ list
  STAILQ_INSERT_TAIL(&STQ_head, newEntry, entries);

  return mmapAddress;
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

  struct entry *tempEntry_1, *tempEntry_2;
  tempEntry_1 = STAILQ_FIRST(&STQ_head);
  
  //  Traverse the list
  while (tempEntry_1 != NULL) {

    //  Get the first/next element
    tempEntry_2 = STAILQ_NEXT(tempEntry_1, entries);
    
    //  If we found the specified entry
    if (tempEntry_2->address == mem) {
  
      //  Retrieve the data
      char* addressToFree = tempEntry_2->address;
      size_t sizeToFree = tempEntry_2->size;
      
      //  Delete mapping
      munmap(addressToFree, sizeToFree);
      
      //  Remove entry from the STQ List
      STAILQ_REMOVE(&STQ_head, tempEntry_2, entry, entries);
      free(tempEntry_2);
      
      //  Stop the looping
      break;
    }

    //  Continue to find the specified entry
    tempEntry_1 = tempEntry_2;
  }

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
// 4. Faulting memory address can be found in the signinfo t struct passed to the signal handler

// Write a function to handle page faults (Where the logic bulk for the CMR will reside)
// 1. Page fault handler only needs to user mprotect to make the page containing the access memory PROT_READ, thereby making the page resident
// 2. Memory newly allocated by userswap_alloc should be initialized to zero.
// 3. Nothing needs to be doen for this, as memory allocated by mmap is always initialised to zero