#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <math.h>
#include <stddef.h>
#include <pthread.h>
#include <errno.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <semaphore.h>

# include "uprio.h"
# include "server.h"

int dans_send(int sock, char* msg, int msgsize)
{
	int bytes_sent = 0, total_bytes_sent = 0, bytes_to_send=0;
	bytes_to_send = msgsize;
	do{
		bytes_sent = write(sock, msg, bytes_to_send);
		
		if (bytes_sent < 0)
			return -1;
			
		bytes_to_send -= bytes_sent;
		
		total_bytes_sent += bytes_sent;
		msg = msg+bytes_sent;
		
	}while(total_bytes_sent < msgsize);

	return total_bytes_sent;
}

int dans_recv(int sock, char* msg, int msgsize)
{
	int bytes_recvd = 0, total_bytes_recvd = 0, bytes_to_recv=0;
	bytes_to_recv = msgsize;
	do{
		bytes_recvd = read(sock, msg, bytes_to_recv);
		if (bytes_recvd < 0)
			return -1;
			
		bytes_to_recv -= bytes_recvd;
		
		total_bytes_recvd += bytes_recvd;
		msg = msg+bytes_recvd;
		
	}while(total_bytes_recvd < msgsize);

	return total_bytes_recvd;
}


int dans_accept(int listen_sock)
{
	int com_sock;
	struct sockaddr_in cli_addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	
	com_sock = accept(listen_sock, (struct sockaddr *)&cli_addr, &addr_len);
	if (com_sock == -1){
		accept_count_err++;
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK)){
			
        		if (DEBUG)
				syslog(LOG_INFO, "Error: dans_accept:"
						" No pending request.\n");
			return -1;
		}
		else{
			
        		if (DEBUG)
				syslog(LOG_INFO, "Error: dans_accept: "
					"Failed to accept new connection\n");
			return -1;
		}
	}
	else if (DEBUG)
		syslog(LOG_INFO, "Info: accepted new connection\n");
	
	return com_sock;
}
int make_non_blocking (int new_sock)
{
	int flags, status;
	flags = fcntl(new_sock, F_GETFL, 0);
	if (flags == -1){
		if (DEBUG)	
			syslog(LOG_INFO, "Error: make_non_blocking: "
				"Failed to get socket options\n");
		return 0;
	}

	flags |= O_NONBLOCK;
	status = fcntl(new_sock, F_SETFL, flags);
	if (status == -1){
		if (DEBUG)	
			syslog(LOG_INFO, "Error: make_non_blocking: "
				"Failed to set socket options\n");
		return 0;
	}
	return 1;	

}
int make_blocking (int socket)
{
	int flags, status;
	flags = fcntl(socket, F_GETFL, 0);
	if (flags == -1){
		if (DEBUG)	
                	syslog(LOG_INFO, "Error: make_blocking: "
				"Failed to get socket options\n");
                return 0;
        }

        flags = flags & (~O_NONBLOCK);
        status = fcntl(socket, F_SETFL, flags);
        if (status == -1){
		if (DEBUG)	
                	syslog(LOG_INFO, "Error: make_blocking: "
				"Failed to set socket options\n");
                return 0;
        }
        return 1;
}
int dans_com_handler(int com_sock, int priority)
{
	int bytes_read, queued; 		
	int filesize=10000000, ret_status=0;
        int bytes_to_read = MSGSIZE;

	char *msg_buf=malloc(MSGSIZE);
	char object_path[100];
	
	struct proto *msg_hdr = malloc(MSGSIZE);
	struct queue_data q_data;
	struct stat filestats;


	bytes_read = read(com_sock, msg_buf, bytes_to_read);


	if (bytes_read == 0){
		if (DEBUG)	
			syslog(LOG_INFO,"Com_Handler: Client closed connection"
				" on socket %d\n", com_sock);
		return ST_ERR_CONN_CLOSED;
	}
	else if(bytes_read == -1){
		if (DEBUG)	
			syslog(LOG_INFO,"Com_Handler: Error occurred %s. (%d,%d,-)\n",
						strerror(errno), com_sock, priority);
		return ST_ERR_COM_FAILURE;
	}
	else{

		parse_msg_hdr(msg_hdr, msg_buf);	

		if (msg_hdr->type == REQUEST_GETFILE){
			if(DEBUG)
				syslog(LOG_INFO,"Com_Handler: Info: GetFile request recvd."
					" Read %d bytes. (%d,%d,%d)\n", bytes_to_read, 
						com_sock, priority, msg_hdr->object_id);
			
			if(strcmp(g_config.sched_mode, "hdfs") == 0){
				ret_status = handle_hdfs_get(com_sock, msg_hdr, priority);
			}
			else if(strcmp(g_config.sched_mode, "memory") == 0){
				
				filesize = 5000000;
				//filesize = get_mem_file_size();
				make_queue_obj(&q_data, com_sock, msg_hdr->prio,
						msg_hdr->object_id, msg_hdr->start_idx,
									filesize);
				if (DEBUG)
					syslog(LOG_INFO,"Com_Handler: Queuing request" 
						"(%d,%d,%d)\n",com_sock, priority, 
								msg_hdr->object_id);

				queued = enqueue_data(q_data, priority);
	
				if (queued == -1)
					ret_status = handle_enqueue_failure(com_sock, 
									msg_hdr, priority);
				else
					ret_status = handle_enqueue_success(com_sock, 
									msg_hdr, filesize);
			
			}
			else if(strcmp(g_config.sched_mode, "disk") == 0){
		
				sprintf(object_path,"%s/F%d",g_config.data_dir, 
							msg_hdr->object_id);	
				if(stat(object_path,&filestats) < 0) {
				
					handle_not_found(com_sock, msg_hdr);
				}
			
				else{
					filesize = filestats.st_size;
					make_queue_obj(&q_data, com_sock, msg_hdr->prio,
						msg_hdr->object_id, msg_hdr->start_idx,
									filesize);
				
					if (DEBUG)
						syslog(LOG_INFO,"Com_Handler: Queuing req" 
							"(%d,%d,%d)\n",com_sock, priority, 
									msg_hdr->object_id);

					queued = enqueue_data(q_data, priority);
	
					if (queued == -1)
						ret_status = handle_enqueue_failure(com_sock
								, msg_hdr, priority);
					else
						ret_status = handle_enqueue_success(com_sock
								, msg_hdr, filesize);
				}
			}
		}
		
		/*else if (msg_hdr->type == REQUEST_PURGE){

			syslog(LOG_INFO,"Com_Handler: Info: Purge request received. "
				"(%d,%d,%d)\n", com_sock, priority, msg_hdr->object_id);

			purg_rv = in_queue_purge(priority, com_sock, msg_hdr->object_id);
				
			if (purg_rv == -1)
				syslog(LOG_INFO,"Com_Handler: Error: in_queue purge Failed. "
					"(%d,%d,%d)\n", com_sock, priority, msg_hdr->object_id);
			else{
				syslog(LOG_INFO,"Com_Handler: Info: Request purged from queue. "
					"(%d,%d,%d)\n", com_sock, priority, msg_hdr->object_id);
				close_request(priority);
			}
			ret_status = ST_PURGE_RECVD;
		}*/
		else if (DEBUG){
			syslog(LOG_INFO,"Com_Handler: Warning: Other request received. "
				"(%d,%d,%d)\n",com_sock, priority, msg_hdr->object_id);
		}
	}
	free(msg_hdr);
	free(msg_buf);
	return ret_status;
	
}
int handle_hdfs_get(int com_sock, struct proto *msg_hdr, int priority){

	struct queue_data q_data;
	int queued;
	int ret_status = ST_ACCEPT;
	
	make_queue_obj(&q_data, com_sock, msg_hdr->prio,
			msg_hdr->object_id, msg_hdr->start_idx,0);

	if (DEBUG)
		syslog(LOG_INFO,"Handle_HDFS: Queuing GetFile Req "
			"(%d,%d,%d)\n", com_sock, msg_hdr->prio,
					msg_hdr->object_id);

	queued = enqueue_data(q_data, priority);
	
	if (queued == -1){
		handle_enqueue_failure(com_sock, msg_hdr, priority);
	}
	else{
		if (DEBUG)
			syslog(LOG_INFO,"Handle_HDFS: Request queued"
				"(%d,%d,%d)\n", com_sock, msg_hdr->prio,
						msg_hdr->object_id);

		sem_post(&job_in_sys_mutex);
	}
	return ret_status;
}
int handle_not_found(int com_sock, struct proto *msg_hdr){
	
	int ret_status = ST_ERR_FILE_NOT_FOUND;
	int bytes_written = 0;

	char *msg_buf = malloc(MSGSIZE);	

	if (DEBUG)
		syslog(LOG_INFO, "Not_Found: Error: Filing error %d. %s\n",
						errno, strerror(errno));
		
	make_msg_hdr(msg_buf, REQUEST_REJECT, msg_hdr->prio, msg_hdr->object_id,
							msg_hdr->start_idx, 0);

	bytes_written = dans_send(com_sock, msg_buf, MSGSIZE);
				
	if (bytes_written < 0){
		ret_status = ST_ERR_FILE_NOT_FOUND;
		if (DEBUG)			
			syslog(LOG_INFO,"Com_Handler: Error: com failure occured"
			"(%d,%d,%d)\n", com_sock, msg_hdr->prio,msg_hdr->object_id);

	}
	return ret_status;

}
int handle_enqueue_failure(int com_sock, struct proto *msg_hdr, int priority){
	
	struct epoll_event event;
	int type = REQUEST_REJECT;
	int ret_status = ST_ERR_QUEUE_OVERFLOW;
	int bytes_written = 0;
	char *msg_buf = malloc(MSGSIZE);

	if (epoll_ctl(epoll_fd[priority], EPOLL_CTL_DEL, com_sock, &event) != 0){
		
		if (DEBUG)
			syslog(LOG_INFO,"Enqueue_Failure: EPOLL_CTL_DEL Failed."
			"(%d,%d,%d)\n", com_sock, msg_hdr->prio,msg_hdr->object_id);
		
	}
	else{
			        
		if (DEBUG)
			syslog(LOG_INFO,"Enqeue_Failure: Queue overflow."
			"(%d,%d,%d)\n", com_sock, msg_hdr->prio,msg_hdr->object_id);

		make_msg_hdr(msg_buf, type, msg_hdr->prio, msg_hdr->object_id, 
							msg_hdr->start_idx, 0);

		bytes_written = dans_send(com_sock, msg_buf, MSGSIZE);

		if (bytes_written < 0){
			ret_status = ST_ERR_COM_FAILURE;
		
			if (DEBUG)
				syslog(LOG_INFO,"Enqueue_Failure: Error: com failed"
				"(%d,%d,%d)\n", com_sock, msg_hdr->prio,msg_hdr->object_id);

		}
		else{
			if (DEBUG)
				syslog(LOG_INFO,"Enqueue_Failure: Error: request rejected"
				"(%d,%d,%d)\n", com_sock, msg_hdr->prio,msg_hdr->object_id);
 	
	
		}
		close(com_sock);
	}
	return ret_status;
}
int handle_enqueue_success(int com_sock, struct proto *msg_hdr, int filesize)
{
	int type = REQUEST_ACCEPT;
	int ret_status = ST_ACCEPT;
	int bytes_written = 0;	

	char *msg_buf = malloc(MSGSIZE);
	
	make_msg_hdr(msg_buf, type, msg_hdr->prio, msg_hdr->object_id,
				msg_hdr->start_idx, filesize);

	if (DEBUG)
		syslog(LOG_INFO,"Enqueue_Success: (%d,%d,%d)\n", 
			com_sock, msg_hdr->prio,msg_hdr->object_id);

	bytes_written = dans_send(com_sock, msg_buf, MSGSIZE);
	
	if (bytes_written < 0){
		ret_status = ST_ERR_COM_FAILURE;
	
		if (DEBUG)						
			syslog(LOG_INFO,"Com_Handler: Error: com failed"
			"(%d,%d,%d)\n",com_sock,msg_hdr->prio, msg_hdr->object_id);
	}
	else{	
		if (DEBUG)
			syslog(LOG_INFO,"Com_Handler: Info: request accepted"
			"(%d,%d,%d)\n", com_sock, msg_hdr->prio, msg_hdr->object_id);

		sem_post(&job_in_sys_mutex);
	}
	return ret_status;
}

