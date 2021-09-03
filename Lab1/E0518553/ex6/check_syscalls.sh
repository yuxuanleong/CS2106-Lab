#!/bin/bash
####################
# Lab 1 Exercise 6
# Name: Yang Shiyuan
# Student No: A0214269A
# Lab Group: B17
####################
echo "Printing system call report"
# Compile file
gcc -std=c99 pid_checker.c -o ex6
# Use strace to get report
strace -c ./ex6
