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
#define MAX_NODES 100 // Lets not crash our PC lol.
/*NOTE - Message Format ==================================
byte  |     definition
----------------------------------------------------------
0-3   |     INT - Recipient Node ID
4-1024|     PTR - Message ptr
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

void msgLoop(const int PREV_READ_PIPE, const int NEXT_WRITE_PIPE) {
    msg = malloc(sizeof(struct Message));
    ssize_t msgLen; //NOTE Remove this if we start using Message->length
    const pid_t pid = getpid();

    signal(SIGINT, shutdown);

    if (msg == NULL) {
        perror("Malloc failed!\n");
        shutdown();
    }

    while (1)
    {
        msgLen = read(PREV_READ_PIPE, msg, MAX_MSG_LEN);
        if (msgLen == -1) {
            perror("Read failed, previous node dead or detached, exiting...\n");
            shutdown();
        }
        
        if (msgLen < sizeof(int) || msg->recipientId < 0 || msg->recipientId > MAX_NODES) {
            printf("Node %d: Malformed or partial message, ignoring...\n", nodeId);
            continue;
        }
        
        // Check if msg is for this node
        if (nodeId == msg->recipientId) {
            printf("Node %d (PID: %d) received: %s\n", nodeId, pid, msg->content);
            msg->recipientId = 0;
        }

        // we always pass the message on to the next node (even if it was destined for this one)
        printf("Node %d (PID: %d) passing message (addressed to node %d).\n", nodeId, pid, msg->recipientId);
        
        write(NEXT_WRITE_PIPE, msg, MAX_MSG_LEN);
    };
};

void inputLoop(const int PREV_READ_PIPE, const int NEXT_WRITE_PIPE, int k_nodes) {
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

    // after initializing the node ring,
    // we send a message and wait for it to come back
    // so we know every node has been initialized
    printf("Node 0 (PID: %d) sending initial message\n", getpid());
    msg->recipientId = 0;
    write(NEXT_WRITE_PIPE, msg, MAX_MSG_LEN);
    read(PREV_READ_PIPE, msg, MAX_MSG_LEN);
    
    while (1) {
        printf("Enter message as [ID][MSG]:\n> ");
        if (fgets(buffer, MAX_MSG_LEN, stdin) == NULL) {
            printf("Failed buffer write. Exiting.\n");
            break;
        }

        switch (sscanf(buffer, "%d %[^\n]", &msg->recipientId, msg->content))
        {
        case 0:
            printf("Invalid message, use [ID] [MSG]\n");
            continue;
        case 1:
            printf("Please include both [ID] & [MSG]\n");
            continue;
        default:
            break;
        }
        
        if (msg->recipientId < 1 || msg->recipientId > k_nodes) {
            printf("Bad ID value, please choose a value between 1-%d\n", k_nodes);
            continue;
        }
        
        write(NEXT_WRITE_PIPE, msg, MAX_MSG_LEN);
        read(PREV_READ_PIPE, msg, MAX_MSG_LEN);
    }

    free(buffer);
}

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
        
        if (nodeId) {
            msgLoop(PREV_READ_PIPE, nodePipe[1]);
        } else { // node 0 handles user input
            inputLoop(PREV_READ_PIPE, nodePipe[1], k);
        }
    }
};


void initRing(int k) {
    // Init pipe
    int nodePipe[2];
    if (pipe(nodePipe) == -1) {
        perror("Init pipe failure!\n");
        exit(1);
    }
    const int HEAD_READER = nodePipe[0];
    const int HEAD_WRITER = nodePipe[1];

    initNode(k, 0, HEAD_READER, HEAD_WRITER);
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./oneBadApple <num_nodes>\n");
        exit(1);
    }

    // Convert string argument to integer for number of nodes
    const int nodes = atoi(argv[1]);
    if (nodes < 1 || nodes > MAX_NODES) {
        fprintf(stderr, "Error: Number of nodes must be 1-%d\n", MAX_NODES);
        exit(1);
    }

    initRing(nodes);
}