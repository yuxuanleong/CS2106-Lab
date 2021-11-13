/*
  Assumption
  1. Size of page is 4096 (4kb) -> can ignore the 16 bits of virtual address when designing data struc

  Limitation
  1. no more than 128 bytes of overhead per page of mem in a single alloc
  2. must no use mmap syscall to map a file into memory to perform file I/O
  3. when using mmap -> fd argument must always be -1 -> even for userswap_map
*/

#include "userswap.h"
#include <stdio.h>   // stdio includes printf
#include <stdlib.h>  // stdlib includes malloc() and free()
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define TRUE 1
#define FALSE 0
#define STANDARD_PAGE_SIZE 4096
#define USER_PROT_NONE 0
#define USER_PROT_READ 1
#define USER_PROT_READWRITE 2
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


//----------------------------Data Structure----------------------------

typedef struct pageColumnEntry {
  void* address;
  size_t size;
  int currentAccessStatus;
  STAILQ_ENTRY(pageColumnEntry) pageColumnEntries;  
} pageColumnEntry;

typedef STAILQ_HEAD(pageColumnHead, pageColumnEntry) pageColumnHead;

typedef struct entry {
  void* address;
  size_t size;
  pageColumnHead head;
  STAILQ_ENTRY(entry) entries; 
} entry;

typedef STAILQ_HEAD(stailhead, entry) stailhead;

typedef struct LORM_entry {
  void* address;
  size_t size;
  STAILQ_ENTRY(LORM_entry) LORM_entries;
} LORM_entry;

typedef STAILQ_HEAD(LORM_head, LORM_entry) LORM_head;

//----------------------------Global----------------------------

stailhead global_stailhead;
LORM_head global_LORM_head;
int is_sigsegv_handler_assigned = 0;
size_t FIX_LORM_SIZE;
size_t current_LORM_SIZE = 0;

//----------------------------Prototypes----------------------------

size_t roundUp(size_t numToRound, size_t multiple);

void page_fault_handler(void* address, entry *target_entry);
void assign_sigsegv_handler(void);

entry *search_entry(void *mem);

pageColumnHead attach_Page_Column(void* startingAddress, size_t total_size);
void destroy_Page_Column(pageColumnHead head);
void mark_Page_Column_Entry(pageColumnEntry* targetEntry, int label);
pageColumnEntry *search_Page_Column_Entry(entry* MCR_entry, void *mem);

void destroy_LORM(void);
int calculate_no_of_pages_to_evict(size_t expected_LORM_SIZE);
void insert_entry_to_LORM(pageColumnEntry* targetEntry, entry *target_entry);
void evict_LORM_pages(int no_of_pages, entry *target_entry);
LORM_entry* record_PageEntry_to_LORMEntry(pageColumnEntry* targetEntry);

//----------------------------Misc Functions----------------------------

size_t roundUp(size_t numToRound, size_t multiple) {
  if (multiple == 0)
    return numToRound;

  size_t remainder = numToRound % multiple;
  if (remainder == 0)
    return numToRound;

  return numToRound + multiple - remainder;
}

//----------------------------CMR Functions----------------------------

entry *search_entry(void *mem) {
  entry *entryFound = STAILQ_FIRST(&global_stailhead);

  while (entryFound != NULL) {
    if (mem >= entryFound->address && mem < entryFound->address + entryFound->size) {
      return entryFound;
    }

    entryFound = STAILQ_NEXT(entryFound, entries);
  }

  return NULL;
}

//----------------------------Page Column Functions----------------------------

pageColumnHead attach_Page_Column(void* startingAddress, size_t total_size) {
  pageColumnHead localHead;
  STAILQ_INIT(&localHead);

  // printf("total size: %ld\n", total_size);
  double no_of_pages = total_size / STANDARD_PAGE_SIZE;
  // printf("no_of_pages: %f\n", no_of_pages);
  void* local_Address = startingAddress;

  while (no_of_pages > 0) {
    pageColumnEntry *newPageEntry = malloc(sizeof(pageColumnEntry));
    newPageEntry->address = local_Address;
    newPageEntry->size = STANDARD_PAGE_SIZE;
    newPageEntry->currentAccessStatus = USER_PROT_NONE;
    STAILQ_INSERT_TAIL(&localHead, newPageEntry, pageColumnEntries);
    local_Address = (void*)((uintptr_t) local_Address + STANDARD_PAGE_SIZE);
    no_of_pages--;
  }
  return localHead;
}

