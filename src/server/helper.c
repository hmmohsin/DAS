# include <stdio.h>
# include <string.h>
# include "server.h"
# include <syslog.h>
# include <pthread.h>

int noise_on()
{
	int i =0;
	for (i=0; i<10; i++)
	{
		if(g_config.noise_vector[i] == g_config.server_id)
			return 1;		
	}
	return 0;
}
void close_request (int priority)
{
	pthread_mutex_lock(&QUEUE_LOCK[priority]);
	queue_len[priority]--;
	pthread_mutex_unlock(&QUEUE_LOCK[priority]);
		
}
void close_worker (int priority)
{
	pthread_mutex_lock(&COUNT_LOCK[priority]);
	curr_conn_count[priority]--;
	pthread_mutex_unlock(&COUNT_LOCK[priority]);

	if (DEBUG)
		syslog(LOG_INFO,"Info: closing worker\n");
}
void make_msg_hdr (char *hdr_buf, int type, int prio, int oid, int st_idx, int size)
{
	struct proto *msg_hdr = malloc(sizeof (struct proto));
	
	msg_hdr->type = type;
	msg_hdr->prio = prio;
	msg_hdr->object_id = oid;
	msg_hdr->start_idx = st_idx;
	msg_hdr->size = size;

	memcpy(hdr_buf, msg_hdr, sizeof(struct proto));
}
void parse_msg_hdr(struct proto *msg_hdr, char *hdr_buf)
{
	//add byte ordering support if needed
	memcpy(msg_hdr, hdr_buf, sizeof(struct proto));
}
void make_queue_obj(struct queue_data *q_data, int sock, int prio, int oid, int st_idx, int size)
{
	q_data->sock = sock;
	q_data->prio = prio;
	q_data->object_id = oid;
	q_data->start_idx = st_idx;
	q_data->size = size;
}
