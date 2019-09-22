# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <pthread.h>
# include <syslog.h>
# include <time.h>
# include <unistd.h>
# include <sys/socket.h>
#include <sys/time.h>
# include "client.h"

void dispatcher (struct config *g_config, struct job* job_store, struct exp_config e_config)
{
	int idx=0, th_count=0, interval, i,j;
	struct job* job_info;
	pthread_t *th_handle = malloc(g_config->min_job_count*sizeof(pthread_t));
	


	//Server Queue Monitor Patch Start
	g_server_queue_lock = malloc(g_config->dn_count*sizeof(pthread_mutex_t));
	for (i=0; i<2; i++)
		g_server_queue_count[i] = malloc(g_config->dn_count*sizeof(int));
	for (i=0; i<2; i++){
		for(j=0; j<g_config->dn_count; j++){
			pthread_mutex_init(&g_server_queue_lock[j], NULL);
			g_server_queue_count[i][j] = 0;
		}
	}
	//Server Queue Monitor Patch End

	if (th_handle == NULL){
		syslog(LOG_INFO,"Error: Dispatcher: Failed to allocate memory\n");
		exit(0);
	}	
	
        openlog("slog", LOG_PID|LOG_CONS, LOG_LOCAL0);
	printf("Starting dispacther\n");
	for(idx=0; idx < g_config->min_job_count; idx++){
		
		interval = job_store[idx].sleep;
		job_info = &job_store[idx];
		
		if (pthread_create(&th_handle[th_count++], NULL, 
				job_manager, (void*)job_info) == 0){

		}
		else
			syslog(LOG_INFO,"Error: Dispatcher: Failed to create new thread\n");
		usleep(interval);
	}
	do{
		sleep(1);
		//printf("g_curr_job_count_inc = %d, g_curr_job_count_dec=%d\n",
		//		g_curr_job_count_inc, g_curr_job_count_dec);
	}while(g_curr_job_count_inc != g_curr_job_count_dec);
	
	free(th_handle);	
	
}

