/*************************************
* Lab 1 Exercise 3
* Name: Yang Shiyuan
* Student No: A0214269A
* Lab Group: B17
*************************************/

#include "node.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Copy in your implementation of the functions from ex2.
// There is one extra function called map which you have to fill up too.
// Feel free to add any new functions as you deem fit.
node *get_node_pt (list *lst, int index) {
    
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
// Inserts a new node with data value at index (counting from head
// starting at 0).
// Note: index is guaranteed to be valid.
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
        //printf("list empty\n");
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


// Traverses list and applies func on data values of
// all elements in the list.
void map(list *lst, int (*func)(int)) {
    if (is_empty_list(lst)) {
        return;
    }

    node* temp = lst->head;
    while (temp->next != lst->head) {
        temp->data = func(temp->data);
        temp = temp->next;
    }
    temp->data = func(temp->data);
    return;
}

// Traverses list and returns the sum of the data values
// of every node in the list.
long sum_list(list *lst) {

    if (is_empty_list(lst)) {
        return 0;
    }

    long sum = 0;
    node* temp = lst->head;
    while (temp->next != lst->head) {
        sum += temp->data;
        temp = temp->next;
    }
    sum += temp->data;
    return sum;
}

// Prints out the whole list in a single line
void print_list(list *lst) {
    if (lst->head == NULL) {
        printf("[ ]\n");
        return;
    }

    printf("[ ");
    node *curr = lst->head;
    do {
        printf("%d ", curr->data);
        curr = curr->next;
    } while (curr != lst->head);
    printf("]\n");
}