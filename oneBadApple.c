/**********************************************************
 *
 * oneBadApple.c
 * CIS 451 Project 1 (F25)
 *
 * Elijah Morgan & Emily Heyboer
 *************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>  // Mostly for pid_t
#include <signal.h>
#define MAX_MSG_LEN 1024
#define MSG_CONTENT_LEN MAX_MSG_LEN - 4
#define MAX_NODES 1000 // Lets not crash our PC lol.
/*NOTE - Message Format ==================================
byte  |     definition
----------------------------------------------------------
0-3   |     INT - Recipient Node ID
4-1024|     PTR - Message ptr
==========================================================*/
/*NOTE - Special Messages =================================
[ID] | C^   - Graceful shutdown of ring. Not implemented yet.
[ID] | PING - Ring health check, should reach HEAD. Not implemented yet, optional.
==========================================================*/
pid_t nextNode;
int nodeId;
struct Message *msg;
int PREV_READ_PIPE;
int NEXT_WRITE_PIPE;

struct Message {
    int recipientId;
    char content[MAX_MSG_LEN - sizeof(int)];
    //ssize_t length; // Does nothing rn, but putting this in for future use
};

void shutdown() {
    printf("Node %d (PID: %d) Shutting down...\n", nodeId, getpid());
    free(msg);
    close(PREV_READ_PIPE);
    close(NEXT_WRITE_PIPE);
    exit(0);
}

int msgLoop(const int PREV_READ_PIPE, const int NEXT_WRITE_PIPE) {
    struct Message *msg = malloc(sizeof(struct Message));
    ssize_t msgLen; //NOTE Remove this if we start using Message->length
    const pid_t pid = getpid();

    signal(SIGINT, shutdown);

    if (msg == NULL) {
        perror("Malloc failed!\n");
        return 1;
    }

    while (1)
    {
        msgLen = read(PREV_READ_PIPE, msg, MAX_MSG_LEN);
        if (msgLen == -1) {
            // TODO: Differentiate between dead nodes and gracefully shutting down the ring.
            perror("Read failed, previous node dead or detached, exiting...\n");
            shutdown();
        }
        
        if (msgLen < sizeof(int) || msg->recipientId < 1 || msg->recipientId > MAX_NODES) {
            printf("Node %d (PID: %d): Malformed or partial message, ignoring...\n", nodeId, pid);
            continue;
        }
        
        // Check if msg is for this node
        if (nodeId == msg->recipientId) {
            printf("Message at Node %d (PID: %d) received: %s\n", nodeId, pid, msg->content);
        } else {
            printf("Node %d (PID: %d) passing message to node %d.\n", nodeId, pid, msg->recipientId);
            // TODO: Forward message to next node in ring
            write(NEXT_WRITE_PIPE, msg, MAX_MSG_LEN);
        }
    };
};

void initNode(int k, int id, const int PREV_READ_PIPE, const int HEAD_WRITER) {
    // Init new pipe
    int nodePipe[2];
    if (pipe(nodePipe) == -1) {
        perror("Init pipe failure!\n");
        exit(1);
    }

    nodeId = id;

    printf("Node %d (PID: %d) initialized. Read pipe: %d, Write pipe: %d\n", id, getpid(), PREV_READ_PIPE, nodePipe[1]);
    // Check if this is the last node in the ring, N(k)
    if (k == id) {
        //printf("REMEMBER: read & write pipes should always be 3 apart. This is normal.\n");
        printf("Node %d (PID: %d) is the kth node, looping pipe to head: %d\n", id, getpid(), HEAD_WRITER);
        close(nodePipe[1]);
        msgLoop(PREV_READ_PIPE, HEAD_WRITER);
    // Else Create N(id + 1)
    } else {
        nextNode = fork();
        if (nextNode == -1) {
            perror("Fork failed!\n");
            exit(1);
        } else if (nextNode == 0) {
            // Child recursively starts as next node
            initNode(k, id + 1, nodePipe[0], HEAD_WRITER);
        }
        // Parent continues
        msgLoop(PREV_READ_PIPE, nodePipe[1]);
    }
    // Cleanup
    close(PREV_READ_PIPE);
    exit(0);
};


int initRing(int k) {
    // Init pipe
    int nodePipe[2];
    if (pipe(nodePipe) == -1) {
        perror("Init pipe failure!\n");
        exit(1);
    }
    const int HEAD_READER = nodePipe[0];
    const int HEAD_WRITER = nodePipe[1];
    
    nextNode = fork();
    if (nextNode == -1) {
        perror("Fork failed!\n");
        exit(1);
    } else if (nextNode == 0) {
        initNode(k, 1, HEAD_READER, HEAD_WRITER); // id = 1 since this is the 2nd Node, not 1st
    }
    // Parent continues
    return nodePipe[1];
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./oneBadApple <num_nodes>\n");
        exit(1);
    }
    // Convert string argument to integer for number of nodes
    const int nodes = atoi(argv[1]);
    if (nodes < 1 || nodes > MAX_NODES) {
        perror("Error: Number of nodes must be 1-1000\n");
        exit(1);
    }
    const int HEAD_PIPE = initRing(nodes);
    struct Message *msg = malloc(sizeof(struct Message));

    if (msg == NULL) {
        perror("Malloc failed!\n");
        exit(1);
    }

    char *buffer = malloc(sizeof(struct Message));

    if (buffer == NULL) {
        perror("Malloc failed!\n");
        exit(1);
    }

    signal(SIGINT, shutdown);
    
    while (1) {
        printf("Enter message as [ID][MSG]:\n>");
        if (fgets(buffer, MAX_MSG_LEN, stdin) == NULL) {
            printf("Failed buffer write. Exiting.\n");
            break;
        }

        switch (sscanf(buffer, "%d %[^\n]", &msg->recipientId, msg->content))
        {
        case 0:
            printf("Invalid message, use [ID][MSG]\n");
            continue;
        case 1:
            printf("Please include both [ID] & [MSG]\n");
            continue;
        default:
            break;
        }
        
        if (msg->recipientId < 1 || msg->recipientId > nodes) {
            printf("Bad ID value, please choose a value between 1-%d\n", nodes);
            continue;
        }
        
        write(HEAD_PIPE, msg, MAX_MSG_LEN);
    }
    free(msg);
    free(buffer);
    return 0;
}