void* job_manager (void *args)
{
	struct job* job_info = (struct job*)args;
	
	int jid = job_info->job_id;
	int t_count = job_info->tasks_count, tid=0, th_count=0;
	char prio[20];

	struct flow *flow_info=NULL;
	struct task *task_ptr=NULL;
	
	pthread_t *th_handle = malloc(t_count*2*sizeof(pthread_t));
	if (th_handle == NULL){
		syslog(LOG_INFO,"Error: job_manager: Failed to allocated memory.\n");
       		exit(0);
	}
	
	syslog(LOG_INFO,"Info: job_manager: starting new job-%d."
			" Total tasks=%d\n", jid, t_count);

	__sync_fetch_and_add(&g_curr_job_count_inc, 1);

	for(tid = 0; tid < t_count; tid++){
	
		task_ptr = &job_info->tasks[tid];
		
		flow_info = malloc (sizeof (struct flow));
		if (flow_info == NULL){
			syslog(LOG_INFO,"Error: job_manager: Failed to allocated memory.\n");
			exit(0);
		}
        	flow_info->job_ptr = job_info;
        	flow_info->task_ptr = &job_info->tasks[tid];
		flow_info->job_idx = jid;

       		flow_info->task_idx = tid;
        	flow_info->flow_prio = PRIMARY;
        	flow_info->flow_type = PRIMARY;
		flow_info->task_ptr->status[PRIMARY] = OBJ_ST_WAIT;
                      
		strcpy(prio,"PRIMARY"); 
		if (get_connection (task_ptr, PRIMARY, PRIMARY) == -1){
                	syslog(LOG_INFO,"Job_Manager: (%d,%d) failed to get new connection\n",
				flow_info->job_ptr->job_id, flow_info->task_ptr->task_id);
			//printf ("Job_Manager: server id is %d\n", task_ptr->server_id[PRIMARY]);
			continue;
		}

		if (pthread_create(&th_handle[th_count++], NULL, 
				com_handler, (void*)flow_info) != 0){
			syslog(LOG_INFO,"Error: job_manager: Failed to create new thread.\n");
		}

		if (e_config.scheme == SCHEME_DUP || 
                        e_config.scheme == SCHEME_PRIO ||
                        e_config.scheme == SCHEME_PURGE ||
                        e_config.scheme == SCHEME_HEDGEDREQ ||
                        e_config.scheme == SCHEME_APPTO ||
                        e_config.scheme == SCHEME_TIED){

			flow_info = malloc (sizeof (struct flow));
			if (flow_info == NULL){
				syslog(LOG_INFO,"Error: job_manager: Failed to allocated memory.\n");
				exit(0);
			}
	 		flow_info->job_ptr = job_info;
			flow_info->task_ptr = &job_info->tasks[tid];
			flow_info->job_idx = jid;
        		
       			flow_info->task_idx = tid;
			flow_info->flow_type = SECONDARY;
			flow_info->task_ptr->status[SECONDARY] = OBJ_ST_WAIT;
			
			if((e_config.scheme == SCHEME_PURGE) ||
				(e_config.scheme == SCHEME_PRIO)){

				flow_info->flow_prio = SECONDARY;
				if (get_connection (task_ptr, SECONDARY, SECONDARY) == -1)
					continue;
			}
			else{
				flow_info->flow_prio = PRIMARY;
				if (get_connection (task_ptr, SECONDARY, PRIMARY == -1))
					continue;
			}
			if (pthread_create(&th_handle[th_count++], NULL, 
				com_handler, (void*)flow_info) != 0){
				syslog(LOG_INFO,"Error: job_manager: Failed to create new thread.\n");
			}
		}
	}
		
	do{
		/*printf("Job_Manager: Job %d is waiting for %d tasks to finish\n", 
					job_info->job_id, job_info->curr_tasks_count);*/
		usleep(1000);
	}while(job_info->curr_tasks_count != 0);
	
	__sync_fetch_and_add(&g_curr_job_count_dec, 1);

	syslog(LOG_INFO,"Info: job_manager: completed job-%d."
			" Total tasks=%d\n", jid, t_count);

	free(th_handle);
	pthread_detach(pthread_self());
	
	return NULL;
}

