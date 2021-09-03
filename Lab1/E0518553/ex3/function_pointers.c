/*************************************
* Lab 1 Exercise 3
* Name: Yang Shiyuan 
* Student No: A0214269A
* Lab Group: B17
*************************************/

#include "functions.h"

// Initialize the func_list array here!
int (*func_list[5])(int) = {
  add_one,
  add_two,
  multiply_five,
  square,
  cube  
};

// You can also use this function to help you with
// the initialization. This will be called in ex3.c.
void update_functions() {
    
}
