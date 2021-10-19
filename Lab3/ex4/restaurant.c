#include "restaurant.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

/******** Common Definition ********/
#define ERROR_TABLE_ASSIGNMENT -1;

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

/******** Prototype ********/
int assign_table(int num_people, group_state *state);
void call_next_group(int num_people);
void free_table(group_state *state, int num_people);

/******** Global Var ********/
int table_id[MAX_TABLE_TYPE][MAX_TABLE_NUM];
int table_state[MAX_TABLE_TYPE][MAX_TABLE_NUM];     
int table_num[MAX_TABLE_TYPE] = {0, 0, 0, 0, 0};    //  To track the number of table in restaurant
int table_usage[MAX_TABLE_TYPE] = {0, 0, 0, 0, 0};  //  To track the usage of the table
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
        queue[new_group_index] = state;
        queue_grp_state[new_group_index] = GROUP_NOT_GONE;
        new_group_index++;

        int table_left;
        sem_getvalue(&table_available[num_people], &table_left);
        if (table_left > 0) {
            sem_init(&state->my_turn_to_go, 0, 1);
        } else {
            sem_init(&state->my_turn_to_go, 0, 0);
        }

    sem_post(&mutex[num_people]);

    sem_wait(&state->my_turn_to_go);
    sem_wait(&table_available[num_people]);

    sem_wait(&customer_serve[num_people]);
        //  Assign table to the group
        int table_assigned = assign_table(num_people, state);
    sem_post(&customer_serve[num_people]);

    return table_assigned;
}

// Write your code here.
// TODO
void leave_table(group_state *state) {
    //  Wakes up the next in the queue
    //  destroy the sem_t for those who left the queue
    //  sem_post the table avaiable
    int num_people = state->size_of_group;
    
    //  This semaphore here is to allow customer grp to leave one by one
    sem_wait(&customer_done[num_people]);
        free_table(state, num_people);
        state->state_of_group = GROUP_STATE_LEAVE;
        queue_grp_state[state->queue_ticket] = GROUP_GONE;
        call_next_group(num_people);
        sem_post(&table_available[num_people]);
    sem_post(&customer_done[num_people]);
}

int assign_table(int num_people, group_state *state) {
    for (int i = 0; i < table_num[num_people]; i++) {
        if (table_state[num_people][i] == TABLE_STATE_UNOCCUPIED) {
            table_state[num_people][i] = TABLE_STATE_OCCUPIED;
            state->state_of_group = GROUP_STATE_EAT;
            state->table_assigned = table_id[num_people][i];
            group_served++;
            return table_id[num_people][i];
        }
    }
    //  If this is reached, it means that there is logic fault in sempahores
    return ERROR_TABLE_ASSIGNMENT;
}

void call_next_group(int num_people) {
    for (int i = 0; i < new_group_index; i++) {
        if (queue_grp_state[i] == GROUP_NOT_GONE) {
            if (queue[i]->state_of_group == GROUP_STATE_WAIT && queue[i]->size_of_group == num_people) {
                sem_post(&queue[i]->my_turn_to_go);
                return;
            }
        }
    }
}

void free_table(group_state *state, int num_people) {
    for (int i = 0; i < table_num[num_people]; i++) {
        if (state->table_assigned == table_id[num_people][i]) {
            table_state[num_people][i] = TABLE_STATE_UNOCCUPIED;         
            return;
        }
    }
}