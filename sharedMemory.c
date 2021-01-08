#include<stdlib.h>
#include<stdio.h>
#include<sys/shm.h>

#include "sharedMemory.h"

/* creates a shared memory segment and returns the shmid */
int create_shared_memory() {
    // create a new shared memory segment the size of our data structure
    int shmid = shmget(IPC_PRIVATE, sizeof(game_info), IPC_CREAT | 0644);
    if (shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

// void struct_to_shared_memory(int shmid, game_info info) {
//     game_info * shm;
//     if ((shm = shmat(shmid, NULL, 0)) == (game_info*) -1) {
//         perror("shmat");
//     }

//     &info = shm;

// }