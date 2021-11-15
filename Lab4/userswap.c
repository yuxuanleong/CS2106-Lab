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
#include <fcntl.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define TRUE 1
#define FALSE 0
#define STANDARD_PAGE_SIZE 4096
#define MAX_LORM_PAGE 2106
#define MAX_LORM_SIZE 8626176
#define USER_PROT_NONE 0
#define USER_PROT_READ 1
#define USER_PROT_READWRITE 2
#define USER_PROT_IN_SWAP_FILE 3
#define SWAP_SLOT_FREE 0
#define SWAP_SLOT_OCCUPIED 1
#define ALLOC_MODE 0
#define MAP_MODE 1
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

//----------------------------Data Structure----------------------------

typedef struct pageColumnEntry {
  void* address;
  size_t size;
  int currentAccessStatus;
  off_t SWAP_FILE_offset;
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

typedef struct SWAP_entry {
  off_t SWAP_FILE_offset;
  int status;
  CIRCLEQ_ENTRY(SWAP_entry) SWAP_entries;
} SWAP_entry;

typedef CIRCLEQ_HEAD(SWAP_head, SWAP_entry) SWAP_head;

//----------------------------Global----------------------------

int operation_type;

stailhead global_stailhead;
LORM_head global_LORM_head;
SWAP_head global_SWAP_head;
int is_sigsegv_handler_assigned = 0;
size_t FIX_LORM_SIZE;
size_t current_LORM_SIZE = 0;
int SWAP_FD;

struct stat MAP_buffer;
int MAP_FILE_status;
off_t original_MAP_File_size;

//----------------------------Prototypes----------------------------

size_t roundUp(size_t numToRound, size_t multiple);

void page_fault_handler(void* address, entry *target_entry);
void assign_sigsegv_handler(void);

entry *search_entry(void *mem);

pageColumnHead attach_Page_Column(void* startingAddress, size_t total_size);
pageColumnHead map_Page_Column(void* startingAddress, size_t total_size);
void destroy_Page_Column(pageColumnHead head);
void mark_Page_Column_Entry(pageColumnEntry* targetEntry, int label);
pageColumnEntry *search_Page_Column_Entry(entry* MCR_entry, void *mem);

void destroy_LORM(void);
int calculate_no_of_pages_to_evict(size_t expected_LORM_SIZE);
void insert_entry_to_LORM(pageColumnEntry* targetEntry, entry *target_entry, void* target_address);
void evict_LORM_pages(int no_of_pages, entry *target_entry,void* target_address);
LORM_entry* record_PageEntry_to_LORMEntry(pageColumnEntry* targetEntry);

void create_SWAP_File(size_t correct_size);
void destroy_SWAP_File_Tracker(void);
off_t search_for_empty_SWAP_Slot(void);
void mark_SWAP_slot_empty(off_t target_offset);
void write_into_SWAP_file(void* source_page_address, off_t targeted_SWAP_Slot);
void read_from_SWAP_file(void* target_page_address, off_t source_SWAP_offset);

void set_MAP_File(int fd, size_t correct_size);

//----------------------------Misc Functions----------------------------

/*
  Purpose: To round up the size to the multiple of page size
*/
size_t roundUp(size_t numToRound, size_t multiple) {
  if (multiple == 0)
    return numToRound;

  size_t remainder = numToRound % multiple;
  if (remainder == 0)
    return numToRound;

  return numToRound + multiple - remainder;
}

//----------------------------CMR Functions----------------------------

/*
  Purpose: To evaluate and return the CMR entry with the valid address
  Condidtion: Address must be within the CMR region allocated
*/
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

/*
  Purpose: To initialize a list of pages to the CMR entry's record
*/
pageColumnHead attach_Page_Column(void* startingAddress, size_t total_size) {
  pageColumnHead localHead;
  STAILQ_INIT(&localHead);

  double no_of_pages = total_size / STANDARD_PAGE_SIZE;
  void* local_Address = startingAddress;

  while (no_of_pages > 0) {
    pageColumnEntry *newPageEntry = malloc(sizeof(pageColumnEntry));
    newPageEntry->address = local_Address;                //  label with the right addr
    newPageEntry->size = STANDARD_PAGE_SIZE;              //  label with standard page size
    newPageEntry->currentAccessStatus = USER_PROT_NONE;   //  label with NON-Resident status
    STAILQ_INSERT_TAIL(&localHead, newPageEntry, pageColumnEntries);
    local_Address = (void*)((uintptr_t) local_Address + STANDARD_PAGE_SIZE);  //  increment to the next valid page address
    no_of_pages--;
  }
  return localHead;
}

/*
  Purpose: To initialize a list of pages to CMR entry's record 
*/
pageColumnHead map_Page_Column(void* startingAddress, size_t total_size) {
  pageColumnHead localHead;
  STAILQ_INIT(&localHead);

  double no_of_pages = total_size / STANDARD_PAGE_SIZE;
  void* local_Address = startingAddress;
  off_t offset = 0;

  while (no_of_pages > 0) {
    pageColumnEntry *newPageEntry = malloc(sizeof(pageColumnEntry));
    newPageEntry->address = local_Address;                          //  label with the right addr
    newPageEntry->size = STANDARD_PAGE_SIZE;                        //  label with standard page size
    newPageEntry->currentAccessStatus = USER_PROT_IN_SWAP_FILE;     //  label with Secondary File Resident Status
    newPageEntry->SWAP_FILE_offset = offset;                        //  label with the allocation slot in the Secondary File Resident Status
    STAILQ_INSERT_TAIL(&localHead, newPageEntry, pageColumnEntries);
    local_Address = (void*)((uintptr_t) local_Address + STANDARD_PAGE_SIZE);
    no_of_pages--;
    offset++;   //  Increment to record the next allocation slot in the Secondary File Resident Status
  }
  return localHead;
};

/*
  Purpose: Destroy the page column data structure
*/
void destroy_Page_Column(pageColumnHead head) {
  pageColumnEntry *temp1, *temp2;
  temp1 = STAILQ_FIRST(&head);
  while (temp1 != NULL) {
    temp2 = STAILQ_NEXT(temp1, pageColumnEntries);
    free(temp1);
    temp1 = temp2;
  }
}

/*
  Purpose: To mark the page entry's residency status 
*/
void mark_Page_Column_Entry(pageColumnEntry* targetEntry, int label) {
  targetEntry->currentAccessStatus = label;
}

/*
  Purpose: To evaluate and return the page entry with the valid address
  Condidtion: Address must be within the CMR region allocated
*/
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

/*
  Purpose: To destroy the LORM Tracker data structure
*/
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

/*
  Purpose: To move a page from non-resident to resident
*/
void insert_entry_to_LORM(pageColumnEntry* targetEntry, entry *target_entry, void* target_address) {   // to insert at tail
  
  current_LORM_SIZE += targetEntry->size;
  LORM_entry* new_LORM_entry = record_PageEntry_to_LORMEntry(targetEntry);

  if (current_LORM_SIZE > FIX_LORM_SIZE) {  //  If exceeded the capacity
    int no_of_pages_to_evict = calculate_no_of_pages_to_evict(current_LORM_SIZE);
    evict_LORM_pages(no_of_pages_to_evict, target_entry, target_address);
    STAILQ_INSERT_TAIL(&global_LORM_head, new_LORM_entry, LORM_entries);
  } else {
    STAILQ_INSERT_TAIL(&global_LORM_head, new_LORM_entry, LORM_entries);
  }
}

/*
  Purpose: To calculate the min. no of pages to evict
*/
int calculate_no_of_pages_to_evict(size_t expected_LORM_SIZE) {
  size_t offset = expected_LORM_SIZE - FIX_LORM_SIZE;

  int no_of_pages = ceil(offset / STANDARD_PAGE_SIZE) + 1;
  
  return no_of_pages;
}

/*
  Purpose: Evict a number of pages from the LORM
*/
void evict_LORM_pages(int no_of_pages, entry *target_entry, void* starget_address) {

  LORM_entry* temp;
  while (no_of_pages > 0) {
    temp = STAILQ_FIRST(&global_LORM_head); //  remove from the head
    void* target_address = temp->address;
    pageColumnEntry* targetPageEntry = search_Page_Column_Entry(target_entry, target_address);
    if (targetPageEntry->currentAccessStatus == USER_PROT_READWRITE) { // if the page is written, it is dirty

      if (operation_type == ALLOC_MODE) { //  ALLOC
        off_t free_SWAP_Slot = search_for_empty_SWAP_Slot();  //  search from an empty slot that is available in the swap file
        targetPageEntry->SWAP_FILE_offset = free_SWAP_Slot;   //  label the allocated slot in the swap file
        write_into_SWAP_file(target_address, free_SWAP_Slot); //  write
      } else {  
        write_into_SWAP_file(target_address, targetPageEntry->SWAP_FILE_offset ); //  just write based on the recorded allocation slot in the page record
      }

      targetPageEntry->currentAccessStatus = USER_PROT_IN_SWAP_FILE;  //  label the new status of the page as non resident
    }

    if (mprotect(target_address, STANDARD_PAGE_SIZE, PROT_NONE) == -1) {  //  syscall to remove from residency
      handle_error("mprotect failed in evict_LORM_pages");
    } else {
      mark_Page_Column_Entry(targetPageEntry, USER_PROT_IN_SWAP_FILE);  //  label again
      STAILQ_REMOVE_HEAD(&global_LORM_head, LORM_entries);  //  Safety remove from head
      free(temp);
      no_of_pages--;
      current_LORM_SIZE -= STANDARD_PAGE_SIZE;  //  Decrement the LORM size by 1 page size
    }

  }
}

/*
  Purpose: To copy the data of a page from non residency DS to it's residency DS
*/
LORM_entry* record_PageEntry_to_LORMEntry(pageColumnEntry* targetEntry) {
  LORM_entry* new_LORM_entry = malloc(sizeof(LORM_entry));
  new_LORM_entry->address = targetEntry->address;
  new_LORM_entry->size = targetEntry->size;
  return new_LORM_entry;
}

//----------------------------SIGSEGV Functions----------------------------

/*
  Requirement
*/
void sigsegv_handler(int signo, siginfo_t *info, void *context) {
  void *address = info->si_addr;

  entry *check_entry = search_entry(address);

  // if the address is not in CMR, reset the signal to SIGSEGV
  if (check_entry == NULL) {
    signal(SIGSEGV, SIG_DFL);
  }

  page_fault_handler(address, check_entry);
}

/*
  Purpose: To handle SIGSEGV
*/
void page_fault_handler(void* address, entry *target_entry) {
  void *targetedAddr = (void *) ((( (uintptr_t) address + STANDARD_PAGE_SIZE) / STANDARD_PAGE_SIZE - 1) * STANDARD_PAGE_SIZE);  //  adjust to correct address
  entry* first_level_search = search_entry(targetedAddr);   
  pageColumnEntry* second_level_search = search_Page_Column_Entry(first_level_search, targetedAddr);  //  locate the page from non residency
  int access_status = second_level_search->currentAccessStatus; //  check the page current access status
  switch(access_status) {
    case USER_PROT_NONE:  //  move to residency
      if (mprotect(targetedAddr, STANDARD_PAGE_SIZE, PROT_READ) == -1) {
        handle_error("mprotect failed in page_fault_handler");
      } else {
        insert_entry_to_LORM(second_level_search, first_level_search, targetedAddr);
        second_level_search->currentAccessStatus = USER_PROT_READ;
      }
      break;
    case USER_PROT_READ:  //  change access status to include write
      if (mprotect(targetedAddr, STANDARD_PAGE_SIZE, PROT_READ | PROT_WRITE) == -1) {
        handle_error("mprotect failed in page_fault_handler");
      } else {
        second_level_search->currentAccessStatus = USER_PROT_READWRITE;
      }
      break;
    case USER_PROT_IN_SWAP_FILE:  //  move from Secondary Storage to residency
      if (mprotect(targetedAddr, STANDARD_PAGE_SIZE, PROT_READ | PROT_WRITE) == -1) { //  set to RW access ability to information retrival
        handle_error("mprotect failed in page_fault_handler");
      } else {
        read_from_SWAP_file(targetedAddr, second_level_search->SWAP_FILE_offset); //  retrieve information
        insert_entry_to_LORM(second_level_search, first_level_search, targetedAddr);  //  move to residency
        if (mprotect(targetedAddr, STANDARD_PAGE_SIZE, PROT_READ) == -1 ) { //  set the access ability back only read
          handle_error("mprotect failed in page_fault_handler");
        } else {
          if (operation_type == ALLOC_MODE) {
            mark_SWAP_slot_empty(second_level_search->SWAP_FILE_offset);
          }
          second_level_search->currentAccessStatus = USER_PROT_READ;
        }
      }
      break;
    default:
      printf("Go listen to 21 pilots and go stare into empty space\n");
  }
}

/*
  Requirement
*/
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

//----------------------------SWAP File Functions----------------------------

/*
  Purpose: Create Swap file
*/
void create_SWAP_File(size_t correct_size) {
  int swap_file_name = getpid();
  char buffer[15];
  sprintf(buffer, "%d.swap", swap_file_name);

  SWAP_FD = open(buffer, O_CREAT|O_RDWR|O_TRUNC, 0777);

  if (SWAP_FD == -1) {
    handle_error("swap_fd creation has error\n");
  }

  CIRCLEQ_INIT(&global_SWAP_head);  //  Using a circular linked list to track available slots

  int max_swap_slots = correct_size / STANDARD_PAGE_SIZE; //  Calculate the max no of pages
  int i = 0;
  while (max_swap_slots > 0) {
    SWAP_entry* new_swap_slot = malloc(sizeof(SWAP_entry));
    new_swap_slot->SWAP_FILE_offset = i;  //  record the offset location allocated for this page in SWAP file
    new_swap_slot->status = SWAP_SLOT_FREE; //  record this slot as free
    CIRCLEQ_INSERT_TAIL(&global_SWAP_head, new_swap_slot, SWAP_entries);
    i++;
    max_swap_slots--;
  }
}

/* 
  Purpose: To destroy the SWAP File Track DS
*/
void destroy_SWAP_File_Tracker(void) {
  SWAP_entry *temp1, *temp2;
  temp1 = CIRCLEQ_FIRST(&global_SWAP_head);
  while (temp1 != (void *)&global_SWAP_head) {
    temp2 = CIRCLEQ_NEXT(temp1, SWAP_entries);
    free(temp1);
    temp1 = temp2;
  }
}

/*
  Purpose: Locate an empty slot in SWAP file
*/
off_t search_for_empty_SWAP_Slot(void) {
  SWAP_entry *temp1, *temp2;
  temp1 = CIRCLEQ_FIRST(&global_SWAP_head);
  while (temp1 != (void *)&global_SWAP_head) {
    if (temp1->status == SWAP_SLOT_FREE) {
      temp1->status = SWAP_SLOT_OCCUPIED; //  mark the slot as occupied
      break;
    }
    temp2 = CIRCLEQ_NEXT(temp1, SWAP_entries);
    temp1 = temp2;
  }
  return temp1->SWAP_FILE_offset;
}

/*
  Purpose: To mark a SWAP slot as empty
*/
void mark_SWAP_slot_empty(off_t target_offset) {
  SWAP_entry *temp1, *temp2;
  temp1 = CIRCLEQ_FIRST(&global_SWAP_head);
  while (temp1 != (void *)&global_SWAP_head) {
    if (temp1->SWAP_FILE_offset == target_offset) {
      temp1->status = SWAP_SLOT_FREE;
      break;
    }
    temp2 = CIRCLEQ_NEXT(temp1, SWAP_entries);
    temp1 = temp2;
  }
}

/*
  Purpose: Write evicted page content into SWAP slot
*/
void write_into_SWAP_file(void* source_page_address, off_t targeted_SWAP_Slot) {
  ssize_t checkBytes = pwrite(SWAP_FD, source_page_address, STANDARD_PAGE_SIZE, targeted_SWAP_Slot * STANDARD_PAGE_SIZE);
  if (checkBytes == -1) {
    handle_error("pwrite failed in write_into_SWAP_file\n");
  } else if (checkBytes != STANDARD_PAGE_SIZE) {
    handle_error("pwrite failed to fully write in write_into_SWAP_file\n");
  }
  madvise(source_page_address, STANDARD_PAGE_SIZE, MADV_DONTNEED);
}

/*
  Purpose: Retrieve information from a SWAP slot into a page
*/
void read_from_SWAP_file(void* target_page_address, off_t source_SWAP_offset) { 
  ssize_t checkBytes = pread(SWAP_FD, target_page_address, STANDARD_PAGE_SIZE, source_SWAP_offset * STANDARD_PAGE_SIZE);
  if (checkBytes == -1) {
    handle_error("pread failed in read_from_SWAP_file\n");
  } else if (checkBytes != STANDARD_PAGE_SIZE) {
    handle_error("pread failed to fully read in read_from_SWAP_file\n");
  }
}


// ----------------------------MAP File Functions----------------------------

/*
  Purpose: Set up MAP File tracker DS
*/
void set_MAP_File(int fd, size_t correct_size) {
  MAP_FILE_status = fstat(fd, &MAP_buffer);
  SWAP_FD = fd;

  CIRCLEQ_INIT(&global_SWAP_head);

  int max_swap_slots = correct_size / STANDARD_PAGE_SIZE;
  int i = 0;
  while (max_swap_slots > 0) {
    SWAP_entry* new_swap_slot = malloc(sizeof(SWAP_entry));
    new_swap_slot->SWAP_FILE_offset = i;
    CIRCLEQ_INSERT_TAIL(&global_SWAP_head, new_swap_slot, SWAP_entries);
    i++;
    max_swap_slots--;
  }

  if (ftruncate(SWAP_FD, correct_size) == -1) {
    handle_error("ftruncate failed in set_MAP_File\n");
  }
}


// ----------------------------Assignment Functions----------------------------

/*
  1. sets the LORM to size
  2. Rounding up too
  3. total size of resident mem in all controlled regions is above the new LORM, do the minimum eviction using FIFO
*/
void userswap_set_size(size_t size) { 

  if (size > MAX_LORM_SIZE) {
    FIX_LORM_SIZE = MAX_LORM_SIZE;
  } else {
    FIX_LORM_SIZE = size;
  }

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
  operation_type = ALLOC_MODE;
  assign_sigsegv_handler();

  //  Round up the size
  size_t correctSize = roundUp(size, STANDARD_PAGE_SIZE);

  userswap_set_size(correctSize);
  create_SWAP_File(correctSize);

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

  if (operation_type == MAP_MODE) {
    pageColumnEntry *temp1, *temp2;
    temp1 = STAILQ_FIRST(&targetEntry->head);
    while (temp1 != NULL) {
      temp2 = STAILQ_NEXT(temp1, pageColumnEntries);
      if (temp1->currentAccessStatus == USER_PROT_READWRITE) { // dirty
        write_into_SWAP_file(temp1->address, temp1->SWAP_FILE_offset);
      }
      temp1 = temp2;
    }
  }

  destroy_Page_Column(targetEntry->head);
  
  if (munmap(targetEntry->address, targetEntry->size) == -1) {
    handle_error("userswap_free munmap err");
  }

  STAILQ_REMOVE(&global_stailhead, targetEntry, entry, entries);
  free(targetEntry);

  destroy_LORM();
  destroy_SWAP_File_Tracker();
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
  
  operation_type = MAP_MODE;
  assign_sigsegv_handler();

  original_MAP_File_size = size;
  size_t correctSize = roundUp(size, STANDARD_PAGE_SIZE);
  
  userswap_set_size(correctSize);
  set_MAP_File(fd, correctSize);

  //  mmap
  void* mmapAddress = mmap(NULL, correctSize, PROT_NONE, MAP_ANON | MAP_SHARED, -1, 0);

  if (mmapAddress == NULL) {
      handle_error("mmap in userswap_alloc");
  }

  entry *newEntry = (entry *) malloc(sizeof(entry));
  newEntry->address = mmapAddress;
  newEntry->size = correctSize;
  newEntry->head = map_Page_Column(mmapAddress, size);

  // Insert into the STQ list
  STAILQ_INSERT_HEAD(&global_stailhead, newEntry, entries);

  return mmapAddress;
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