int send_file (struct job_data* j_data)
{
    	int total_bytes_sent = 0;
  	int bytes_to_send = CHUNKSIZE;
	
    	int bytes_sent = 0, bytes_read=0;

    	int fd, fid = j_data->object_id;
	int com_sock = j_data->sock, prio = j_data->prio;
    	FILE *fp;

    	char fileName[50], *cur_buf = NULL;
    
    	cur_buf = malloc(CHUNKSIZE);
    	if (cur_buf == NULL){
		syslog(LOG_INFO, "send_file: Error: malloc failed\n");
		return 0;
	}
	memset(cur_buf, 0, CHUNKSIZE);

	sprintf(fileName, "%s/F%d", g_config.data_dir, j_data->object_id);

    	fp = fopen(fileName, "rb");
    	if (fp == NULL) {
		if (DEBUG)
			syslog(LOG_INFO, "send_file: File not found. "
				"(%d,%d,F%d)\n", com_sock, prio, fid);
    		return -1;
	}
   
	if (DEBUG)
		syslog(LOG_INFO, "send_file: starting transfer. "
			"(%d,%d,%d)\n", com_sock, prio, fid);

	
	while (!feof(fp))
    	{
		/*if (g_config.noise == 1){
			noise = get_noise_latency();
			usleep(noise);
		}*/
	
        	bytes_read = fread(cur_buf, sizeof(char), CHUNKSIZE, fp);
		if (bytes_read <=0){
			if (DEBUG)
				syslog(LOG_INFO, "send_file: fread failed." 
					"(%d,%d,%d)\n", com_sock, prio, fid);

			break;
		}

	 	if (bytes_read > 0)
        	{
                	bytes_to_send = bytes_read;
                	if ((bytes_sent = write(com_sock, cur_buf, bytes_to_send)) <=0 ) {
				if(DEBUG)
                	        	syslog(LOG_INFO, "send_file: com failure.%s"
						"(%d,%d,%d)\n", strerror(errno), 
						com_sock, prio, fid);

                        	break;
                	}
                	else{
                        	total_bytes_sent += bytes_sent;
                        	/*syslog(LOG_INFO, "send_file: (%d,%d,%d) bytes_read=%d"
						" Total Bytes Sent= %d\n",com_sock, prio, 
						fid, bytes_read, total_bytes_sent);
				*/
                	}

			// PURGE REQUEST MESSAGE LOGIC GOES HERE
			/*
                	FD_ZERO(&readfds);
                	FD_SET(com_sock, &readfds);
                	max_sock = com_sock + 1;

                	tv.tv_sec = 0;
                	tv.tv_usec = 1;
                	rv = select(max_sock, &readfds, NULL, NULL, &tv);
			if (rv == -1){
				syslog(LOG_INFO, "send_file: com failure. (%d,%d,%d)\n", 
					com_sock, prio, fid);
                		break;
			}
			
	                if (rv == 0)
	                        continue;
	
	                bytes_recvd = read(com_sock, read_buf, READ_BUF_LEN);

			if (bytes_recvd <= 0){
				syslog(LOG_INFO, "send_file:(%d,%d,F%d). errno=%d, %s\n", 
					com_sock, prio, fid, errno, strerror(errno));
				break;
			}
		
			memcpy(&msg, read_buf, sizeof(msg));
	                if ((bytes_recvd > 0) && (msg.type == REQUEST_PURGE)){
	
	                        syslog(LOG_INFO, "send_file: Received Purge signal. "
	                        	"(%d,%d,%d)\n", com_sock, prio, fid);
	                        break;
	                }*/

	        }
	}
	
	fclose(fp);

    	if (!g_config.caching){
        	if ((fd=open(fileName, O_RDONLY)) != -1) {
                	posix_fadvise(fd, 0,0,POSIX_FADV_DONTNEED);
                	close(fd);
        	}
    	}


	if (DEBUG && total_bytes_sent == j_data->size){
		syslog(LOG_INFO, "send_file: File Transfered. "
                                "(%d,%d,%d)\n", com_sock, prio, fid);
	}
    	else if (DEBUG)
        	syslog(LOG_INFO, "send_file: Transfer Failed. %d Bytes sent "
                	"(%d,%d,%d)\n", total_bytes_sent, com_sock, prio, fid);


    	free(cur_buf);
/*	if (!make_non_blocking(com_sock))
	{
		//close(com_sock);
		syslog(LOG_INFO, "send_file: socket error."
                                "(%d,%d,%d)\n", com_sock, prio, fid);
	}
	else{
		event.data.fd = com_sock;
		event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
		if (epoll_ctl(epoll_fd[prio], EPOLL_CTL_ADD, com_sock, &event) == -1){
			if (errno == 9){
				syslog(LOG_INFO, "Server: Couldn't add socket back to epoll list"
						" Remote peer closed the connection.\n");
				//close(com_sock);
			}else
				syslog(LOG_INFO,"Server: Couldn't add socket back to epoll list\n");
		
		}
	}
*/
    	return total_bytes_sent;

}
int in_queue_purge(int priority, int com_sock, int object_id)
{
	int rv;
	if (priority == PRIMARY){
		pthread_mutex_lock(&QUEUE_LOCK[PRIMARY]);
		rv = req_purge(Req_Queue[PRIMARY], com_sock);
		pthread_mutex_unlock(&QUEUE_LOCK[PRIMARY]);
	}
	else if (priority == SECONDARY){
		pthread_mutex_lock(&QUEUE_LOCK[SECONDARY]);
		rv = req_purge(Req_Queue[SECONDARY], com_sock);
		pthread_mutex_unlock(&QUEUE_LOCK[SECONDARY]);
	}
	return rv;
}
int enqueue_data (struct queue_data data, int priority)
{
	int q_len;
	if (priority == PRIMARY) {
			
		// No Limit on primary Queue Size
		
		/*pthread_mutex_lock(&QUEUE_LOCK[PRIMARY]);
               	q_len = enqueue(Req_Queue[PRIMARY], data);
		queue_len[PRIMARY]++;*/


		// Limit on primary Queue Size
		if (queue_len[PRIMARY] >= g_config.max_q_len[PRIMARY]){
			if (DEBUG)
				syslog(LOG_INFO, "Enqueue: Primary Queue is full." 
					"length is %d (%d,%d,F%d)\n", 
					queue_len[PRIMARY], data.sock, 
					data.prio, data.object_id);
		
			pthread_mutex_unlock(&QUEUE_LOCK[PRIMARY]);
			return -1;
		}
		else {	
	        	q_len = enqueue(Req_Queue[PRIMARY], data);
			queue_len[PRIMARY]++;
		
			if (DEBUG)
				syslog(LOG_INFO, "Enqueue: inc: enqueued new request" 
					" in secondary queue(%d,%d,F%d) q_len is %d\n", 
					data.sock, data.prio, data.object_id,
					queue_len[PRIMARY]);
		}
	
		pthread_mutex_unlock(&QUEUE_LOCK[PRIMARY]);
	}

	else if(priority == SECONDARY){

		pthread_mutex_lock(&QUEUE_LOCK[SECONDARY]);

        	if (queue_len[SECONDARY] >= g_config.max_q_len[SECONDARY]){
			if (DEBUG)
				syslog(LOG_INFO, "Enqueue: Secondary Queue is full." 
					"q_len is %d (%d,%d,F%d)\n", 
					queue_len[SECONDARY], data.sock, 
					data.prio, data.object_id);
		
			pthread_mutex_unlock(&QUEUE_LOCK[SECONDARY]);
			return -1;
		}
		else {	
	        	q_len = enqueue(Req_Queue[SECONDARY], data);
			queue_len[SECONDARY]++;

			if (DEBUG)
				syslog(LOG_INFO, "Enqueue: inc: enqueued new request" 
					" in secondary queue(%d,%d,F%d)"
					" queue_len is %d\n", data.sock, 
					data.prio, data.object_id,
					queue_len[SECONDARY]);


		}
		pthread_mutex_unlock(&QUEUE_LOCK[SECONDARY]);
	}
	return q_len;
}


