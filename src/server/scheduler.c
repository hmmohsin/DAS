#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

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

void* disk_scheduler(void *ptr)
{
	int deq_ret;
	struct queue_data *q_data = malloc(sizeof(struct queue_data));
	struct queue_data *j_data = NULL;	
	pthread_t th_handle;
	int run_count[2];
	int epoll_ret;
	
	struct epoll_event event;

	srand(27172);

	run_count[PRIMARY] = 0;
	run_count[SECONDARY] = 0;
	while(1){
		//reverse semaphore
		while(1){
			
			sem_wait(&job_in_sys_mutex);
			sem_wait(&job_limit_mutex);

			pthread_mutex_lock(&QUEUE_LOCK[PRIMARY]);
			deq_ret = dequeue(Req_Queue[PRIMARY], q_data);
			if (deq_ret != -1)
				queue_len[PRIMARY]--;
			pthread_mutex_unlock(&QUEUE_LOCK[PRIMARY]);
			
			if(deq_ret != -1){
			
				j_data = malloc(sizeof(struct queue_data));
				memcpy(j_data, q_data, sizeof(struct queue_data));
				j_data->prio = PRIMARY;
			
				epoll_ret = epoll_ctl (epoll_fd[PRIMARY], 
							EPOLL_CTL_DEL, 
							q_data->sock, &event);
				if (epoll_ret != 0){
					if (DEBUG)
						syslog(LOG_INFO, "Scheduler: "
							"Failed to delete "
							"from primary epoll list."
							"(%d,%d,%d)\n", 
							q_data->sock, PRIMARY, 
							q_data->object_id);

                                        sem_post(&job_limit_mutex);
                                }


				else{
					make_blocking(q_data->sock);
					
					if (strcmp(g_config.sched_mode, "disk") == 0){	
						pthread_create(&th_handle, NULL, 
							disk_worker, (void*)j_data);
					}else if(strcmp(g_config.sched_mode, "hdfs") == 0){
						pthread_create(&th_handle, NULL, 
							hdfs_worker, (void*)j_data);
				
					}else if(strcmp(g_config.sched_mode, "memory") == 0){
						pthread_create(&th_handle, NULL, 
							memory_worker, (void*)j_data);
					}

					pthread_mutex_lock(&COUNT_LOCK[PRIMARY]);
					run_count[PRIMARY] = ++curr_conn_count[PRIMARY];
					pthread_mutex_unlock(&COUNT_LOCK[PRIMARY]);
					
					if (DEBUG)	
						syslog(LOG_INFO, "Scheduler: Dispatched Primary request." 
							"stats (%d/%d) (%d,%d,%d)\n", run_count[PRIMARY], 
							run_count[SECONDARY],q_data->sock, PRIMARY, 
										q_data->object_id);
				}
			}
			else{
				break;
			}
		}
		
		if (curr_conn_count[SECONDARY] < g_config.sec_max_conn){
				
        	        pthread_mutex_lock(&QUEUE_LOCK[SECONDARY]);
			deq_ret = dequeue(Req_Queue[SECONDARY], q_data);
			if (deq_ret != -1){
				queue_len[SECONDARY]--;

				if (DEBUG)
					syslog(LOG_INFO,"Scheduler: dec: dequeud new req"
					"from secondary epoll list.(%d,%d,%d) queue_len=%d\n", 
						q_data->sock, SECONDARY, q_data->object_id, 
						queue_len[SECONDARY]);

			}
	                pthread_mutex_unlock(&QUEUE_LOCK[SECONDARY]);

			if (deq_ret != -1){
						
				j_data = malloc(sizeof(struct job_data));
				memcpy(j_data, q_data, sizeof(struct queue_data));
				j_data->prio = SECONDARY;
					
				epoll_ret= epoll_ctl(epoll_fd[SECONDARY], 
					EPOLL_CTL_DEL, q_data->sock, &event);
			
				// Need to check this case. Socket close issue may pop up	
				if (epoll_ret != 0){
			
					if (DEBUG)
						syslog(LOG_INFO,"Scheduler: Couldn't delete"
							"from secondary epoll list. (%d,%d,%d)\n", 
							q_data->sock,SECONDARY, q_data->object_id);

					sem_post(&job_limit_mutex);
				}
				else{
					make_blocking(q_data->sock);
					
					if (strcmp(g_config.sched_mode, "disk") == 0){	
						pthread_create(&th_handle, NULL, 
							disk_worker, (void*)j_data);
					}else if(strcmp(g_config.sched_mode, "hdfs") ==0){
						pthread_create(&th_handle, NULL, 
							hdfs_worker, (void*)j_data);
					}else if(strcmp(g_config.sched_mode, "memory") == 0){
						pthread_create(&th_handle, NULL, 
							memory_worker, (void*)j_data);
					}
		
					pthread_mutex_lock(&COUNT_LOCK[SECONDARY]);
					run_count[SECONDARY] = ++curr_conn_count[SECONDARY];
					pthread_mutex_unlock(&COUNT_LOCK[SECONDARY]);

					if (DEBUG)
						syslog(LOG_INFO, "Scheduler: Dispatched Secondary "
							"request. stats(%d/%d) (%d,%d,%d)\n", 
							run_count[PRIMARY],run_count[SECONDARY],
							q_data->sock, SECONDARY, q_data->object_id);
				}
			}
			else{
				sem_post(&job_limit_mutex);
			}
		}
	
		else{
			sem_post(&job_limit_mutex);
			sem_post(&job_in_sys_mutex);
		}	
	}
}
void* memory_worker(void* ptr){
	struct job_data *j_data = (struct job_data*)ptr;
	
	
	int com_sock = j_data->sock;
	int mem_file_size = 5000000;
	//int mem_file_size = g_config.mean_file_size;
	//int chunksize = g_config.chunksize;
	int chunksize = 1000;
	int tos, latency=0, object_size=0;	
	int bytes_sent, total_bytes_sent=0;
	char *data_buf = malloc(chunksize);

	object_size = j_data->size;
	if (g_config.noise > 0){
		latency = get_noise_latency(object_size);
		usleep(latency);
	}


	if (DEBUG)
		syslog(LOG_INFO, "MemWorker: starting work. (%d,%d,%d)\n", 
			j_data->sock, j_data->prio, j_data->object_id);

	if (j_data->prio == PRIMARY){
		tos = 4;
	}
	else if (j_data->prio == SECONDARY){
		tos = 8;
	}

	setsockopt(com_sock, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));

	while (total_bytes_sent < mem_file_size){
		bytes_sent = write(com_sock, data_buf, chunksize);	
		if (bytes_sent <= 0){
		
			if (DEBUG)
				syslog(LOG_INFO,"Error: MemWorker: "
						"Failed to send data\n");
			break;
		}
		total_bytes_sent += bytes_sent;
	}

	close(j_data->sock);	
	close_worker(j_data->prio);	

	free(j_data);

        sem_post(&job_limit_mutex);
	pthread_detach(pthread_self()); 

	return (void*)0;

	
}
void* disk_worker(void* ptr)
{
        struct job_data *j_data = (struct job_data*)ptr;

        int prio_value, latency=0;
	int total_bytes_sent=0;
	pid_t tid = pthread_self();
	int object_size = j_data->size;	

	if (g_config.noise > 0){
		latency = get_noise_latency(object_size);
		usleep(latency);
	}

	/*MOVE THIS BLOCK TO THE UPPER CONDITION*/	

        else if (j_data->prio == SECONDARY)
                prio_value = getPrioValue (IOPRIO_CLASS_IDLE, 4);

        setPrio (IOPRIO_WHO_PROCESS, tid, prio_value);
	
	if (DEBUG)
		syslog(LOG_INFO, "Worker: starting work. (%d,%d,%d,%d)\n", 
			j_data->sock, j_data->prio, j_data->object_id, j_data->size);
	total_bytes_sent = send_file(j_data);

	if (DEBUG)
		syslog(LOG_INFO, "Worker: Total Bytes Sent = %d\n", total_bytes_sent);

	close(j_data->sock);
	
	close_worker(j_data->prio);	
	free(j_data);
        sem_post(&job_limit_mutex);
	pthread_detach(pthread_self()); 
	return (void*)0;
}