void* com_handler (void *args)
{
	struct flow* flow_info = (struct flow*)args;
	int bytes_recvd=0;
	int jid = flow_info->job_idx; 
	int tid = flow_info->task_idx;
	int prio = flow_info->flow_prio;
	int type = flow_info->flow_type;
	int oid = flow_info->task_ptr->object_id;
	int osize = flow_info->task_ptr->object_size;	
	int sock = flow_info->task_ptr->com_sock[type];
	int rep_type = get_rep_type(type);	


	int ret_status, curr_load, server_id[2];
	
	server_id[PRIMARY] = flow_info->task_ptr->server_id[PRIMARY];	
	server_id[SECONDARY] = flow_info->task_ptr->server_id[SECONDARY];	

	long duration = 0;
	int wait_interval = 0;
	
	struct timeval tv1, tv2;
	struct task *task_ptr = flow_info->task_ptr;
	
	char *msg_data, scheme_name[100];
	char *msg_buf = malloc(MSGSIZE*sizeof(char));
	struct header *msg_hdr = malloc(sizeof(struct header));


	if (msg_hdr == NULL || msg_buf == NULL){
		syslog(LOG_INFO,"Error: com_handler: Failed to allocate memory.\n");
		exit(0);
	}


	server_id[PRIMARY] = task_ptr->server_id[PRIMARY];
	server_id[SECONDARY] = task_ptr->server_id[SECONDARY];

		
	get_scheme_name(e_config.scheme, scheme_name);

	syslog(LOG_INFO,"Info: com_handler: starting new task. "
			"(sock,jid,tid,sid,oid,prio,type)=(%d,%d,%d,%d,%d,%d,%d)\n", 
				sock,jid, tid, server_id[type], oid, prio, type);
	
	
	__sync_fetch_and_add (&flow_info->job_ptr->curr_tasks_count, 1);
	
	
        if (((e_config.scheme == SCHEME_APPTO) ||
		(e_config.scheme == SCHEME_HEDGEDREQ))
		&& (type == SECONDARY)){

	
                wait_interval = g_config->hedged_wait;
                flow_info->task_ptr->bytes_read_perc = flow_info->task_ptr->bytes_read;
		
		if (e_config.scheme == SCHEME_APPTO){
                	usleep(wait_interval);
			ret_status = handle_appto_req(flow_info, type, rep_type);
		}
		else if (e_config.scheme == SCHEME_HEDGEDREQ){
                	usleep(wait_interval);
			ret_status = handle_hedged_req(flow_info, type, rep_type);
			syslog(LOG_INFO,"Purge_Issue: oid=%d, type=%d ret_status=%d\n", 
							oid, type, ret_status);
		}
		if (ret_status <0){
                        update_global_counters(server_id[type], JOB_PURGED, type, INCREMENT);
                        __sync_fetch_and_sub(&flow_info->job_ptr->curr_tasks_count, 1);
	
			if (ret_status <0){
				syslog(LOG_INFO, "Purge_Issue: Replica was finished"
						"(%d,%d,%d,%d,%d,%d,%d)\n",jid, tid, 
						sock,server_id[type], oid, prio, type);

				close(sock);
			}

                        free(msg_buf);
                        free(msg_hdr);
                        free(flow_info);
                        pthread_detach(pthread_self());

                        return NULL;
                }
        }

	make_pkt(msg_buf, REQUEST_GETFILE, type, oid, 0, osize);
	
	gettimeofday(&tv1, NULL);

	//Server Queue Monitor Patch Start
	if (type == PRIMARY){
		curr_load = update_server_load(server_id[PRIMARY], prio, 1);
		task_ptr->server_curr_load[PRIMARY] = curr_load;
	}
	else if (type == SECONDARY){
		curr_load = update_server_load(server_id[SECONDARY], prio, 1);
		task_ptr->server_curr_load[SECONDARY] = curr_load;
	}
	//Server Queue Monitor Patch Start


	//if (e_config.scheme == SCHEME_TIED && type == SECONDARY)
	//	usleep(1000);
	if (!dans_send(sock, msg_buf)){
	
		if (get_status(flow_info, type) == OBJ_ST_PURGED){
			update_global_counters(server_id[type], 
				JOB_PURGED, type, INCREMENT);

			syslog(LOG_INFO, "Info: com_handler: Replica was purged"
						"(%d,%d,%d,%d,%d,%d,%d)\n",jid, tid, 
						sock,server_id[type], oid, prio, type);

		}
		else{
			set_status(flow_info, type, OBJ_ST_ERROR);
			update_global_counters(server_id[type], 
				JOB_ERROR, type, INCREMENT);
	
			syslog(LOG_INFO, "Info: com_handler: Error occured on dans_send()"
						"(%d,%d,%d,%d,%d,%d,%d)\n",jid, tid, 
						sock,server_id[type], oid, prio, type);
		
		}
		
		__sync_fetch_and_sub(&flow_info->job_ptr->curr_tasks_count, 1);

		//Server Queue Monitor Patch Start
		if (type == PRIMARY)
			update_server_load(server_id[PRIMARY], prio, -1);
		else if (type == SECONDARY)
			update_server_load(server_id[SECONDARY], prio, -1);
		//Server Queue Monitor Patch Start

		close(sock);
		free(msg_buf);
		free(msg_hdr);
		free(flow_info);
	
		pthread_detach(pthread_self());	
		
		return NULL;
	}

	update_global_counters(server_id[type], JOB_IN_FLIGHT, type, INCREMENT);
	bytes_recvd = dans_recv(sock, msg_buf);
	
	if (bytes_recvd <= 0){
		
		if (get_status(flow_info, type) == OBJ_ST_PURGED){
			update_global_counters(server_id[type], 
				JOB_PURGED, type, INCREMENT);

			syslog(LOG_INFO, "Info: com_handler: Replica was purged"
						"(%d,%d,%d,%d,%d,%d,%d)\n",jid, tid, 
						sock,server_id[type], oid, prio, type);

		}
		else{
			set_status(flow_info, type, OBJ_ST_ERROR);
			update_global_counters(server_id[type], 
				JOB_ERROR, type, INCREMENT);
	
			syslog(LOG_INFO, "Info: com_handler: Error occured on dans_recv()"
						"(%d,%d,%d,%d,%d,%d,%d)\n",jid, tid, 
						sock,server_id[type], oid, prio, type);
			//close(sock);	
		}

		update_global_counters(server_id[type], JOB_IN_FLIGHT, type, DECREMENT);
		__sync_fetch_and_sub(&flow_info->job_ptr->curr_tasks_count, 1);
		
		//Server Queue Monitor Patch Start
		if (type == PRIMARY)
			update_server_load(server_id[PRIMARY], prio, -1);
		else if (type == SECONDARY)
			update_server_load(server_id[SECONDARY], prio, -1);
		//Server Queue Monitor Patch Start

		close(sock);
		free(msg_buf);
		free(msg_hdr);
		free(flow_info);

		pthread_detach(pthread_self());	
		return NULL;
	}
	
	dans_parse(msg_buf, msg_hdr);

        if (e_config.scheme == SCHEME_TIED){

		pthread_mutex_lock(&flow_info->task_ptr->task_lock);
		ret_status = handle_tied_req(flow_info, type, rep_type);
		pthread_mutex_unlock(&flow_info->task_ptr->task_lock);
                if (ret_status <0){
			
			syslog(LOG_INFO,"Info: Tied_Req: Self Purging"
     				" (%d,%d,%d,%d)\n",jid, tid, prio, type);

                        __sync_fetch_and_sub(&flow_info->job_ptr->curr_tasks_count, 1);
			update_global_counters(server_id[type], JOB_PURGED, type, INCREMENT);
			update_global_counters(server_id[type], JOB_IN_FLIGHT, type, DECREMENT);
			
			
			//Server Queue Monitor Patch Start
			if (type == PRIMARY)
				update_server_load(server_id[PRIMARY], prio, -1);
			else if (type == SECONDARY)
				update_server_load(server_id[SECONDARY], prio, -1);
			//Server Queue Monitor Patch Start

			close(sock);
                        free(msg_buf);
                        free(msg_hdr);
                        free(flow_info);

                        pthread_detach(pthread_self());
                        return NULL;
                }
        }


	if (msg_hdr->type == REQUEST_ACCEPT){


		set_status(flow_info, type, OBJ_ST_INPROG);
		
		syslog(LOG_INFO,"Info: com_handler: Task is scheduled on server. "
				"oid size=%d (%d,%d,%d,%d,%d,%d,%d)\n", msg_hdr->size, 
					sock,jid, tid, server_id[type], oid, prio,type);
		
		pthread_mutex_lock(&flow_info->task_ptr->task_lock);
		flow_info->task_ptr->object_size = msg_hdr->size;
		pthread_mutex_unlock(&flow_info->task_ptr->task_lock);

		
		msg_data = malloc(msg_hdr->size);
		if (msg_data == NULL){
			syslog(LOG_INFO, "com_handler: Failed to allocate memory.\n");
			exit(0);
		}

		
		bytes_recvd = recv_file(flow_info, msg_data, e_config.scheme);

		update_status(flow_info, e_config.scheme, bytes_recvd,
						flow_info->task_ptr->object_size);
		
		gettimeofday(&tv2, NULL);
		duration = (long) (tv2.tv_usec - tv1.tv_usec) +
	                (long) (tv2.tv_sec - tv1.tv_sec)*1000000;
	
		flow_info->task_ptr->duration[type] = duration;
		flow_info->task_ptr->bytes_recvd[type] = bytes_recvd;

		free(msg_data);
		
	}
	else if (msg_hdr->type == REQUEST_REJECT){
		set_status(flow_info, type, OBJ_ST_ERROR);
		syslog(LOG_INFO,"Info: com_handler: Task request is rejected. "
				"(%d, %d,%d,%d,%d,%d,%d)\n", 
				sock, jid, tid, server_id[type], oid, prio,type);

		update_global_counters(server_id[type], JOB_ERROR, type, INCREMENT);
	}
	
	
	
	close(sock);
	syslog(LOG_INFO,"Info: com_handler: Exiting now. (%d, %d,%d,%d,%d,%d,%d)\n", 
				sock, jid, tid, server_id[type], oid, prio,type);
	syslog(LOG_INFO,"Info: com_handler: curr_task_count=%d",
				flow_info->job_ptr->curr_tasks_count);

	
	__sync_fetch_and_sub(&flow_info->job_ptr->curr_tasks_count, 1);
	update_global_counters(server_id[type], JOB_IN_FLIGHT, type, DECREMENT);

	//Server Queue Monitor Patch Start
	if (type == PRIMARY)
		update_server_load(server_id[PRIMARY], prio, -1);
	else if (type == SECONDARY)
		update_server_load(server_id[SECONDARY], prio, -1);
	//Server Queue Monitor Patch Start

	free(msg_buf);
	free(msg_hdr);
	free(flow_info);

	pthread_detach(pthread_self());

	return NULL;
}
int handle_hedged_req(struct flow* flow_info, int self_type, int rep_type)
{
	int jid = flow_info->job_idx; 
	int tid = flow_info->task_idx;
	int prio = flow_info->flow_prio;
	int oid = flow_info->task_ptr->object_id;

	int self_status, rep_status;

	rep_status = get_status(flow_info, rep_type);
	self_status = get_status(flow_info, self_type);
	syslog(LOG_INFO, "Purge_Issue: oid=%d self_status=%d, rep_status=%d, type=%d\n",
				oid, self_status, rep_status, self_type);
	if (rep_status == OBJ_ST_FINISHED){
		syslog(LOG_INFO, "Purge_Issue: Replica is already finished. oid=%d, self_type=%d\n",
									oid, self_type);
	
		self_status = get_status(flow_info, self_type);
		if (self_status != OBJ_ST_PURGED){
				
			set_status(flow_info, self_type, OBJ_ST_PURGED);
			syslog(LOG_INFO, "Info: Hedged_Req: Completed by other replica."
					 " Triggering self termination. %d,%d,%d,%d",
							jid, tid, prio, self_type);
			return -1;

		}
		else{
			syslog(LOG_INFO, "Info: Hedged_Req: Completed by other replica."
					 " Already terminated by replica. %d,%d,%d,%d", 
							jid, tid, prio, self_type);

			return -2;
		}
	}
	syslog(LOG_INFO, "Purge_Issue: Replica is in progress. oid=%d, self_type=%d\n",
									oid, self_type);

	return 1;	
}
int handle_appto_req(struct flow* flow_info, int self_type, int rep_type)
{
	int jid = flow_info->job_idx; 
	int tid = flow_info->task_idx;
	int prio = flow_info->flow_prio;

	int rep_status, self_status, purged;
	rep_status = get_status(flow_info, rep_type);

       	if (rep_status == OBJ_ST_FINISHED){
		self_status = get_status(flow_info, self_type);
                if (self_status != OBJ_ST_PURGED){

                	set_status(flow_info, self_type, OBJ_ST_PURGED);
			syslog(LOG_INFO, "Info: AppTO_Req: Completed by other replica."
					 " Triggering self purge. %d,%d,%d,%d", 
							jid,tid,prio,self_type);

			return -1;
               	}
                else{
			syslog(LOG_INFO, "Info: AppTO_Req: Completed by other replica."
					 " Already purged by replica. %d,%d,%d,%d", 
							jid,tid,prio,self_type);

			return -2;
		}
	}
        else{
		pthread_mutex_lock(&flow_info->task_ptr->task_lock);
                purged = purge_replica(flow_info);
                pthread_mutex_unlock(&flow_info->task_ptr->task_lock);

                if (purged){
                	set_status(flow_info, rep_type, OBJ_ST_PURGED);
			syslog(LOG_INFO, "Info: AppTO_Req: Purging replica."
					 " %d,%d,%d,%d\n", jid,tid,prio,self_type);

			return 1;
		}else{
                	syslog(LOG_INFO,"Info: AppTO_Req: Replica Purge Failed. "
                        		"%d,%d,%d,%d \n",jid,tid,prio,self_type);
                       
                	return 2;
		}
	}

}
int handle_tied_req(struct flow* flow_info, int self_type, int rep_type)
{
	int jid = flow_info->job_idx; 
	int tid = flow_info->task_idx;
	int prio = flow_info->flow_prio;

	int rep_status;

        rep_status = flow_info->task_ptr->status[rep_type];
        if (rep_status == OBJ_ST_INPROG ||
		rep_status == OBJ_ST_FINISHED){
		flow_info->task_ptr->status[self_type] = OBJ_ST_PURGED;
		syslog(LOG_INFO,"Info: Tied_Req: Rep-%d is in progress."
     			" (%d,%d,%d,%d)\n",rep_type, jid, tid, prio, self_type);

		return -1;
	}
       	else{
		flow_info->task_ptr->status[self_type] = OBJ_ST_INPROG;
		syslog(LOG_INFO,"Info: Tied_Req: Rep-%d will continue."
     			" (%d,%d,%d,%d)\n",self_type, jid, tid, prio,self_type);

                //int purged = purge_replica(flow_info);
		
                /*if (purged){
                	set_status(flow_info, rep_type, OBJ_ST_PURGED);
			syslog(LOG_INFO,"Info: Tied_Req: Purging replica."
     					" (%d,%d,%d,%d)\n",jid, tid, prio,self_type);
				return 1;

                }else{
                	syslog(LOG_INFO,"Info: Tied_Req: Replica purge failed."
                        	"(%d,%d,%d,%d)\n", jid,tid,prio,self_type);
                        	return 2;
                }*/
        }
	return 0;
}

