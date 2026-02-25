#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
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

    /*This struct sigaction sa is used to set up a signal handler
    for SIGINT to allow for shutdown of the program. 
    The handle_sigint function will be called when the SIGINT signal 
    is received, setting the shouldExit flag to tell the program that it
    should exit. This allows the program to clean up resources and exit 
    instead of abruptly terminating.*/
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);

    // no SA_RESTART flag so read can break with EINTR
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

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

        // Print the file descriptors for the read and write ends of the pipe for debugging purposes
        printf("Pipe %d created: read end = %d, write end = %d\n", i, ringPipes[i][0], ringPipes[i][1]);
    }

    // Array to hold the process IDs of the child processes (nodes)
    pid_t childPids[k];

    for (int i = 0; i < k; i++) {

        childPids[i] = -1;
    }

    // The parent starts as node 0
    int nodeID = 0; 

    for (int i = 1; i < k; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            return 1;
        }

        if (pid == 0) {

            // This is where the child process (node) starts
            // Child becomes node i
            nodeID = i;
            break;
        } else {

            // This is the parent process, which will continue to fork until all nodes are created
            childPids[i] = pid;
            printf("Node created %d with PID %d\n", i, pid);
        }
    }

    // Calculate the index of the previous node
    int inputIndex = (nodeID - 1 + k) % k; 

    // The current node's index is the output index
    int outputIndex = nodeID; 


    // This will read the end of the previous node's pipe for this node
    int inputReadFd = ringPipes[inputIndex][0];

    // This will write to the next node's pipe for this node
    int outputWriteFd = ringPipes[outputIndex][1];

    int nextNode = (nodeID + 1) % k;


    // Using this to see the node's input and output configuration for debugging
    printf("Node %d: input from node %d (fd %d), output to node %d (fd %d)\n", nodeID, inputIndex, inputReadFd, nextNode, outputWriteFd);

    for (int j = 0; j < k; j++) {

        // close read ends we dont use
        if (ringPipes[j][0] != inputReadFd) {
            close(ringPipes[j][0]);
        }

        // close write ends we dont use
        if (ringPipes[j][1] != outputWriteFd) {
            close(ringPipes[j][1]);
        }
    }


    if (nodeID == 0) {

        // This token will be used to pass the apple and messages around the ring
        Token token;

        // Start with the apple at node 0
        token.apple = 1;
        
        // No message initially
        token.isEmpty = 1; 

        // No destination initially
        token.destinationNode = -1;

        // No source initially
        token.sourceNode = -1;

        // Empty message text
        token.text[0] = '\0';

        printf("Node 0 sending initial token with apple to node %d\n", nextNode);

        // Send the initial token to the next node in the ring
        if (write(outputWriteFd, &token, sizeof(Token)) != sizeof(Token)) {
            perror("write failed");
            return 1;
        }
    }

    while (!shouldExit) {
        // This will hold the token read from the previous node
        Token token;

        // Read a token from the previous node's pipe
        ssize_t bytesRead = read(inputReadFd, &token, sizeof(Token));

        // If bytesRead is 0, it means the pipe has been closed by the previous node, so we should exit
        if (bytesRead == 0) {
            printf("Node %d: pipe closed, exiting\n", nodeID);
            break;
        }

        // Check if the read was successful
        if (bytesRead < 0) {
            if (errno == EINTR && shouldExit) {
                break;
            }
            perror("read failed");
            break;
        }

        printf("Node %d got apple, passing to node %d\n", nodeID, nextNode);
        

        // Pass the token to the next node in the ring
        ssize_t bytesWritten = write(outputWriteFd, &token, sizeof(Token));
        
        // Check if the write was successful
        if (bytesWritten != sizeof(Token)) {
            perror("write failed");
            break;
        }

    }


    /* This section is more of a cleaning up sections to make sure you know that the program is exiting after
    you ctrl c*/

    close(inputReadFd);
    close(outputWriteFd);
    
    if (nodeID == 0) {

        // Parent waits for children
        for (int i = 1; i < k; i++) {
            if (childPids[i] > 0) {
                waitpid(childPids[i], NULL, 0);
            }
        }
        printf("Parent exiting\n");
    } else {
        printf("Node %d exiting\n", nodeID);
    }

    return 0;

}