void* hdfs_worker(void* ptr)
{
        struct job_data *j_data = (struct job_data*)ptr;
	struct sockaddr_in proxy_addr;
	struct proto *hdr = malloc(sizeof(struct proto));
	
	char *msg = malloc(MSGSIZE);
	int c_sock, p_sock, issue=0;

	int total_bytes_sent=0;
	int bytes_to_recv = 0;
	int bytes_to_pipe = 0;
	int bytes_sent = 0;
	int bytes_recvd=0; 
	int obj_id = 0;
	int sock_error = 0;
	int latency=0;
	
	c_sock = j_data->sock;
	obj_id = j_data->object_id;	
	make_msg_hdr(msg, REQUEST_GETFILE, j_data->prio, 
			j_data->object_id,j_data->start_idx, j_data->size);
	
	proxy_addr.sin_family = AF_INET;
	proxy_addr.sin_addr.s_addr = inet_addr(g_config.ip_addr);
	proxy_addr.sin_port = htons(g_config.hdfs_proxy_port);




	
	if (DEBUG)
		syslog(LOG_INFO,"Info: HDFS_Worker: "
			"Work started on file %d\n",obj_id);

	if ((p_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		if (DEBUG)
			syslog(LOG_INFO,"Info: hdfs_worker:" 
				"socket creation failed\n");

		issue = 1;
	}
	else if(connect(p_sock, (struct sockaddr *)&proxy_addr, 
						sizeof(proxy_addr)) < 0){

		if (DEBUG)
			syslog(LOG_INFO,"Info: hdfs_worker: connect failed"
						" for file %d\n", obj_id);
	
		issue = 1;
	}	
	
	if (DEBUG)
		syslog(LOG_INFO,"Info: HDFS_Worker: Connection established"
				" with proxy for file %d\n", obj_id);
	
	if (dans_send(p_sock, msg, MSGSIZE) != MSGSIZE){
		if (DEBUG)
			syslog(LOG_INFO,"Info: hdfs_worker: dans_send failed\n");
		
		issue = 1;
	}
	
	if (DEBUG)
		syslog(LOG_INFO,"Info: HDFS_Worker: Request message sent"
					" for file %d\n", obj_id);
	if (dans_recv(p_sock, msg, MSGSIZE) != MSGSIZE){
	
		if (DEBUG)
			syslog(LOG_INFO,"Info: hdfs_worker: dans_recv failed"
						" for file %d\n", obj_id);
		
		issue = 1;
		
	}
	memcpy(hdr, msg, MSGSIZE);
	bytes_to_recv = hdr->size;


	if (DEBUG)
		syslog(LOG_INFO, "Info: HDFS_Worker: Proxy scheduled request" 
				" %d, %d, %d, %d, %d\n", hdr->type, hdr->prio, 
				hdr->object_id, hdr->start_idx, hdr->size);

	if (dans_send(c_sock, msg, MSGSIZE) != MSGSIZE){
		
		if (DEBUG)
			syslog(LOG_INFO,"Info: hdfs_worker: dans_send failed"
					" for file %d\n", obj_id);
		
		issue = 1;
	}

	if (issue){
		close(p_sock);
		close(c_sock);

		close_worker(j_data->prio);	
		free(j_data);
	        sem_post(&job_limit_mutex);
		pthread_detach(pthread_self()); 
		return (void*)0;
	}

	memcpy(hdr, msg, MSGSIZE);
	bytes_to_recv = hdr->size;

	char *msg_buf = malloc(64000);
	char *send_buf;
	
	if (DEBUG)
		syslog(LOG_INFO, "Info: HDFS_Worker: Starting work for file %d\n",
									 obj_id);
	if (g_config.noise > 0){
		latency = get_noise_latency(bytes_to_recv);

		if (DEBUG)
			syslog(LOG_INFO, "Sleep: Would wait for %d seconds\n", 
								latency);
		usleep(latency);
	}

	
	do{
		bytes_recvd = read(p_sock, msg_buf, 64000);
		if(bytes_recvd < 0){
		
			if (DEBUG)
				syslog(LOG_INFO, "Info: HDFS_Wroker:" 
					"socket recv error %s\n", strerror(errno));
			break;
		}
		if (bytes_recvd == 0){
			if (DEBUG)
				syslog(LOG_INFO, "Info: HDFS_Wroker: "
					"recvd 0 bytes sock is closed\n");
			break;
		}
		bytes_to_pipe = bytes_recvd;
		send_buf = msg_buf;
		do{
			bytes_sent = write(c_sock, send_buf, bytes_to_pipe);
			if (bytes_sent <= 0){
		
				if (DEBUG)
					syslog(LOG_INFO, "Info: HDFS_Wroker:"
							" socket error\n");
				sock_error = 1;
				break;
			
			}
			bytes_to_pipe -= bytes_sent;
			send_buf += bytes_sent;

		}while(bytes_to_pipe !=0);
		
		if (sock_error)
			break;

		total_bytes_sent += bytes_recvd;
	}while(1);

	if (DEBUG)
		syslog(LOG_INFO, "Info: HDFS_Worker: "
			"Total bytes sent %d for file %d\n", 
				total_bytes_sent, obj_id);

	close(p_sock);
	close(c_sock);

	close_worker(j_data->prio);	
	free(j_data);
	
        sem_post(&job_limit_mutex);
	pthread_detach(pthread_self()); 
	return (void*)0;
}


int get_noise_latency(int object_size)
{
	double latency_dist_ratio[] = {2, 2.3, 2.6, 2.8, 3.2, 3.5, 8};
	double latency = 0.0;
	int delay = 0;
	int ftt = g_config.noise;
	int rand_num = rand()%1000 + 1;

	if (rand_num < 900)
		return delay;
	else if (rand_num >= 900 && rand_num<930)
		latency = latency_dist_ratio[0] * ftt;
	else if (rand_num >= 930 && rand_num < 950)
		latency = latency_dist_ratio[1] * ftt;
	else if (rand_num >= 950 && rand_num < 970)
		latency = latency_dist_ratio[2] * ftt;
	else if (rand_num >= 970 && rand_num < 980)
		latency = latency_dist_ratio[3] * ftt;
	else if (rand_num >= 980 && rand_num <990)
		latency = latency_dist_ratio[4] * ftt;
	else if (rand_num >= 990 && rand_num <999)
		latency = latency_dist_ratio[5] * ftt;
	else if (rand_num == 1000)
		latency = latency_dist_ratio[6] * ftt;

	delay = (int)latency;
	if (DEBUG)
		syslog(LOG_INFO,"sleep: ftt=%d rand_num=%d"
				" delay is %d microseconds\n",
					ftt, rand_num, delay);
	return delay;
}
