# include <stdio.h>
# include <stdlib.h>

struct queue_data
{
	int sock;
	int prio;
	int object_id;
	int start_idx;
	int size;	
};

struct Node
{
	struct queue_data data;
	struct Node *next;
};
struct Queue
{
	struct Node* head;
	struct Node* tail;
	
	int priority;
	int length;
};

void create_queue(struct Queue *queue, int priority);
int enqueue(struct Queue *queue, struct queue_data data);
int dequeue(struct Queue *queue, struct queue_data *data);
int req_purge(struct Queue *queue, int sock);

void print_queue(struct Queue *queue);
int destroy_queue(struct Queue *queue);
