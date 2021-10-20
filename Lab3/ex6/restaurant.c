#include "restaurant.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

/******** Common Definition ********/
#define ERROR_TABLE_ASSIGNMENT -1
#define LEGIT_PACKED -2
#define ERROR -1

#define MAX_TABLE_TYPE 5
#define MAX_TABLE_NUM 1000
#define MAX_CUSTOMER_GROUP_NUM 1000

#define TABLE_TYPE_1P 0
#define TABLE_TYPE_2P 1
#define TABLE_TYPE_3P 2
#define TABLE_TYPE_4P 3
#define TABLE_TYPE_5P 4
#define TABLE_STATE_UNOCCUPIED 0
#define TABLE_STATE_OCCUPIED 1

#define GROUP_TYPE_1P 0
#define GROUP_TYPE_2P 1
#define GROUP_TYPE_3P 2
#define GROUP_TYPE_4P 3
#define GROUP_TYPE_5P 4
#define GROUP_STATE_ARRIVE 0
#define GROUP_STATE_WAIT 1
#define GROUP_STATE_EAT 2
#define GROUP_STATE_LEAVE 3

#define GROUP_GONE 0
#define GROUP_NOT_GONE 1

#define FALSE 0
#define TRUE 1

/******** Prototype ********/
int assign_table(int num_people, group_state *state, int legit_want_to_eat);
int call_next_group(group_state *state, int num_people, int table_freed);
void free_table(group_state *state, int num_people);

/******** Global Var ********/
int table_id[MAX_TABLE_TYPE][MAX_TABLE_NUM];
int table_state[MAX_TABLE_TYPE][MAX_TABLE_NUM];     
int table_num[MAX_TABLE_TYPE] = {0, 0, 0, 0, 0};    //  To track the number of table in restaurant
int table_usage[MAX_TABLE_TYPE] = {0, 0, 0, 0, 0};  //  To track the usage of the table
int table_full_usage[MAX_TABLE_TYPE][MAX_TABLE_NUM];
int total_table = 0;

group_state* queue[MAX_CUSTOMER_GROUP_NUM];
int queue_grp_state[MAX_CUSTOMER_GROUP_NUM];

int new_group_index = 0;
int group_served = 0;

/******** Semaphore ********/
sem_t mutex[MAX_TABLE_TYPE];
sem_t customer_serve[MAX_TABLE_TYPE];
sem_t customer_done[MAX_TABLE_TYPE];
sem_t table_available[MAX_TABLE_TYPE];

/******** Restaurant Operations ********/

//  Main Purpose: Track Restaurant Information
//  Write initialization code here (called once at the start of the program).
//  It is guaranteed that num_tables is an array of length 5.
//  TODO
void restaurant_init(int num_tables[5]) {
    for (int i = 0; i < MAX_TABLE_TYPE; i++) {
        for (int j = 0; j < num_tables[i]; j++) {
            table_id[i][j] = total_table;
            table_state[i][j] = TABLE_STATE_UNOCCUPIED;
            total_table++;       
        }
        table_num[i] = total_table;
        sem_init(&mutex[i], 0, 1);
        sem_init(&customer_serve[i], 0, 1);
        sem_init(&customer_done[i], 0, 1);
        sem_init(&table_available[i], 0, num_tables[i]);
    }
}

// Write deinitialization code here (called once at the end of the program).
// TODO
void restaurant_destroy(void) {
    for (int i = 0; i < MAX_TABLE_TYPE; i++) {
        sem_destroy(&mutex[i]);
        sem_destroy(&customer_serve[i]);
        sem_destroy(&customer_done[i]);
        sem_destroy(&table_available[i]);
    }
}


