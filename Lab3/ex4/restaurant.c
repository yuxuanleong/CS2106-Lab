#include "restaurant.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

/******** Common Definition ********/
#define MAX_TABLE_TYPE 5

#define TABLE_TYPE_1P 0
#define TABLE_TYPE_2P 1
#define TABLE_TYPE_3P 2
#define TABLE_TYPE_4P 3
#define TABLE_TYPE_5P 4
#define TABLE_STATE_EMPTY 0
#define TABLE_STATE_OCCUPIED 1

#define GROUP_STATE_ARRIVE 0
#define GROUP_STATE_EAT 1
#define GROUP_STATE_WAIT 2
#define GROUP_STATE_LEAVE 3


/******** Custom TypeDef ********/
typedef struct table {
    int table_id;               //  ID of the table
    int table_state;            //  State of the table = {WAITING, OCCUPIED}
    int table_type;             //  Type of the table = {TABLE_1P ... TABLE_5P}
    struct table *next_table;   //  Pointer to the next table
}table;


/******** Global Var ********/
struct table *first_table;      //  Pointer to the first table

int queue_ticket_issued = 0;    //  Each ticket is issued for each group to track the FCFS nature
int total_tables = 0;           //  A global var to track the no of tables in the restaurant

/******** Semaphore ********/
sem_t mutex[MAX_TABLE_TYPE];
sem_t queue[MAX_TABLE_TYPE];
sem_t turnstile[MAX_TABLE_TYPE];
sem_t full_queue[MAX_TABLE_TYPE];


/******** Restaurant Operations ********/
//  Write initialization code here (called once at the start of the program).
//  It is guaranteed that num_tables is an array of length 5.
//  TODO
void restaurant_init(int num_tables[5]) {

    //  Creation of a linked list to track the states of the table

    struct table *current_table, *temp;

    int table_index = 0;

    //  For all table types
    for (int i = 0; i < MAX_TABLE_TYPE; i++) {
        //  For all tables in each type
        for (int j = 0; j < num_tables[i]; j++) {
            current_table = (table*) malloc(sizeof(table));
            current_table->table_id = table_index;              //  Assign ID
            current_table->table_type = i;                      //  Assign Table Type
            current_table->table_state = TABLE_STATE_EMPTY;     //  Assign Table State

            //  If this is the first table, head_table and temp both point to current_table
            if (table_index == 0) 
            {
                first_table = temp = current_table;
            } 
            //  Else, we join the current table with the previous table (temp)
            else {
                temp->next_table = current_table;
                temp = current_table;
            }

            table_index++;  //  Increment ID for next table
        }
    }
    
    temp->next_table = NULL;    //  Assign the next table to NULL, since it does not exist

    // Mutex Initiation
    for (int i = 0; i < MAX_TABLE_TYPE; i++) {
        sem_init(&mutex[i], 0, 1);
        sem_init(&queue[i], 0, 1);
        sem_init(&turnstile[i], 0, 1);
        sem_init(&full_queue[i], 0, num_tables[i]);
    }
}

// Write deinitialization code here (called once at the end of the program).
// TODO
void restaurant_destroy(void) {
}


// Write your code here.
// Return the id of the table you want this group to sit at.
// TODO
int request_for_table(group_state *state, int num_people) {
    return 0;
}

void leave_table(group_state *state) {
    // Write your code here.
    // TODO
}