void destroy_Page_Column(pageColumnHead head) {
  pageColumnEntry *temp1, *temp2;
  temp1 = STAILQ_FIRST(&head);
  // int i = 0;
  while (temp1 != NULL) {
    // printf("executed: %d\n", i++);
    temp2 = STAILQ_NEXT(temp1, pageColumnEntries);
    free(temp1);
    temp1 = temp2;
  }
}

void mark_Page_Column_Entry(pageColumnEntry* targetEntry, int label) {
  targetEntry->currentAccessStatus = label;
}

pageColumnEntry *search_Page_Column_Entry(entry* MCR_entry, void *mem) {
  pageColumnEntry *page_entry_Found;
  page_entry_Found = STAILQ_FIRST(&(MCR_entry->head));

  while (page_entry_Found != NULL) {
    if (mem >= page_entry_Found->address && mem < page_entry_Found->address + page_entry_Found->size) {
      return page_entry_Found;
    }

    page_entry_Found = STAILQ_NEXT(page_entry_Found, pageColumnEntries);
  }

  return NULL;
}

//----------------------------LORM Functions----------------------------

void destroy_LORM(void) {
  LORM_entry *temp1, *temp2;
  temp1 = STAILQ_FIRST(&global_LORM_head);
  // int i = 0;
  while (temp1 != NULL) {
    // printf("executed: %d\n", i++);
    temp2 = STAILQ_NEXT(temp1, LORM_entries);
    free(temp1);
    temp1 = temp2;
  }
}

void insert_entry_to_LORM(pageColumnEntry* targetEntry, entry *target_entry) {   // to insert at tail

  size_t expected_LORM_SIZE = current_LORM_SIZE + targetEntry->size;

  LORM_entry* new_LORM_entry = record_PageEntry_to_LORMEntry(targetEntry);

  if (expected_LORM_SIZE > FIX_LORM_SIZE) {
    int no_of_pages_to_evict = calculate_no_of_pages_to_evict(expected_LORM_SIZE);
    evict_LORM_pages(no_of_pages_to_evict, target_entry);
    STAILQ_INSERT_TAIL(&global_LORM_head, new_LORM_entry, LORM_entries);
  } else {
    STAILQ_INSERT_TAIL(&global_LORM_head, new_LORM_entry, LORM_entries);
  }

}

int calculate_no_of_pages_to_evict(size_t expected_LORM_SIZE) {
  size_t offset = expected_LORM_SIZE - FIX_LORM_SIZE;

  int no_of_pages = ceil(offset / STANDARD_PAGE_SIZE) + 1;
  
  return no_of_pages;
}

void evict_LORM_pages(int no_of_pages, entry *target_entry) {
  
  LORM_entry* temp;

  while (no_of_pages > 0) {
    //  Remove from the back
    temp = STAILQ_FIRST(&global_LORM_head);
    void* target_address = temp->address;

    if (mprotect(target_address, STANDARD_PAGE_SIZE, PROT_NONE) == -1) {
      handle_error("mprotect failed in evict_LORM_pages");
    } else {
      pageColumnEntry* targetPageEntry = search_Page_Column_Entry(target_entry, target_address);
      mark_Page_Column_Entry(targetPageEntry, USER_PROT_NONE);
    }

    STAILQ_REMOVE_HEAD(&global_LORM_head, LORM_entries);
    free(temp);
    no_of_pages--;
  }
}

LORM_entry* record_PageEntry_to_LORMEntry(pageColumnEntry* targetEntry) {
  LORM_entry* new_LORM_entry = malloc(sizeof(LORM_entry));
  new_LORM_entry->address = targetEntry->address;
  new_LORM_entry->size = targetEntry->size;
  return new_LORM_entry;
}

//----------------------------SIGSEGV Functions----------------------------

void sigsegv_handler(int signo, siginfo_t *info, void *context) {
  void *address = info->si_addr;

  entry *check_entry = search_entry(address);

  // if the address is not in CMR, reset the signal to SIGSEGV
  if (check_entry == NULL) {
    signal(SIGSEGV, SIG_DFL);
  }

  page_fault_handler(address, check_entry);
}

void page_fault_handler(void* address, entry *target_entry) {
  void *targetedAddr = (void *) ((( (uintptr_t) address + STANDARD_PAGE_SIZE) / STANDARD_PAGE_SIZE - 1) * STANDARD_PAGE_SIZE);
  entry* first_level_search = search_entry(targetedAddr);
  pageColumnEntry* second_level_search = search_Page_Column_Entry(first_level_search, targetedAddr);
  int access_status = second_level_search->currentAccessStatus;
  
  switch(access_status) {
    case USER_PROT_NONE:
      if (mprotect(targetedAddr, STANDARD_PAGE_SIZE, PROT_READ) == -1) {
        handle_error("mprotect failed in page_fault_handler");
      } else {
        insert_entry_to_LORM(second_level_search, first_level_search);
        second_level_search->currentAccessStatus = USER_PROT_READ;
      }
      break;
    case USER_PROT_READ:
      if (mprotect(targetedAddr, STANDARD_PAGE_SIZE, PROT_READ | PROT_WRITE) == -1) {
        handle_error("mprotect failed in page_fault_handler");
      } else {
        second_level_search->currentAccessStatus = USER_PROT_READWRITE;
      }
      break;
    default:
      printf("Current page is all mighty\n");

  }
}

