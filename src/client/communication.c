# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <pthread.h>

# include <syslog.h>
# include <error.h>
# include <errno.h>
# include <unistd.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include "client.h"

int make_pkt(char *buf, int type, int prio, int obj_id, int start_idx, int size)
{
	struct header hdr;
	int len = sizeof(struct header);

	syslog(LOG_INFO, "HM_Debug: type:%d, prio:%d, obj_id:%d\n", 
				type, prio, obj_id);
	hdr.type = type;
	hdr.prio = prio;
	hdr.object_id = obj_id;
	hdr.start_idx = start_idx;
	hdr.size = size;

	memcpy(buf, &hdr, len);
	
	return 1;
}
int get_connection (struct task* curr_task, int type, int priority)
{
	// This method would be used to get new sock from sock pool
	// If there is no free socket is available, create a new one

	// creating new sockets for every request for now
	int server_id, com_sock;
	struct sockaddr_in serv_addr;
	struct server tmp_server;

	server_id = curr_task->server_id[type];
	
	struct linger so_linger;
        so_linger.l_onoff = 1;
        so_linger.l_linger = 0;

	com_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (setsockopt(com_sock, SOL_SOCKET, SO_LINGER, 
			&so_linger, sizeof so_linger) < 0)
        {
               	syslog(LOG_INFO,"Error: Failed to set socket options.\n");
	
		return -1;
	}	
	tmp_server = get_server_info(server_id);

/*	if (server_id == 0){
		printf("id=%d, IP=%s port=%d\n", 
			tmp_server.id, tmp_server.ip_addr, 
			tmp_server.port_no[priority]);
	}*/

	if (tmp_server.id == -1){
	
                syslog(LOG_INFO, "Error: get_connection: Invalid server id %d\n", 
								server_id);
		return -1;
	}
	
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(tmp_server.ip_addr);
	serv_addr.sin_port = htons(tmp_server.port_no[priority]);

	if (connect(com_sock, (struct sockaddr *)&serv_addr,
				sizeof(serv_addr)) < 0)
        {
                syslog(LOG_INFO, "Error: Failed to connect with server %d\n", 
								server_id);
		printf("Error: Failed to connect with server. %d %d %s\n", 
				server_id, errno, strerror(errno));

                return -1;
        }
	else{
		curr_task->com_sock[type] = com_sock;
		curr_task->serv_addr[type] = serv_addr;
	}
	return 0;	
}		
int dans_send(int sock, char *msg_buf)
{
	int len = sizeof (struct header);
	int bytes_sent = 0, total_bytes_sent=0;
	do{
		bytes_sent = write(sock, msg_buf, len);
		if (bytes_sent <= 0){
			syslog(LOG_INFO, "Error: dans_send(): Socket error occurred\n");

			return -1;
		}
		msg_buf += bytes_sent;
		total_bytes_sent += bytes_sent;

	}while(total_bytes_sent < MSGSIZE);

	return total_bytes_sent;
}
int dans_recv(int sock, char *buf)
{
	int bytes_to_read, bytes_read = 0, total_bytes_read = 0;
	int total_bytes_to_read = MSGSIZE;
	
	bytes_to_read = total_bytes_to_read;
	do {
		
		bytes_read = read(sock, buf+total_bytes_read, bytes_to_read);
		if (bytes_read > 0){	
			total_bytes_read += bytes_read;
			if (total_bytes_read == total_bytes_to_read){
				return total_bytes_read;
			}
			
			bytes_to_read = total_bytes_to_read - total_bytes_read;
		}
		else if(bytes_read <0){
			syslog(LOG_INFO, "Error: dans_recv(): Socket error occurred\n");
			return bytes_read;
		}
		else if (bytes_read == 0)
			return total_bytes_read;

	}while(1);
}
int recv_file(struct flow *flow_info, char *buf, int scheme)
{
	int bytes_to_read, bytes_read = 0, total_bytes_read = 0;

        int type = flow_info->flow_type;
        int sock = flow_info->task_ptr->com_sock[type];
        int osize = flow_info->task_ptr->object_size;
	
	int total_bytes_to_read = osize;
	char *msg_buf = malloc(MSGSIZE*sizeof(char));
	if (msg_buf == NULL){
		printf("Error: Communication: Malloc failed to allocate memory for msg_buf\n");
		exit(0);
	}


//	struct timeval tv;
//	fd_set readfd;

	bytes_to_read = total_bytes_to_read;
	do {
		
		bytes_read = read(sock, buf+total_bytes_read, bytes_to_read);
		
		if (bytes_read > 0){	
			total_bytes_read += bytes_read;
			
			bytes_to_read = total_bytes_to_read - total_bytes_read;
			
		}
		else if(bytes_read <0){
			syslog(LOG_INFO, "Error: recv_file(): Socket error %s. bytes_read%d\n", 
							strerror(errno), total_bytes_read);
			//return bytes_read;
			break;
		}
		else if (bytes_read == 0){
			syslog(LOG_INFO, "Info: recv_file(): Socket is closed. bytes_read=%d\n",
									total_bytes_read);
			//return bytes_read;
			break;
		}
	}while(total_bytes_read < osize);
	
	free(msg_buf);
	return total_bytes_read;
}
void dans_parse(char *buf, struct header *data)
{
	int len = sizeof (struct header);
	memcpy(data, buf, len);
}
