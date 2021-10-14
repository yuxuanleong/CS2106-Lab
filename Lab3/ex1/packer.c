#include <semaphore.h>
#include <stdio.h>
#include "packer.h"

// You can declare global variables here
#define RGB 3
#define R 0
#define G 1
#define B 2
#define NULL_ID -1

// 1D Array that keeps track of the number of balls for the respective colour
int packingAreaBallCount[RGB] = {0, 0, 0};

// 2D Array that contains a pair of balls for the respective colour
int pairingSystem[RGB][2];

sem_t mutex[RGB];
sem_t queue[RGB];
sem_t turnstile[RGB];

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
}

// Algo -> Modified from Reuseable Barrier of "The Little Book of Semaphores - Green Tea Press"
int pack_ball(int colour, int id) {
    // Write your code here.
    colour--;

    // One by one, send in each colour all into packing area
    sem_wait(&mutex[colour]);
        // Pairing System to keep track of the Id
        pairingSystem[colour][packingAreaBallCount[colour]] = id;
        packingAreaBallCount[colour]++;

        // When there are 2 balls of a specific colour in packing area
        if (packingAreaBallCount[colour] == 2) {
            sem_wait(&turnstile[colour]); // close the exit gate of packing area
            sem_post(&queue[colour]); // open allocating gate to allocate partner
        }
    sem_post(&mutex[colour]);

    sem_wait(&queue[colour]); // check if ball can enter allocating area
    sem_post(&queue[colour]);

    // Critical Section: Allocating Area
    int pair = partner(colour, id);

    // One by one, send the ball off
    sem_wait(&mutex[colour]);
        packingAreaBallCount[colour]--;

        // When 0 ball in packing area
        if (packingAreaBallCount[colour] == 0) {
            sem_wait(&queue[colour]); // close gate of allocating area
            sem_post(&turnstile[colour]); // open the exit gate of packing area
        }
    sem_post(&mutex[colour]);

    sem_wait(&turnstile[colour]); // check can send the ball off the packing area
    sem_post(&turnstile[colour]);

    return pair;
}

// Allocating Area
int partner(int colour, int myID) {
    int partner = NULL_ID;
    for (int i = 0; i < 2; i++) {
        if (myID != pairingSystem[colour][i]) {
            partner = pairingSystem[colour][i];
        }
    }
    return partner;
}
