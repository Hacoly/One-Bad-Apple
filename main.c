#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "structs.h"

volatile sig_atomic_t shouldExit = 0;

void handle_sigint(int sig) {
    // Unused parameter
    (void)sig; 

    // Set the flag to indicate that the program should exit
    shouldExit = 1;
}

int main() {

    // Buffer to hold user input for the number of nodes
    char inputBuffer[64];

    // Variable to store the number of nodes in the ring
    int k;

    printf("Enter the number of nodes (k): ");

    // Read the number of nodes from user input
    fflush(stdout);

    // fgets reads a line from stdin and stores it in inputBuffer, ensuring we don't exceed the buffer size
    if (fgets(inputBuffer, sizeof(inputBuffer), stdin) == NULL) {
        fprintf(stderr, "Error reading input.\n");
        return 1;
    }

    // Convert the input string to an integer
    k = atoi(inputBuffer);

    // Check if k is at least 2
    // A ring with fewer than 2 nodes doesn't make sense
    if (k < 2) {
        printf("k must be at least 2.\n");
        return 1;
    }

    // Print the number of nodes in the ring
    printf("Creating ring with %d nodes\n", k);

    // Array to hold the file descriptors for the pipes
    // Each node will have a pipe to the next node in the ring
    int ringPipes[k][2];
    

    // Create pipes for interprocess communication between nodes
    // Each node will have a pipe to the next node in the ring
    // The pipe() system call creates a unidirectional data channel that can be used for interprocess communication
    for (int i = 0; i < k; i++) {
        if (pipe(ringPipes[i]) == -1) {
            perror("pipe has failed");
            return 1;
        }
    }

    
}
