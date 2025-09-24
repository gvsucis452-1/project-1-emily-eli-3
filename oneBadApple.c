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
#include <string.h>
#include <stdint.h>
#include <stdbool.h> // Wish I knew about this before...

struct Node {
    uint64_t id;
    char *data;
    struct Node *next;
};

/*
// Forgot, this has to be a process, not a struct.
// Use the recursive idea tho.

// Recursively initialize a linked list of nodes. Last node points to N0.
struct Node* initNodeRing(struct Node *firstNode, uint64_t id, uint64_t maxId) {
    if (id > maxId) {
        return firstNode;
    }
    
    struct Node *newNode = malloc(sizeof(struct Node));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    newNode->id = id;
    newNode->data = NULL;
    newNode->next = initNode(id + 1, maxId);
    printf("Initialized node: %lu\n", id);
    return newNode;
}

// Freeing memory works in the same recursive way.
void freeNodeRing(struct Node *head) {
    struct Node *current = head;
    struct Node *nextNode;

    if (head == NULL) return;

    do {
        nextNode = current->next;
        free(current);
        current = nextNode;
    } while (current != head);
}
*/

int main(int argc, char *argv[]) {
    int64_t k = argv[1];

    // Manually initialize the HEAD node.
    struct Node *head = malloc(sizeof(struct Node));
    if (head == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    head->id = 0;
    // This may take a while...
    head->next = initNodeRing(head, 1, k);


    return 0;
}