void assign_sigsegv_handler(void) {
  if (is_sigsegv_handler_assigned) {
    return;
  }

  is_sigsegv_handler_assigned = 1;
  STAILQ_INIT(&global_stailhead);

  struct sigaction act = {0};
  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = &sigsegv_handler;

  if (sigaction(SIGSEGV, &act, NULL) == -1) {
    handle_error("assign_sigsegv_handler");
  }
}

// ----------------------------Assignment Functions----------------------------

/*
  1. sets the LORM to size
  2. Rounding up too
  3. total size of resident mem in all controlled regions is above the new LORM, do the minimum eviction using FIFO
*/
void userswap_set_size(size_t size) { 
  FIX_LORM_SIZE = roundUp(size, STANDARD_PAGE_SIZE);
  STAILQ_INIT(&global_LORM_head);
}

/*
  1. allocate siez bytes of memory that is controlled by the swap scheme (ex0)
  2. return a pointer to the start of the memory
  3. If size not a multiple of the page size, size should be rounded up to the next multiple of page size (quick maths)
  4. This function can be called many times but cannot intervening userswap_free -> possible semaphore?
  5. If SIGSEGV handler has not yet been installed when this function is called -> call internally for the handler
*/
void *userswap_alloc(size_t size) {
  /* 
    1. allocate the requested amount of memory (rounded up as needed) using mmap (todo)
    2. Memory should be initially non-resident and allocated as PROT_NONE -> in order for any accesses to the memory to cause a page fault 
    3. Should also install the SIGSEGV handler
  */
  userswap_set_size(size);

  assign_sigsegv_handler();

  //  Round up the size
  size_t correctSize = roundUp(size, STANDARD_PAGE_SIZE);

  //  mmap
  void* mmapAddress = mmap(NULL, correctSize, PROT_NONE, MAP_ANON | MAP_SHARED, -1, 0);

  if (mmapAddress == NULL) {
      handle_error("mmap in userswap_alloc");
  }

  //  Data Recording
  entry *newEntry = (entry *) malloc(sizeof(entry));
  newEntry->address = mmapAddress;
  newEntry->size = correctSize;
  newEntry->head = attach_Page_Column(mmapAddress, size);

  //  Insert into the STQ list
  STAILQ_INSERT_HEAD(&global_stailhead, newEntry, entries);

  return mmapAddress;
}

/*
  1. Free from the start of the mem for each block
  2. takes in a pointer that is previously returned by alloc or map (not freed)
  3. if the mem region was allocated by map -> any changes made to the mem must be written to the file accordingly
  4. fd should not be closed
*/
void userswap_free(void *mem) {
  /*  
    1. Free the entire allocation starting at the provided address using munmap
    2. Need to track the size of each allocation in userswap_alloc.
    3. A simple linked list of allocations, storing the start address (from mmap) and size will be sufficient
  */
  entry* targetEntry = search_entry(mem);

  if (targetEntry == NULL) {
    printf("mem not in list");
  }

  destroy_Page_Column(targetEntry->head);

  if (munmap(targetEntry->address, targetEntry->size) == -1) {
    handle_error("userswap_free munmap err");
  }

  STAILQ_REMOVE(&global_stailhead, targetEntry, entry, entries);
  free(targetEntry);

  destroy_LORM();
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
  assign_sigsegv_handler();
  return NULL;
}

/* 
 Write a SIGSEGV handler.
 1. Does not need to check whether the faulting memory address is within a controled memory region;
 2. It can simply call the page fault handler.
 3. The page fault handler will need the address to perform mprotect
 4. Faulting memory address can be found in the signinfo t struct passed to the signal handler
*/

/*
 Write a function to handle page faults (Where the logic bulk for the CMR will reside)
 1. Page fault handler only needs to user mprotect to make the page containing the access memory PROT_READ, thereby making the page resident
 2. Memory newly allocated by userswap_alloc should be initialized to zero.
 3. Nothing needs to be doen for this, as memory allocated by mmap is always initialised to zero
 */