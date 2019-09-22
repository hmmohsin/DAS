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

#include "server.h"

void* server(void *param_ptr)
{
	struct server *serv_params = (struct server*)param_ptr;
	
	int listen_sock, optval=1, new_sock;
	int sock_status;
	int server_class = serv_params->serv_class;
	int event_count=0, itr, curr_sock;
	
	struct sockaddr_in serv_addr;
	struct epoll_event event;
	struct epoll_event *events_buff;

	memset(&serv_addr, 0, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(serv_params->ip_addr);
	serv_addr.sin_port = htons(serv_params->port);

	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	
	/* Making the listen socket non blocking so that it may restart immidiately*/

	if (listen_sock < 0){
        	if (DEBUG)
			syslog(LOG_INFO, "Error: Failed to create listen socket\n");
		exit(0);
	}
    	if (bind(listen_sock,(struct sockaddr *)&serv_addr, 
				sizeof(struct sockaddr)) < 0){
        	if (DEBUG)
			syslog(LOG_INFO, "Error: Failed to bind listen socket\n");
		exit(0);
	}
    	
  	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
	make_non_blocking(listen_sock);
	if (listen(listen_sock, 100) < 0)
        	perror("Error: listen failure\n");
	
	
	epoll_fd[server_class] = epoll_create(1);
	if (epoll_fd[server_class] == -1){
        	if (DEBUG)
			syslog(LOG_INFO, "Error: Failed to create epoll file descriptor\n");
		exit(0);
	}
	
	events_buff = calloc (MAXEPOLLEVENTS, sizeof (event));
	
	event.data.fd = listen_sock;
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
	if (epoll_ctl(epoll_fd[server_class], EPOLL_CTL_ADD, listen_sock, &event) == -1){
		
        	if (DEBUG)
			syslog(LOG_INFO, "Error: Failed to add listen socket to epoll list\n");
		exit(0);
	}
	signal(SIGPIPE, SIG_IGN);	

	while(1){
		
		event_count = epoll_wait(epoll_fd[server_class], 
					events_buff, MAXEPOLLEVENTS, -1);
		if (DEBUG && event_count == -1){
			
			syslog(LOG_INFO, "Info: epoll_wait returned -1. errno %d\n",errno);
		}
		for(itr =0; itr < event_count; itr++){

			curr_sock = events_buff[itr].data.fd;

			if ((events_buff[itr].events & EPOLLERR) ||
			    (events_buff[itr].events & EPOLLHUP)){ 
			
				if (DEBUG && epoll_ctl(epoll_fd[server_class], 
					EPOLL_CTL_DEL, curr_sock, &event) != 0)
					
					syslog(LOG_INFO, "Server: EPOLL_CTL_DEL failed");
				else{
				
					pthread_mutex_lock(&QUEUE_LOCK[server_class]);
					if (req_purge(Req_Queue[server_class], 
								curr_sock) != -1){
						queue_len[server_class]--;
						if (DEBUG && server_class == SECONDARY){
							
							syslog(LOG_INFO, "Server: dec: Error on socket %d. "
							"Closing Socket. queue_len=%d\n", 
							curr_sock, queue_len[server_class]);
						}
					}
				}
				pthread_mutex_unlock(&QUEUE_LOCK[server_class]);
					
				close(curr_sock);
			        

				continue;

			}
			else if (events_buff[itr].events & EPOLLRDHUP){
				
				if(DEBUG && epoll_ctl(epoll_fd[server_class], 
					EPOLL_CTL_DEL, curr_sock, &event) !=0 ){
					syslog(LOG_INFO, "Server: EPOLL_CTL_DEL failed");
				}
				else{	
					pthread_mutex_lock(&QUEUE_LOCK[server_class]);
					if (req_purge(Req_Queue[server_class], 
								curr_sock) != -1){
						queue_len[server_class]--;
				
						if (DEBUG && server_class == SECONDARY)
							syslog(LOG_INFO, "Server: dec: connection on socket %d "
                        	        	        "closed by remote peer. queue_len=%d\n", 
							curr_sock, queue_len[server_class]);
					}
				}
				pthread_mutex_unlock(&QUEUE_LOCK[server_class]);

				close(curr_sock);
			        
				continue;
			}
			else if (events_buff[itr].events & EPOLLIN){ 

				if (curr_sock == listen_sock){
					do {
						new_sock = dans_accept(listen_sock);
						if (new_sock == -1){
							break;
						}
						
						event.data.fd = new_sock;
						event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | 
									EPOLLERR | EPOLLHUP;
						if (epoll_ctl(epoll_fd[server_class], EPOLL_CTL_ADD,
									new_sock, &event) == -1){
							syslog(LOG_INFO, "Server: Couldn't add new socket\n"
								"Error = %d, %s", errno, strerror(errno));
							//Reject request and close connection with error.
							//exit(0);
						}
					}while(1);
				}
				else{
					sock_status = dans_com_handler(curr_sock, 
								serv_params->serv_class);
					if (sock_status == ST_PURGE_RECVD){
						if (DEBUG)
							syslog(LOG_INFO, "Server: Purge Request"
							" (%d,%d,-)\n", curr_sock, server_class);
					
						close(events_buff[itr].data.fd);
						continue;
					}
					else if(sock_status == ST_ERR_COM_FAILURE){
						if (DEBUG)
							syslog(LOG_INFO, "Server: Communication failed"
							" (%d,%d,-).\n", curr_sock, server_class);
						
						close(events_buff[itr].data.fd);
						continue;
					}
					else if(sock_status == ST_ERR_QUEUE_OVERFLOW){
										
						epoll_ctl(epoll_fd[server_class], EPOLL_CTL_DEL, curr_sock, &event);
						close(curr_sock);
						if (DEBUG)
							syslog(LOG_INFO, "Server: Queue limit reached. "
							"(%d,%d,-).\n",curr_sock, server_class);
						continue;
					}
					else if (sock_status == ST_ERR_CONN_CLOSED){
						
        					if (DEBUG)
							syslog(LOG_INFO, "Server: Connection closed by client."
							"(%d,%d,-)",curr_sock, server_class);
	
						continue;
					}
					else if (DEBUG && sock_status == ST_ACCEPT){
						syslog(LOG_INFO, "Server: New request accepted. "
							"(%d,%d,-).\n", curr_sock, server_class);

					}
				}
			}
		}
	}
}
