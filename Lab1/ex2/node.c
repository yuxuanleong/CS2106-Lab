/*************************************
* Lab 1 Exercise 2
* Name: Yang Shiyuan
* Student No: A0214269A
* Lab Group: B17
*************************************/

#include "node.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Add in your implementation below to the respective functions
// Feel free to add any headers you deem fit (although you do not need to)

/**
 * What are some of the common function we should have?
 * 1. getNode at the index
 * 2. create new node with data
 * 3. create a empty list
 */

node *get_node_pt(list *lst, int index) {
    
    int curr_index = 0;
    node *curr_node_pt = lst->head;

    while (curr_index != index) {
        curr_node_pt = curr_node_pt->next;
        curr_index++;
    }
    return curr_node_pt;
}

node *create_new_node_pt(int data) {

    node *new_node_pt = (node *) malloc(sizeof(node));
    new_node_pt->data = data;
    new_node_pt->next = NULL;
    return new_node_pt;
}

int is_empty_list(list *list_pt) {
    return list_pt->head == NULL;
}

void remove_node(node *prev_pt, node *curr_pt) {
    prev_pt->next = curr_pt->next;
    free(curr_pt);
}

void insert_node_after(node *this_node, node *new_node) {

    new_node->next = this_node->next;
    this_node->next = this_node;
}

/**
 * Essential things to keep track:
 * How to create a pointer
 * How to allocate memory to a pointer
 */


/**
 * Cases:
 * 1. if the list is empty, then we need to create the list and insert the node at zero
 * 2. if the list is not empty, then we need to find the node specify by the index,
 *      then get that node to point to the newNode, and then set the newNode to point to the next node
 * 3. if the node to insert is at the last place, then we need to point the newNode to the first node's address 
 */

// Inserts a new node with data value at index (counting from head
// starting at 0).
// Note: index is guaranteed to be valid.
void insert_node_at(list *lst, int index, int data) {

    node *new_node_pt_at_index = create_new_node_pt(data);

    if (is_empty_list(lst)) {
        lst->head = new_node_pt_at_index;
        new_node_pt_at_index->next = lst->head;
        return;
    }

    if (!index) {
        node* temp = lst->head;
        new_node_pt_at_index->next = temp;

        while (temp->next != lst->head) {
            temp = temp->next;
        }

        lst->head = new_node_pt_at_index;
        temp->next = new_node_pt_at_index;
        return;
    } else {
        int curr_index = 0;
        node* temp = lst->head;
        while (curr_index != index - 1) {
            temp = temp->next;
            curr_index++;
        }
        new_node_pt_at_index->next = temp->next;
        temp->next = new_node_pt_at_index;
        return;
    }
}

// Deletes node at index (counting from head starting from 0).
// Note: index is guarenteed to be valid.
void delete_node_at(list *lst, int index) {
    
    if (is_empty_list(lst)) {
        printf("The list is empty");
        return;
    }

    if (lst->head == lst->head->next) {
        free(lst->head);
        lst->head = NULL;
        return;
    }

    if (!index) {
        node* front = lst->head;
        node* temp = lst->head;
        while (temp->next != lst->head) {
            temp = temp->next;
        }
        lst->head = lst->head->next;
        temp->next = lst->head;
        free(front);
        return;
    } else {
        int curr_index = 0;
        node* temp = lst->head;
        while (curr_index != index - 1) {
            temp = temp->next;
            curr_index++;
        }
        node* node_to_delete = temp->next;
        temp->next = temp->next->next;
        free(node_to_delete);
        return;
    }
    
}

// Rotates list by the given offset.
// Note: offset is guarenteed to be non-negative.
void rotate_list(list *lst, int offset) {
    node* temp = lst->head;
    while (offset > 0) {
        temp = temp->next;
        lst->head = temp;
        offset--;
    }
}

// Reverses the list, with the original "tail" node
// becoming the new head node.
void reverse_list(list *lst) {
    
    if (is_empty_list(lst)) {
        printf("list empty");
        return;
    }

    node *prev, *curr, *next;
    curr = lst->head;
    
    do {
        next = curr->next;
        curr->next = prev;
        prev = curr;
        curr = next;
    } while (curr != lst-> head);

    lst->head->next = prev;
    lst->head = prev;
}

// Resets list to an empty state (no nodes) and frees
// any allocated memory in the process
void reset_list(list *lst) {

    if (is_empty_list(lst)) {
        lst->head = NULL;
        return;
    }

    if (lst->head == lst->head->next) {
        free(lst->head);
        lst->head = NULL;
        return;
    }

    node* curr = lst->head;
    while (curr->next != lst->head) {
        node* prev = curr; 
        curr = curr->next;
        free(prev);
    }
    free(curr);
    lst->head = NULL;
    return;
}