int update_global_counters(int server, int status, int type, int change)
{
	syslog(LOG_INFO,"Info: update_counters: status=%d, server=%d, type=%d, change=%d\n",
					status, server, type, change);
	if (status == JOB_IN_FLIGHT){
		if (change == INCREMENT)
			__sync_fetch_and_add(&g_counter_stats[server].in_flight[type].value,1);
		else if (change == DECREMENT)
			__sync_fetch_and_sub(&g_counter_stats[server].in_flight[type].value,1);

	}
	else if(status == JOB_PURGED){
		if (change == INCREMENT)
			__sync_fetch_and_add(&g_counter_stats[server].purged[type].value,1);
		else if (change == DECREMENT)
			__sync_fetch_and_sub(&g_counter_stats[server].purged[type].value,1);

	}
	else if (status == JOB_FINISHED){
		if (change == INCREMENT)
			__sync_fetch_and_add(&g_counter_stats[server].completed[type].value,1);
		else if (change == DECREMENT)
			__sync_fetch_and_sub(&g_counter_stats[server].completed[type].value,1);
	
	}
	else if (status == JOB_ERROR){
		if (change == INCREMENT)
			__sync_fetch_and_add(&g_counter_stats[server].failed[type].value,1);
		else if (change == DECREMENT)
			__sync_fetch_and_sub(&g_counter_stats[server].failed[type].value,1);
	}		
	return 0;
}
int get_counter_value(int st_type, int cnt_type, int server_id)
{
	int cnt_value=0;
	if (st_type == JOB_IN_FLIGHT){
		cnt_value = g_counter_stats[server_id].in_flight[cnt_type].value;
	}
	else if (st_type == JOB_PURGED){
		cnt_value = g_counter_stats[server_id].purged[cnt_type].value;
	}
	else if (st_type == JOB_FINISHED){
		cnt_value = g_counter_stats[server_id].completed[cnt_type].value;
	}
	else if (st_type == JOB_ERROR){
		cnt_value = g_counter_stats[server_id].failed[cnt_type].value;
	}

	return cnt_value;
}
int update_status (struct flow *flow_info, int scheme, int bytes_recvd, int obj_size)
{
	int last_status, status, rep_status, purged, rep_type;

	int jid = flow_info->job_idx; 
	int tid = flow_info->task_idx;
	int prio = flow_info->flow_prio;
	int type = flow_info->flow_type;
	int oid = flow_info->task_ptr->object_id;
	int sid = flow_info->task_ptr->server_id[type];


	last_status = get_status(flow_info, type);

	
	if (last_status == OBJ_ST_PURGED){
		status = last_status;
		
		update_global_counters(sid, JOB_PURGED, type, INCREMENT);
		syslog(LOG_INFO,"Info: update_status: Task is purged."
				" Bytes recvd=%d (%d,%d,%d,%d,%d)\n", 
				bytes_recvd, jid, tid, oid, prio,type);
	
	}
	else if ((last_status == OBJ_ST_INPROG) && 
		(bytes_recvd == obj_size)){

		set_status(flow_info, type, OBJ_ST_FINISHED);
		status = OBJ_ST_FINISHED;

		update_global_counters(sid, JOB_FINISHED, type, INCREMENT);
		syslog(LOG_INFO,"Info: update_status: Task is finished."
				" Bytes recvd=%d (%d,%d,%d,%d,%d)\n", 
				bytes_recvd, jid, tid, oid, prio,type);

	}
	else if ((last_status == OBJ_ST_INPROG) && 
		(bytes_recvd != obj_size)){

		set_status(flow_info, type, OBJ_ST_ERROR);
		status = OBJ_ST_ERROR;
		
		update_global_counters(sid, JOB_ERROR, type, INCREMENT);
		syslog(LOG_INFO,"Info: update_status: Error on replica."
					" Bytes recvd=%d (%d,%d,%d,%d,%d)\n", 
					bytes_recvd, jid, tid, oid, prio,type);

	}
	
	if (scheme == SCHEME_PURGE || scheme == SCHEME_HEDGEDREQ){	
	//if (scheme == SCHEME_PURGE){	
			
		rep_type = get_rep_type(type);
		rep_status = get_status(flow_info, rep_type); 
		
		if ((rep_status == OBJ_ST_INPROG 
			|| rep_status == OBJ_ST_WAIT)
			&& (status == OBJ_ST_FINISHED)){

			pthread_mutex_lock(&flow_info->task_ptr->task_lock);
			purged = purge_replica(flow_info);
			pthread_mutex_unlock(&flow_info->task_ptr->task_lock);
			
			if (purged){
				set_status(flow_info, rep_type, OBJ_ST_PURGED);
	
				syslog(LOG_INFO,"Info: Replica Purged."
						" (%d,%d,%d,%d,%d)\n",
						jid, tid, oid, prio,type);

			}else{
				syslog(LOG_INFO,"Info: Replica Purge Failed. "
						"(%d,%d,%d,%d,%d)\n", 
						jid, tid,oid, prio,type);
			}
		}
	}
	return status;
}
void set_status(struct flow *flow_info, int type, int status){

	pthread_mutex_lock(&flow_info->task_ptr->task_lock);
	flow_info->task_ptr->status[type] = status;
	pthread_mutex_unlock(&flow_info->task_ptr->task_lock);

}
int get_status(struct flow *flow_info, int type)
{
	int status = -1;

	pthread_mutex_lock(&flow_info->task_ptr->task_lock);
	status = flow_info->task_ptr->status[type];
	pthread_mutex_unlock(&flow_info->task_ptr->task_lock);
	
	return status;
}

//Server Queue Monitor Patch Start
int update_server_load(int server_id, int req_type, int update)
{
	int count=0;
	pthread_mutex_lock(&g_server_queue_lock[server_id]);
	g_server_queue_count[req_type][server_id] += update;
	count = g_server_queue_count[req_type][server_id];
	pthread_mutex_unlock(&g_server_queue_lock[server_id]);
	return count;
}
//Server Queue Monitor Patch End
