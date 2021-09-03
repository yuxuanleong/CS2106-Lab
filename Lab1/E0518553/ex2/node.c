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
 * @brief Get the node pt object at a specific index
 * @param lst 
 * @param index 
 * @return node* 
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

/**
 * @brief Create a new node pt object with next pointer set as null
 * 
 * @param data 
 * @return node* 
 */
node *create_new_node_pt(int data) {

    node *new_node_pt = (node *) malloc(sizeof(node));
    new_node_pt->data = data;
    new_node_pt->next = NULL;
    return new_node_pt;
}

/**
 * @brief check if a list is currently empty
 * 
 * @param list_pt 
 * @return int 
 */
int is_empty_list(list *list_pt) {
    return list_pt->head == NULL;
}

/**
 * @brief remove a node at a specfic index
 * 
 * @param prev_pt 
 * @param curr_pt 
 */
void remove_node(node *prev_pt, node *curr_pt) {
    prev_pt->next = curr_pt->next;
    free(curr_pt);
}

/**
 * @brief insert a node after a specific index
 * 
 * @param this_node 
 * @param new_node 
 */
void insert_node_after(node *this_node, node *new_node) {

    new_node->next = this_node->next;
    this_node->next = this_node;
}

// Inserts a new node with data value at index (counting from head
// starting at 0).
// Note: index is guaranteed to be valid.

/**
 * @brief insert a node at a specific index
 * 
 * @param lst 
 * @param index 
 * @param data 
 */
void insert_node_at(list *lst, int index, int data) {

    // create the new node
    node *new_node_pt_at_index = create_new_node_pt(data);

    // if the list is empty, we initialize the list
    if (is_empty_list(lst)) {
        lst->head = new_node_pt_at_index;
        new_node_pt_at_index->next = lst->head;
        return;
    }

    // if we are going to insert at 0, else we insert at any index
    if (!index) {
        
        node* temp = lst->head;
        new_node_pt_at_index->next = temp;

        // iterate to end of the list
        while (temp->next != lst->head) {
            temp = temp->next;
        }

        // adjust the pointer at the end of list
        lst->head = new_node_pt_at_index;
        temp->next = new_node_pt_at_index;
        return;
    } else {

        int curr_index = 0;
        node* temp = lst->head;

        // iterate to end of the list
        while (curr_index != index - 1) {
            temp = temp->next;
            curr_index++;
        }

        // adjust the node
        new_node_pt_at_index->next = temp->next;
        temp->next = new_node_pt_at_index;
        
        return;
    }
}

// Deletes node at index (counting from head starting from 0).
// Note: index is guarenteed to be valid.
/**
 * @brief delete a node from the list
 * 
 * @param lst 
 * @param index 
 */
void delete_node_at(list *lst, int index) {
    
    // if the list is already empty, then we just prompt the user
    if (is_empty_list(lst)) {
        printf("The list is empty");
        return;
    }

    // if the node we are going to delete is the last node in the list
    if (lst->head == lst->head->next) {
        free(lst->head);
        lst->head = NULL;
        return;
    }

    // if we are going to remove a node from the start, else we remove a node from any index
    if (!index) {

        node* front = lst->head;
        node* temp = lst->head;
        
        // iterate to the end the list
        while (temp->next != lst->head) {
            temp = temp->next;
        }

        //  adjustment of the list
        lst->head = lst->head->next;
        temp->next = lst->head;
        //  free the front
        free(front);

        return;
    } else {

        int curr_index = 0;
        node* temp = lst->head;

        // find the node at the index
        while (curr_index != index - 1) {
            temp = temp->next;
            curr_index++;
        }

        // adjustment of the list
        node* node_to_delete = temp->next;
        temp->next = temp->next->next;

        // free the node
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
    
    // if list is empty, we just prompt the user
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

    // empty list
    if (is_empty_list(lst)) {
        lst->head = NULL;
        return;
    }

    // only one node in list
    if (lst->head == lst->head->next) {
        free(lst->head);
        lst->head = NULL;
        return;
    }

    // iteratively free all nodes
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
