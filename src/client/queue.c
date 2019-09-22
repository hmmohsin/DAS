# include "queue.h"

void create_queue(struct Queue *queue)
{
	queue->head = NULL;
	queue->tail = NULL;	

	queue->length = 0;
}

int enqueue(struct Queue *queue, void *data, int size)
{
	struct Node *tmp = malloc(sizeof(struct Node));
	tmp->data = malloc(size);

	memcpy(tmp->data, data, size);

	if(queue->head == NULL) {

		queue->head = tmp;
		queue->tail = tmp;
		queue->length += 1;
	}
	else{
		queue->tail->next = tmp;
		queue->tail = tmp;
		queue->length += 1;

	} 
	return queue->length;
}

int dequeue(struct Queue *queue, void *data, int size)
{
	if (queue->length == 0)
		return -1;
	
	struct Node* tmp;

	if(queue->length == 1) {
			
		/*data->sock = queue->head->data.sock;
		data->file_id = queue->head->data.file_id;
		data->prio_class = queue->head->data.prio_class;*/
	
		memcpy(data, queue->head->data, size);
		
		tmp = queue->head;
		queue->head = NULL;
		queue->tail = NULL;
		tmp->next = NULL;
		queue->length = 0;
	}
	else{
		
		memcpy(data, queue->head->data, size);
                /*data->sock = queue->head->data.sock;
                data->file_id = queue->head->data.file_id;
                data->prio_class = queue->head->data.prio_class;*/
		
		tmp = queue->head;
		queue->head = tmp->next;
		
		queue->length -= 1;
	}
	free(tmp);
	return queue->length;
}

void print_queue(struct Queue *queue)
{
	int index = 0;
	struct Node *curr = queue->head;
	if (curr == NULL)
		printf("Info: Queue is empty\n");
	while(curr != NULL)
	{
		struct qData *data = (struct qData*)curr;
		printf("%d: %d %d %d\n", index, 
				data->sock,
				data->file_id,
				data->prio_class);
		curr = curr->next;
	}
}

int destroy_queue(struct Queue *queue)
{
	printf("Not implemented as of yet\n");
	return 0;
}	
