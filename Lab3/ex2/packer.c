#include <semaphore.h>
#include <stdio.h>
#include "packer.h"


// You can declare global variables here
#define RGB 3
#define R 0
#define G 1
#define B 2
#define NULL_ID -1

// 1D Array to track number of balls for each colour
int packingAreaBallCount[RGB] = {0, 0, 0};
// 2D Array to track partner's ID
int pairingSystem[RGB][2];

sem_t mutex[RGB];
sem_t queue[RGB];
sem_t turnstile[RGB];
sem_t fullPackingArea[RGB];

void packer_init(void) {
    // Write initialization code here (called once at the start of the program).
    sem_init(&mutex[R], 0, 1);
    sem_init(&mutex[G], 0, 1);
    sem_init(&mutex[B], 0, 1);
    sem_init(&queue[R], 0, 0);
    sem_init(&queue[G], 0, 0);
    sem_init(&queue[B], 0, 0);
    sem_init(&turnstile[R], 0, 1);
    sem_init(&turnstile[G], 0, 1);
    sem_init(&turnstile[B], 0, 1);
    sem_init(&fullPackingArea[R], 0, 2);
    sem_init(&fullPackingArea[G], 0, 2);
    sem_init(&fullPackingArea[B], 0, 2);
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


// Algo -> Modified from Reuseable Barrier of "The Little Book of Semaphores - Green Tea Press"
int pack_ball(int colour, int id) {
    // Write your code here.

    colour--;

    // wait outside, when packing area is full
    sem_wait(&fullPackingArea[colour]);

    // one by one enter packing area
    sem_wait(&mutex[colour]);
        
        // tracking IDs
        pairingSystem[colour][packingAreaBallCount[colour]] = id;
        // tracking no of balls
        packingAreaBallCount[colour]++;

        // when there are 2 balls
        if (packingAreaBallCount[colour] == 2) {
            sem_wait(&turnstile[colour]); // close the exit gate
            sem_post(&queue[colour]); // open allocating area gate
        }
    sem_post(&mutex[colour]);

    sem_wait(&queue[colour]); // check if can enter allocating area
    sem_post(&queue[colour]);

    // Critical Section: Allocating Area
    int pair = partner(colour, id);

    // One by one, send the ball off
    sem_wait(&mutex[colour]);
        packingAreaBallCount[colour]--;

        // When 0 ball in packing area
        if (packingAreaBallCount[colour] == 0) {
            
            // Open the packing area gate to receive new balls
            sem_post(&fullPackingArea[colour]);
            sem_post(&fullPackingArea[colour]);

            sem_wait(&queue[colour]); // close allocating area gate
            sem_post(&turnstile[colour]); // open exit gate
        }
    sem_post(&mutex[colour]);

    sem_wait(&turnstile[colour]); // check if the exit gate is opened
    sem_post(&turnstile[colour]);

    return pair;
}

int partner(int colour, int myID) {
    int partner = NULL_ID;
    for (int i = 0; i < 2; i++) {
        if (myID != pairingSystem[colour][i]) {
            partner = pairingSystem[colour][i];
        }
    }
    return partner;
}
