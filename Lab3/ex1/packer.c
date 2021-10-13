#include <semaphore.h>
#include <stdio.h>
#include "packer.h"


// You can declare global variables here
#define RGB 3
#define R 0
#define G 1
#define B 2
#define NULL_ID -1

int packingAreaBallCount[RGB] = {0, 0, 0};
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

int pack_ball(int colour, int id) {
    // Write your code here.
    colour--;
    sem_wait(&mutex[colour]);
        pairingSystem[colour][packingAreaBallCount[colour]] = id;
        packingAreaBallCount[colour]++;
        if (packingAreaBallCount[colour] == 2) {
            sem_wait(&turnstile[colour]);
            sem_post(&queue[colour]);
        }
    sem_post(&mutex[colour]);

    sem_wait(&queue[colour]);
    sem_post(&queue[colour]);

    int pair = partner(colour, id);

    sem_wait(&mutex[colour]);
        packingAreaBallCount[colour]--;
        if (packingAreaBallCount[colour] == 0) {
            sem_wait(&queue[colour]);
            sem_post(&turnstile[colour]);
        }
    sem_post(&mutex[colour]);

    sem_wait(&turnstile[colour]);
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
