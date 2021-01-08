#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<stdbool.h>
#include<netdb.h>
#include<sys/wait.h>
#include<sys/shm.h>

#include "config.h"
#include "performConnection.h"
#include "sharedMemory.h"
#include "sysprakclient.h"

#define GAMEIDLEN 14
#define CONFFILENAMELEN 128

// TODO: Errormessage when no command line arguments are passed
// TODO: General Error handling

int main(int argc, char*argv[]) {

    /* Initialize user parameters */
    char game_id[GAMEIDLEN] = {0};
    int player = -1;
    
    /* Configuration with default to client.conf,
       but will be overriden if put in command line 
       using the flag -c */
    char conf_file[CONFFILENAMELEN] = "client.conf";

    // TODO: do we want GAME_ID and PLAYER to move to the .conf file??

    /* Read Parameters from Commandline */
    int ret;
    while ((ret=getopt(argc, argv, "g:p:c:")) != -1) {
      switch (ret) {
        case 'g':
            strncpy(game_id, optarg, 13);
            break;
        case 'p':
            player = atoi(optarg);
            break;
        case 'c':
            memset(conf_file, 0, 128);
            strncpy(conf_file, optarg, strlen(optarg) + 1);
            break;
        default:
            break;
      }
    }

    /* Get configuration details */
    configuration game_conf = read_conf_file(conf_file);

    /* Connect to Game Server */
    int socket_file_descriptor;
    socket_file_descriptor = getSocketInfo(argc, argv, game_conf.hostname, game_conf.portnumber);

    /* Create Shared Memory */
    int shmid = create_shared_memory();

    /* Divide into Connector and Thinker */
    pid_t pid = fork();

    /* Connecter (Child Process) */
    if (pid == 0) {
        printf("Child process ->  PPID: %d, PID: %d\n", getppid(), getpid());

        /* Enter Prolog Phase */
        game_info our_info = serverConnect(socket_file_descriptor, game_id, player);

        printf("====> child sees game name: %s\n", our_info.game_name);

        game_info * shm_info;
        shm_info = (game_info*) shmat(shmid, NULL, 0);

        *shm_info = our_info;

    }
    /* Thinker (Parent Process) */
    else {
        printf("Parent process -> PID: %d\n", getpid());
        printf("Waiting for child processes to finish...\n");
        wait(NULL);
        printf("Child process finished.\n");

        game_info * parent_game_info;
        parent_game_info = (game_info*) shmat(shmid, NULL, 0);

        printf("====> parent sees game name: %s\n", parent_game_info->game_name);
    }

    exit(EXIT_SUCCESS);
}


/* This function connects the client to the server of LMU
   NOTE: to work you must be connected using either a VPN
   or an SSH Tunnel */
int getSocketInfo(int argc, char*argv[], char hostname[], char portnumber[]) {

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int socket_file_descriptor, s;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s host port msg...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Obtain address(es) matching host/port */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;           /* Any protocol */

    s = getaddrinfo(hostname, portnumber, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        socket_file_descriptor = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (socket_file_descriptor == -1)
            continue;

        if (connect(socket_file_descriptor, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(socket_file_descriptor);
    }

    freeaddrinfo(result);           /* No longer needed */

    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    return socket_file_descriptor;
}
