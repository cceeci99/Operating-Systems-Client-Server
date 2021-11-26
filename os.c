#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>

#include <time.h>

#define LINE_SIZE 100
#define PERMS 0666


typedef struct shared_memory{
    int random_line_number;             // the number of the line which will be requested
    char requested_line[LINE_SIZE];     // the line itself which will be returnet

    // 3 unnamed semaphores as part of the shared memory
    sem_t client_to_other_clients;
    sem_t client_to_server;
    sem_t server_to_client;

} shared_memory;


int main(int argc, char* argv[]){

    // get line arguments
    char* file_name = argv[1];
    int number_of_childs = atoi(argv[2]);   // K
    int number_of_requests = atoi(argv[3]); // N

    // A File with name file_name is given by the command line arguments, open it and count it's lines //
    FILE *file = fopen(file_name, "r");

    if ( file == NULL ){
        printf("Could not open file with name: '%s' \n", file_name);
        exit(EXIT_FAILURE);
    }

    int number_of_lines = 0;
    char line[LINE_SIZE];
    while ( fgets(line, LINE_SIZE, file) != NULL ){
        number_of_lines++;
    }
    fclose(file);


     // create the shared memory. Since it's done before fork() both parent and child will have access
     // to the same shared memory, as so to the number_of_lines and number_of_requests (they don't need to be passed to the childs)
    int segment_id;
    shared_memory* segment;

    segment_id = shmget(IPC_PRIVATE, sizeof(shared_memory), IPC_CREAT | PERMS);
    if ( segment_id == -1 ){
        perror("Shared Memory Create");
        exit(EXIT_FAILURE);
    }

    segment = shmat(segment_id, NULL, 0);   // attach to shared memory
    if ( segment == (void *) -1 ){
        perror("Shared Memory Attach");
        exit(EXIT_FAILURE);
    }

    if ( sem_init(&(segment->client_to_other_clients), 1, 1) == -1 ){
        perror("Initialize Semaphore");
        exit(EXIT_FAILURE);
    }

    if ( sem_init(&segment->client_to_server, 1, 0) == -1 ){
        perror("Initialize Semaphore");
        exit(EXIT_FAILURE);
    }

    if ( sem_init(&segment->server_to_client, 1, 0) == -1 ){
        perror("Initialize Semaphore");
        exit(EXIT_FAILURE);
    }

    // create how many childs you want (number_of_childs = K)
    int pids[number_of_childs];

    for ( int i=0; i < number_of_childs; i++ ){
        pids[i] = fork();

        if ( pids[i] == -1 ){
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if ( pids[i] == 0 ){  // CHILD PROCESS aka CLIENT
            srand(time(NULL)+getpid()); // because childs are created at the same time the seed will be same for all of them

            usleep(2);  // sleep for 2miliseconds so that we can randomize which child will take over

            int j=0;

            while ( j < number_of_requests ){

                sem_wait(&segment->client_to_other_clients);

                sem_post(&segment->client_to_server);

                fprintf(stdout,"Child %d  with ID %d requesting: \n", i+1, getpid());
                fflush(stdout);

                // client asks from server to give him a random line from the file
                segment->random_line_number = rand() % number_of_lines + 1;
                fprintf(stdout,"Deliver me line %d\n", segment->random_line_number);
                fflush(stdout);

                sem_post(&segment->server_to_client);           // unblock server to take the request

                sem_wait(&segment->client_to_server);           // block current client
                sem_wait(&segment->client_to_server);

                fprintf(stdout,"Child %d with ID %d Printing line: %s \n", i+1, getpid(), segment->requested_line);
                fflush(stdout);


                sem_post(&segment->client_to_other_clients);    // unblock
                
                j++;

                usleep(2);
            }
            return 0;       // child must return and not continue the loop, in that case 2^K childs will be created
        }
    }


     // here executes only the parent process
    int transactions = 0;
    while ( transactions < number_of_childs*number_of_requests ){
        sem_wait(&segment->server_to_client);   // block server till it get's request

        file = fopen(file_name, "r");
        if ( file == NULL ){
            printf("Could not open file %s", file_name);
            exit(EXIT_FAILURE);
        }

        int count = 1;
        char line[100];

        while ( fgets(line, LINE_SIZE, file) != NULL ){
            if ( count == segment->random_line_number ){
                fprintf(stdout,"DELIVERING LINE ->\n");
                fflush(stdout);
                strcpy(segment->requested_line, line);
                break;
            }
            else{
                count++;
            }
        }
        fclose(file);

        transactions++;
        sem_post(&segment->client_to_server);   // unblock client to get the response
    }

     // parent waits till all child terminate then detach and deallocate the memory and semaphores.
    for ( int i=0; i<number_of_childs; i++ ){
        wait(NULL);
    }

     // delete semaphores and shared memory

    if ( sem_destroy(&segment->client_to_other_clients) == -1 ){
        perror("Semaphore destroy");
        exit(EXIT_FAILURE);
    }

    if ( sem_destroy(&segment->client_to_server) == -1 ){
        perror("Semaphore destroy");
        exit(EXIT_FAILURE);
    }

    if ( sem_destroy(&segment->server_to_client) == -1 ){
        perror("Semaphore destroy");
        exit(EXIT_FAILURE);
    }

     if ( shmdt((void *) segment) == -1 ){
        perror("Shared Memory Detach");
        exit(EXIT_FAILURE);
    }

    if ( shmctl(segment_id, IPC_RMID, 0) == -1 ){
        perror("Shared Memory Remove");
        exit(EXIT_FAILURE);
    }

    return 0;
}