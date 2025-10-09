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
pid_t next_node;
int node_id;
struct Message *msg;
int PREV_READ_PIPE;
int NEXT_WRITE_PIPE;
char* buffer;

struct Message {
    int recipient_id;
    char content[MAX_MSG_LEN - sizeof(int)];
};

void shutdown(int sig) {
    if (sig == SIGINT && node_id == 0) {
        printf("\nNode %d (PID: %d) Shutting down...\n", node_id, getpid());
        kill(next_node, SIGUSR1);
        free(buffer);
    } else if (sig == SIGUSR1) {
        printf("Node %d (PID: %d) Shutting down...\n", node_id, getpid());
        if (next_node != 0) {
            kill(next_node, SIGUSR1);
        } 
    } else {
        return;
    }

    free(msg);
    close(PREV_READ_PIPE);
    close(NEXT_WRITE_PIPE);
    exit(0);
}

void msg_loop(const int PREV_READ_PIPE, const int NEXT_WRITE_PIPE) {
    msg = malloc(sizeof(struct Message));
    ssize_t msg_len;
    const pid_t pid = getpid();

    signal(SIGINT, shutdown);
    signal(SIGUSR1, shutdown);

    if (msg == NULL) {
        perror("Malloc failed!\n");
        return;
    }

    while (1)
    {
        msg_len = read(PREV_READ_PIPE, msg, MAX_MSG_LEN);
        if (msg_len == -1) {
            perror("Read failed, previous node dead or detached, exiting...\n");
            break;
        }
        
        if (msg_len < sizeof(int)) {
            printf("Node %d: Malformed or partial message, ignoring...\n", node_id);
            continue;
        }

        if (node_id == msg->recipient_id) {
            printf("Node %d (PID: %d) received: %s\n", node_id, pid, msg->content);
            msg->recipient_id = 0;
        }

        printf("Node %d (PID: %d) passing message (addressed to node %d).\n", node_id, pid, msg->recipient_id);
        
        write(NEXT_WRITE_PIPE, msg, MAX_MSG_LEN);
    };
};

void input_loop(const int PREV_READ_PIPE, const int NEXT_WRITE_PIPE, int k_nodes) {
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
    msg->recipient_id = 0;
    write(NEXT_WRITE_PIPE, msg, MAX_MSG_LEN);
    read(PREV_READ_PIPE, msg, MAX_MSG_LEN);
    
    while (1) {
        printf("Enter message as [ID] [MSG]:\n> ");
        if (fgets(buffer, MAX_MSG_LEN, stdin) == NULL) {
            printf("Failed buffer write. Exiting.\n");
            break;
        }

        switch (sscanf(buffer, "%d %[^\n]", &msg->recipient_id, msg->content))
        {
        case 0:
        case 1:
            printf("Please use the format [ID] [MSG]\n");
            printf("For example: '%d this is an example message'\n", k_nodes);
            continue;
        default:
            break;
        }
        
        if (msg->recipient_id < 1 || msg->recipient_id > k_nodes) {
            printf("ID should be value between 1 and %d\n", k_nodes);
            printf("For example: '%d %s'\n", k_nodes, msg->content);
            continue;
        }
        
        write(NEXT_WRITE_PIPE, msg, MAX_MSG_LEN);
        read(PREV_READ_PIPE, msg, MAX_MSG_LEN);
    }
}

void init_node(int k, int id, const int PREV_READ_PIPE, const int HEAD_WRITER) {
    int node_pipe[2];
    if (pipe(node_pipe) == -1) {
        perror("Init pipe failure!\n");
        exit(1);
    }

    node_id = id;

    printf("Node %d (PID: %d) initialized. Read pipe: %d, Write pipe: %d\n", id, getpid(), PREV_READ_PIPE, node_pipe[1]);

    if (k == id) {
        printf("Node %d (PID: %d) is the kth node, looping pipe to head: %d\n", id, getpid(), HEAD_WRITER);
        close(node_pipe[1]);
        msg_loop(PREV_READ_PIPE, HEAD_WRITER);
    } else {
        next_node = fork();

        if (next_node == -1) {
            perror("Fork failed!\n");
            exit(1);
        } else if (next_node == 0) {
            // Child recursively starts the next node
            init_node(k, id + 1, node_pipe[0], HEAD_WRITER);
        }
        

        if (node_id) {
            msg_loop(PREV_READ_PIPE, node_pipe[1]);
        } else { // node 0 handles user input
            input_loop(PREV_READ_PIPE, node_pipe[1], k);
        }
    }
};


void init_ring(int k) {
    int node_pipe[2];
    if (pipe(node_pipe) == -1) {
        perror("Init pipe failure!\n");
        exit(1);
    }
    const int HEAD_READER = node_pipe[0];
    const int HEAD_WRITER = node_pipe[1];

    init_node(k, 0, HEAD_READER, HEAD_WRITER);
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_nodes>\n", argv[0]);
        fprintf(stderr, "For example: '%s %d'\n", argv[0], MAX_NODES);
        exit(1);
    }

    const int nodes = atoi(argv[1]);
    if (nodes < 1 || nodes > MAX_NODES) {
        fprintf(stderr, "Number of nodes should be between 1 and %d\n", MAX_NODES);
        fprintf(stderr, "For example: '%s %d'\n", argv[0], MAX_NODES);
        exit(1);
    }

    init_ring(nodes);
}