// Write your code here.
// Return the id of the table you want this group to sit at.
// TODO
int request_for_table(group_state *state, int num_people) {
    
    num_people--;
    //  Simulate a scene where a group comes into the restaurant,
    //  1.  Record down the information for the group
    //  2.  Put the group into queue
    sem_wait(&mutex[num_people]);
        on_enqueue();
        //  Information Record
        state->state_of_group = GROUP_STATE_WAIT;
        state->queue_ticket = new_group_index;
        state->size_of_group = num_people;
        state->size_of_table_to_fit = num_people;
        state->legit_want_to_eat = FALSE;
        queue[new_group_index] = state;
        queue_grp_state[new_group_index] = GROUP_NOT_GONE;
        new_group_index++;

        int table_left_for_specific_type;
        int table_type = num_people;

        //  For all the table size >= group size 
        for (int i = num_people; i < MAX_TABLE_TYPE; i++) {
            table_type = i;
            sem_getvalue(&table_available[i], &table_left_for_specific_type);

            //  If there is table left for this table type
            if (table_left_for_specific_type > 0) {
                state->size_of_table_to_fit = table_type;   //  Change the size of this group to fit the table size
                sem_init(&state->my_turn_to_go, 0, 1);      //  call them in
                break;
            }
        }

        //  table_left_for_specific_group that falls through the above segment will be the table of 5's
        //  if at **here**, table_left_for_specific_group is 0, then it means that there are no empty left to allocate

        int table_id_to_allocate = LEGIT_PACKED;
        //  If there is table_left_for_specfic_type, this looping would not be called
        //  This will only be executed when there is non-empty table left to allocate the group
        if (table_left_for_specific_type == 0) {
            
            //  Find the non_empty table that can allocate the group
            //  To iterate through all tables of the size bigger than the current group number
            for (int i = num_people + 1; i < MAX_TABLE_TYPE; i++) {
                for (int j = 0; j < table_num[i]; j++) {
                    //  table_size - no. of people already at the table >= size of the group 
                    //  -> True -> allocate that table to the group
                    if (i - table_full_usage[num_people][j] >= num_people) {
                        table_id_to_allocate = table_id[num_people][j];
                        state->table_assigned = table_id_to_allocate;
                        state->size_of_table_to_fit = i;
                        state->legit_want_to_eat = TRUE;
                        sem_init(&state->my_turn_to_go, 0, 1);
                        break;
                    }
                }
            }    
        }
        
        //  At **here**, if table_id_to_allocate reaches here is still legit packed,
        //  it means that restaurant legit cannot take in anymore people

        //  If all table types >= group_size, there is not a single table left
        if (table_left_for_specific_type == 0 && table_id_to_allocate == LEGIT_PACKED) {
            sem_init(&state->my_turn_to_go, 0, 0);      //  send them off to mall
        }
    //  At this line, num_people is not changed yet
    sem_post(&mutex[num_people]);

    sem_wait(&state->my_turn_to_go);                    //  Group received call to enter

    if (state->legit_want_to_eat) {
        sem_post(&table_available[state->size_of_table_to_fit]);    //  Open the table for eat specfically for the customer
    }

    sem_wait(&table_available[state->size_of_table_to_fit]);   //  Group takes the table according to the size of the table allocated to them.

    sem_wait(&customer_serve[state->size_of_table_to_fit]);  //  table service for only 1 group
        //  Assign table to the group
        int table_assigned = assign_table(state->size_of_table_to_fit, state, state->legit_want_to_eat); //  Assignment of table according to table type allocated
    sem_post(&customer_serve[state->size_of_table_to_fit]);

    return table_assigned;
}

// Write your code here.
// TODO
void leave_table(group_state *state) {

    //  Remember that this num_people is can be an imaginary number of people that is forced to fit the table type
    int num_people = state->size_of_group;
    int table_freed = state->table_assigned;
    //  This semaphore here is to allow customer grp to leave one by one
    sem_wait(&customer_done[num_people]);
        free_table(state, num_people);                      //  Mark table used by gorup as empty
        state->state_of_group = GROUP_STATE_LEAVE;          //  Change group state as left
        queue_grp_state[state->queue_ticket] = GROUP_GONE;  //  Remarks Queue says that the group has left
        int special_call = call_next_group(state, num_people, table_freed);    //  Service called the next group
        if (!special_call) {
            sem_post(&table_available[num_people]);             //  Free up the table for the next group
        }
    sem_post(&customer_done[num_people]);
}

