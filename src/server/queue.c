# include "queue.h"

void create_queue(struct Queue *queue, int priority)
{
	queue->head = NULL;
	queue->tail = NULL;	

	queue->priority = priority;
	queue->length = 0;
}

int enqueue(struct Queue *queue, struct queue_data data)
{
	struct Node *tmp = malloc(sizeof(struct Node));
	
	tmp->data.sock = data.sock;
	tmp->data.prio = data.prio;
	tmp->data.object_id = data.object_id;
	tmp->data.start_idx = data.start_idx;
	tmp->data.size = data.size;
	tmp->next = NULL;
	
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

int dequeue(struct Queue *queue, struct queue_data *data)
{
	if (queue->length == 0){
		return -1;
	}

	struct Node* tmp;

	if(queue->length == 1) {
		data->sock = queue->head->data.sock;
		data->prio = queue->head->data.prio;
		data->object_id = queue->head->data.object_id;
		data->start_idx = queue->head->data.start_idx;
		data->size = queue->head->data.size;
		
		tmp = queue->head;
		queue->head = NULL;
		queue->tail = NULL;
		tmp->next = NULL;
		queue->length = 0;
	}
	else{
		data->sock = queue->head->data.sock;
		data->prio = queue->head->data.prio;
		data->object_id = queue->head->data.object_id;
		data->start_idx = queue->head->data.start_idx;
		data->size = queue->head->data.size;

		tmp = queue->head;
		queue->head = tmp->next;
		tmp->next = NULL;		
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
		fprintf(stderr, "%d: %d %d %d\n", index, 
				curr->data.sock,
				curr->data.object_id,
				curr->data.prio);
		curr = curr->next;
	}
}
int req_purge(struct Queue *queue, int sock)
{
	struct Node *curr = queue->head;
	struct Node *prev = curr;

	if (curr == NULL)
		return -1;

	while(curr != NULL){
		if(curr->data.sock == sock){

			if (curr == queue->head){
				if (queue->length == 1){
					queue->head = NULL;
					queue->tail = NULL;
				}
				else{
					queue->head = curr->next;
				}
			}
			else if(curr == queue->tail){
				prev->next = NULL;
				queue->tail = prev;

			}
			else{
				prev->next = curr->next;
			}
			
			curr->next = NULL;
			queue->length -=1;
			free(curr);
			
			return queue->length;	
		}

		prev = curr;
		curr = curr->next;
	}
	
	return -1;	

}
int destroy_queue(struct Queue *queue)
{
	printf("Not implemented as of yet\n");
	return 0;
}
	
