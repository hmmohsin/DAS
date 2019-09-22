# include <stdio.h>
# include <stdlib.h>
# include <math.h>
# include <string.h>
# include <syslog.h>

# include "client.h"
struct server get_server_info (int id)
{
        struct server _server;

        _server.id = -1;
        _server.port_no[PRIMARY] = -1;
        _server.port_no[SECONDARY] = -1;

        bzero(_server.ip_addr, 20);

        if ((id >= 0) && (id < g_config->dn_count)) {
                _server.id = g_config->servers[id].id;
                strcpy(_server.ip_addr, g_config->servers[id].ip_addr);
                _server.port_no[PRIMARY] = g_config->servers[id].port_no[PRIMARY];
                _server.port_no[SECONDARY] = g_config->servers[id].port_no[SECONDARY];
        }
        return _server;
}
void get_scheme_name(int scheme, char *scheme_name)
{
	if (scheme == 0)
		strcpy(scheme_name,"BASE");
	else if (scheme == 1)
		strcpy(scheme_name,"DUP");
	else if (scheme == 2)
		strcpy(scheme_name,"PRIO");
	else if (scheme == 3)
		strcpy(scheme_name,"PURGE");
	else if (scheme == 4)
		strcpy(scheme_name, "BYTE_AGG");
	else if (scheme == 5)
		strcpy(scheme_name, "HEDGED_REQ");
        else if (scheme == 7)
                strcpy(scheme_name, "APPTO");
        else if (scheme == 9)
                strcpy(scheme_name, "TIED");

}
void flush_data_struct(struct job *job_store, int j_count)
{
	int j=0;
	for (j=0; j<j_count; j++){
		free(job_store[j].tasks);
	}
	free(job_store);
}
int get_file_replicas(int file_id, int *file_replicas)
{
	if (file_id > g_file_count)
		return -1;
	file_replicas[0] = g_file_meta[file_id].replicas[0];
	file_replicas[1] = g_file_meta[file_id].replicas[1];
	file_replicas[2] = g_file_meta[file_id].replicas[2];
	return 0;
}

int purge_replica(struct flow *flow_info){
	struct job* job_ptr = flow_info->job_ptr;
	char *msg_buf = malloc(MSGSIZE*sizeof(char));	
	
	int task_idx = flow_info->task_idx;
	int type = flow_info->flow_type;
	int oid = job_ptr->tasks[task_idx].object_id;
	int com_sock,rep_type, bytes_sent=0;
	
	rep_type = get_rep_type(type);
	com_sock = job_ptr->tasks[task_idx].com_sock[rep_type];
	syslog(LOG_INFO,"Info: replica_purge() working on socket %d."
				" (%d,%d)\n", com_sock,oid,type);


	//PURGE MESSAGE LOGIC
	/*make_pkt(msg_buf, REQUEST_PURGE, prio, oid, 0, 0);	
	bytes_sent = dans_send(com_sock, msg_buf);

	if (bytes_sent != MSGSIZE){
		free(msg_buf);
		return 0;	
	}*/

	syslog(LOG_INFO,"Info: common.c: sent %d bytes", bytes_sent);
	shutdown(com_sock, SHUT_RDWR);
	free(msg_buf);
	return 1;
}
int get_rep_type(int type)
{
	if (type == PRIMARY)
		return SECONDARY;
	
	else if (type == SECONDARY)
		return PRIMARY;

	return -1;
}
int genRandRange(int lower, int upper)
{
	int num = (rand() % (upper - lower + 1)) + lower;
	return num;
}
