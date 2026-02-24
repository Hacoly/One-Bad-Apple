#ifndef STRUCTS_H
#define STRUCTS_H

#define MAX_MESSAGES 256

typedef struct {

    // 1 if apple is present
    int apple;

    // destination node id (-1 if none)
    int destinationNode;

    // Source node id (-1 if none)
    int sourceNode;

    // This is 1 if no message inside
    int isEmpty;
    
    // message content
    char text[MAX_MESSAGES]; 
} Token;

#endif