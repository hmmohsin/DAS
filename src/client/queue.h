# include <stdio.h>
# include <stdlib.h>
# include <string.h>
struct qData
{
	int j;
	int k;
	int l;
	int m;
	int sock;
	int file_id;
	int prio_class;
};

struct Node
{
	void *data;
	struct Node *next;
};
struct Queue
{
	struct Node* head;
	struct Node* tail;
	
	int length;
};

void create_queue(struct Queue *queue);
int enqueue(struct Queue *queue, void *data, int size);
int dequeue(struct Queue *queue, void *data, int size);
void print_queue(struct Queue *queue);
int destroy_queue(struct Queue *queue);
