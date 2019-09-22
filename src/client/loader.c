# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <pthread.h>
# include <errno.h>
# include <assert.h>

# include <syslog.h>
# include "client.h"

void job_loader(struct config* g_config, struct job* job_store, struct exp_config e_config)
{
	int idx = 0, job_index, tasks_count=1;
	int job_count = g_config->min_job_count;
	char tuple[1024];
	int tuple_offset, tuple_read, task_id;
	int interarrival;
	int object_id, server_id[2], size, replica;

	char *filename = malloc(100);
	sprintf(filename, "%sload_%d", LOADDIR, e_config.load);
	FILE *fp = fopen(filename,"r");	
	
	for (idx=0; idx < job_count; idx++){

		memset(tuple,0,1024);	
		if(fgets(tuple, sizeof(tuple), fp) == NULL){
			syslog(LOG_INFO,"Error: job_loader. Malloc failed. exiting now.\n");
			exit(0);
		}
		sscanf(tuple,"%d%d%n", &job_index, &tasks_count, &tuple_read);

		job_store[idx].job_id = idx; 
		job_store[idx].tasks_count = tasks_count;		
		job_store[idx].curr_tasks_count = 0;		

	
		job_store[idx].tasks = malloc(tasks_count*sizeof(struct task));
				
	
			
		if (job_store[idx].tasks == NULL){
			syslog(LOG_INFO,"Error: job_loader. Malloc failed. exiting now.\n");
			exit(0);
		}	
		pthread_mutex_init(&job_store[idx].status_lock, NULL);
		pthread_mutex_init(&job_store[idx].tasks_count_lock, NULL);

		tuple_offset = tuple_read;

		for (task_id=0; task_id<tasks_count; task_id++){
			sscanf(tuple+tuple_offset, "%d%d%d%n", &object_id, 
				&server_id[0], &server_id[1], &size);

			/*object_id = 0;
			server_id[0] = 0;
			server_id[1] = 0;
			size = 10000;*/

			job_store[idx].tasks[task_id].job_id = idx;
			
			job_store[idx].tasks[task_id].task_id = task_id;
			
			job_store[idx].tasks[task_id].object_id = object_id;
			job_store[idx].tasks[task_id].object_size = 0;
			
			// SUPPORT FOR HEDGED ANALYSIS
			job_store[idx].tasks[task_id].bytes_read = 0;
			job_store[idx].tasks[task_id].bytes_read_perc = 0;
			job_store[idx].tasks[task_id].server_curr_load[PRIMARY] = 0;
			job_store[idx].tasks[task_id].server_curr_load[SECONDARY] = 0;
			
			
			pthread_mutex_init(&job_store[idx].tasks[task_id].task_lock, NULL);			

			for (replica=0; replica<2; replica++){		
				job_store[idx].tasks[task_id].server_id[replica] = 
									server_id[replica];
				job_store[idx].tasks[task_id].com_sock[replica] = 0;
				job_store[idx].tasks[task_id].task_prog[replica] = 0;
				job_store[idx].tasks[task_id].duration[replica] = 0;
				job_store[idx].tasks[task_id].bytes_recvd[replica] = 0;
				
				
				job_store[idx].tasks[task_id].status[replica] = OBJ_ST_ISSUED;
			}
		
			tuple_offset+= size;
		}
		sscanf(tuple+tuple_offset, "%d", &interarrival);
		job_store[idx].sleep = interarrival;
	
		//memset(job_store[idx].tasks, 1, tasks_count*sizeof(struct task));
	}

	for (idx = 0; idx < g_config->dn_count; idx++){
		for (replica = 0; replica<2; replica++){
			g_counter_stats[idx].in_flight[replica].value = 0;
			g_counter_stats[idx].completed[replica].value = 0;
			g_counter_stats[idx].purged[replica].value = 0;
			g_counter_stats[idx].failed[replica].value = 0;
			g_counter_stats[idx].bytes_read[replica].value = 0;

			pthread_mutex_init(&g_counter_stats[idx].in_flight[replica].lock, NULL);
			pthread_mutex_init(&g_counter_stats[idx].completed[replica].lock, NULL);
			pthread_mutex_init(&g_counter_stats[idx].purged[replica].lock, NULL);
			pthread_mutex_init(&g_counter_stats[idx].bytes_read[replica].lock, NULL);
			
		}
	}

	fclose(fp);
	free(filename);
}