int assign_table(int num_people, group_state *state, int legit_want_to_eat) {
    
    //  This segment is for legit hungry people who wants to be in the queue
    //  They will have a table already allocated for them
    //  Thus they are not normal grp and we do not want this group to go into the else condition
    if (legit_want_to_eat) {
        //  Find the table already allocated to them
        for (int i = 0; i < table_num[state->size_of_table_to_fit]; i++) {
            if (table_id[state->size_of_table_to_fit][i] == state->table_assigned){
                table_full_usage[state->size_of_table_to_fit][i] += state->size_of_group;   //  Increment the table usage for the specific table
                state->state_of_group = GROUP_STATE_EAT;                                    //  Group is now eating
                group_served++;                                                             //  INcrement the number of groups being served
                return table_id[state->size_of_table_to_fit][i];
            }
        }
    
    //  This segment is for normal people who are no so desperate to eat food
    //  They go through the normal procedure as Ex4 and Ex5
    } else {
        //  Find a table free for sitting
        for (int i = 0; i < table_num[num_people]; i++) {
            if (table_state[num_people][i] == TABLE_STATE_UNOCCUPIED) {
                table_full_usage[num_people][i] += num_people;          //  Increment the table usage for the specific table
                table_state[num_people][i] = TABLE_STATE_OCCUPIED;      //  Mark table as used
                state->state_of_group = GROUP_STATE_EAT;                //  Group is now eating
                state->table_assigned = table_id[num_people][i];        //  Record down the table number
                group_served++;                                         //  Increment the number of groups being served
                return table_id[num_people][i];
            }
        }
    }
    //  If this is reached, it means that there is logic fault in sempahores
    return ERROR_TABLE_ASSIGNMENT;
}

int call_next_group(group_state* state, int num_people, int table_freed) {
    //  Priority 1: Find a group to fit into an empty table
    for (int i = 0; i < new_group_index; i++) {
        //  Find a group that is not gone [Waiting]
        if (queue_grp_state[i] == GROUP_NOT_GONE) {
            //  Check if the table fits and their status
            if (queue[i]->state_of_group == GROUP_STATE_WAIT && queue[i]->size_of_group <= num_people) {
                queue[i]->size_of_group = num_people;   //  Change the size_of_group to fit the table type (sync the size of grp to table type)
                sem_post(&queue[i]->my_turn_to_go);     //  Call the group to come
                return 0;
            }
        }
    }

    int index_of_table_free;
    for (int i = 0; i < table_num[state->size_of_table_to_fit]; i++) {
        if (table_freed == table_id[state->size_of_table_to_fit][i]) {
            index_of_table_free = i;
        }
    }

    //  Find a desperate to eat group based on the table that is just freed
    for (int i = 0; i < new_group_index; i++) {
        //  Find a group that is not gone [Waiting]
        if (queue_grp_state[i] == GROUP_NOT_GONE) {
            //  Check if the table that was just freed is able to fit any group
            if (state->size_of_table_to_fit - table_full_usage[state->size_of_table_to_fit][index_of_table_free] >= state->size_of_group) {
                state->table_assigned = table_id[state->size_of_table_to_fit][index_of_table_free];
                state->legit_want_to_eat = TRUE;
                sem_post(&queue[i]->my_turn_to_go);
                return 1;
            }
        }
    }
    return ERROR;
}

void free_table(group_state *state, int num_people) {
    for (int i = 0; i < table_num[state->size_of_table_to_fit]; i++) {
        //  Find the table that was used    
        if (state->table_assigned == table_id[state->size_of_table_to_fit][i]) {
            //  Clean the table for next group
            table_full_usage[state->size_of_table_to_fit][i] -= state->size_of_group;   //  Decrement the people who are sitting in the table
            //  If no one at the table
            if (table_full_usage[state->size_of_table_to_fit][i] == 0) {
                table_state[num_people][i] = TABLE_STATE_UNOCCUPIED;                    //  Set table to be free         
            }
            return;
        }
    }
    return;
}
