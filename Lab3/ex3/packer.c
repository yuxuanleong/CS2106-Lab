#include <semaphore.h>
#include <stdio.h>
#include "packer.h"

// You can declare global variables here
#define RGB 3
#define R 0
#define G 1
#define B 2
#define NULL_ID -1
#define MAX_N 64

// Package Size [Custom]
int packageN;
// 1D Array to keep track no of ball for the respective colour
int packingAreaBallCount[RGB] = {0, 0, 0};
// 2D Array to track all balls in the same package
int pairingSystem[RGB][MAX_N];

sem_t mutex[RGB];
sem_t queue[RGB];
sem_t turnstile[RGB];
sem_t fullPackingArea[RGB];

void packer_init(int balls_per_pack) {
    // Write initialization code here (called once at the start of the program).
    packageN = balls_per_pack;
    sem_init(&mutex[R], 0, 1);
    sem_init(&mutex[G], 0, 1);
    sem_init(&mutex[B], 0, 1);
    sem_init(&queue[R], 0, 0);
    sem_init(&queue[G], 0, 0);
    sem_init(&queue[B], 0, 0);
    sem_init(&turnstile[R], 0, 1);
    sem_init(&turnstile[G], 0, 1);
    sem_init(&turnstile[B], 0, 1);
    sem_init(&fullPackingArea[R], 0, packageN);
    sem_init(&fullPackingArea[G], 0, packageN);
    sem_init(&fullPackingArea[B], 0, packageN);
}

void packer_destroy(void) {
    // Write deinitialization code here (called once at the end of the program).
    sem_destroy(&mutex[R]);
    sem_destroy(&mutex[G]);
    sem_destroy(&mutex[B]);
    sem_destroy(&queue[R]);
    sem_destroy(&queue[G]);
    sem_destroy(&queue[B]);
    sem_destroy(&turnstile[R]);
    sem_destroy(&turnstile[G]);
    sem_destroy(&turnstile[B]);
    sem_destroy(&fullPackingArea[R]);
    sem_destroy(&fullPackingArea[G]);
    sem_destroy(&fullPackingArea[B]);
}

//  To provide the IDs of all other balls in the package
void packaging(int colour, int id, int* other_ids) {
    for (int i = 0; i < packageN; i++) {
        if (pairingSystem[colour][i] != id) {
            *other_ids = pairingSystem[colour][i];
            other_ids++;
        }
    }
}

// Modify from the Little Book Of Semaphore
void pack_ball(int colour, int id, int *other_ids) {
    // Write your code here.
    // Remember to populate the array `other_ids` with the (balls_per_pack-1) other balls.
    
    colour--;

    // wait outside, when packing area is full
    sem_wait(&fullPackingArea[colour]);

    // one by one enter the packing area
    sem_wait(&mutex[colour]);
        // track ID of the ball that enters the packing area
        pairingSystem[colour][packingAreaBallCount[colour]] = id;
        // track the number of balls in the packing area
        packingAreaBallCount[colour]++;

        // When balls in packing area reaches the limit 
        if (packingAreaBallCount[colour] == packageN) {
            // packing area is now full
            sem_wait(&turnstile[colour]); // close exit gate
            sem_post(&queue[colour]); // open gate for allocating area
        }
    sem_post(&mutex[colour]);

    // check if the allocating area's gate is opened
    sem_wait(&queue[colour]);
    sem_post(&queue[colour]);

    // Critical Section: Allocation of otherIDs
    packaging(colour, id, other_ids);

    // Remove ball in packing area one by one
    sem_wait(&mutex[colour]);
        packingAreaBallCount[colour]--;
        if (packingAreaBallCount[colour] == 0) {
            // packing area is now empty

            // open the gate of packing area
            for (int i = 0; i < packageN; i++) {
                sem_post(&fullPackingArea[colour]);
            }
            sem_wait(&queue[colour]); // close the gate allocating area
            sem_post(&turnstile[colour]); // open the gate of exit area
        }
    sem_post(&mutex[colour]);

    // check if the exit gate is opened
    sem_wait(&turnstile[colour]);
    sem_post(&turnstile[colour]);
}