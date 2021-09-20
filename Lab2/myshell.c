/**
 * CS2106 AY21/22 Semester 1 - Lab 2
 *
 * This file contains function definitions. Your implementation should go in
 * this file.
 */

#define ERROR_FORK -1
#define INCOMPLETE -1
#define FORKING_ERROR -2
#define _POSIX_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include "myshell.h"
#include <stdio.h> 
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>   // stat
#include <stdbool.h>    // bool type

int child_PID_tracker[MAX_PROCESSES][2];
int new_child_PID_Index = 0;

/**
 * @brief 
 * Contructor
 */
void my_init(void) {
    // Initialize what you need here   
}

/**
 * @brief 
 * A method that checks the existence of a file 
 * @param filename 
 * @return true 
 * @return false 
 */
bool file_exists (char *filename) {
  struct stat buffer;   
  return (stat (filename, &buffer) == 0);
}

/**
 * @brief 
 * A method that checks the type of command input
 * @param num_tokens 
 * @param tokens 
 * @return int 
 * 0 -> Unrecognisable command
 * 1 -> info
 * 2 -> {program} (arg...)
 * 3 -> {program} (arg...) &
 * 4 -> wait {PID}
 */
int check_command(size_t num_tokens, char** tokens) {

    if (num_tokens == 2) {
        if (strcmp (tokens[0], "info") == 0) {
            return 1;
        }
    } else if (num_tokens > 2) {
        if (strcmp (tokens[0], "wait") == 0) {
            return 4;
        }
        if (file_exists (tokens[0])) {
            if (strcmp (tokens[num_tokens-2], "&") == 0){
                return 3;
            }
            return 2;
        }
    } else {
        return 0;
    }
}

/**
 * @brief 
 * Exercise 1a: {program} (args...)
 * @param num_tokens 
 * @param tokens 
 */
void ex1a_process(size_t num_tokens, char **tokens){
    child_PID_tracker[new_child_PID_Index][0] = fork();
    child_PID_tracker[new_child_PID_Index][1] = INCOMPLETE;
    if (child_PID_tracker[new_child_PID_Index][0] == 0) {
        int exe = execv(tokens[0], tokens);
        printf("This is the exit status %d in file %s\n", exe, tokens[0]);
        exit(FORKING_ERROR);
    } else {
        int status;
        waitpid(child_PID_tracker[new_child_PID_Index][0], &status, 0);
        child_PID_tracker[new_child_PID_Index][1] = WEXITSTATUS(status);
    }
    new_child_PID_Index++;
}

/**
 * @brief 
 * Exercise 1b: {program} (args...) &
 * @param num_tokens 
 * @param tokens 
 */
void ex1b_process(size_t num_tokens, char **tokens){
    child_PID_tracker[new_child_PID_Index][0] = fork();
    child_PID_tracker[new_child_PID_Index][1] = INCOMPLETE;
    if (child_PID_tracker[new_child_PID_Index][0] == 0) {
        tokens[num_tokens-2] = NULL;
        int exe = execv(tokens[0], tokens);
        printf("This is the exit status %d in file %s\n", exe, tokens[0]);
        exit(FORKING_ERROR);
    } else {
        printf("Child[%d] in background\n", child_PID_tracker[new_child_PID_Index][0]);
    }
    new_child_PID_Index++;
}

/**
 * @brief 
 * Exercise 1c: info
 * @param num_tokens 
 * @param tokens 
 */
void ex1c_show_info(size_t num_tokens, char **tokens) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (child_PID_tracker[i][0] != 0) {
            if (child_PID_tracker[i][1] == INCOMPLETE) {
                int status;
                int result = waitpid(child_PID_tracker[i][0], &status, WNOHANG);
                if (result == 0) {
                    printf("[%d] Running\n", child_PID_tracker[i][0]);
                } else {
                    child_PID_tracker[i][1] = WEXITSTATUS(status);
                    printf("[%d] Exited %d\n", child_PID_tracker[i][0], child_PID_tracker[i][1]);
                }
            } else {
                printf("[%d] Exited %d\n", child_PID_tracker[i][0], child_PID_tracker[i][1]);
            }
        } 
    }
}

/**
 * @brief 
 * Exercise 1d: Quit
 */
void my_quit(void) {
    // Clean up function, called after "quit" is entered as a user command
    printf("Goodbye!\n");
}

/**
 * @brief
 * Exercise 2a: wait {PID}
 * @param num_tokens
 * @param toeksn
 */
void ex2a_wait_PID(size_t num_tokens, char **tokens) {
    int targetedPID = atoi(tokens[1]);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (child_PID_tracker[i][0] == targetedPID && child_PID_tracker[i][1] == INCOMPLETE) {
            int status;
            int result = waitpid(targetedPID, &status, 0);
            child_PID_tracker[i][1] = WEXITSTATUS(status);
        }
    }
}

/**
 * @brief 
 * A method that controls the execution of a process
 * @param num_tokens 
 * @param tokens 
 */
void my_process_command(size_t num_tokens, char **tokens) {
    // Your code here, refer to the lab document for a description of the arguments
    
    int command_type = check_command (num_tokens, tokens);
    if (command_type == 1) {
        ex1c_show_info(num_tokens, tokens);
    }else if (command_type == 2) {
        ex1a_process(num_tokens, tokens);
    } else if (command_type == 3){
        ex1b_process(num_tokens, tokens);
    } else if (command_type == 4) {
        ex2a_wait_PID(num_tokens, tokens);
    } else {
        printf("%s not found\n", tokens[0]);
    }
}
