# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <pthread.h>

# include <sys/socket.h>
# include "client.h"


int res_compiler(struct config *g_config, struct job* job_store, struct exp_config e_config)
{
	char *res_buf;
	long *t_time[2], j_time[2];
	int bytes_recvd[2], object_size;
	int j_idx=0, t_idx=0, replica, server_id[2], server_load[2];
	char *res_file = malloc(100);
	char *scheme_name = malloc(50);	
	
	get_scheme_name(e_config.scheme, scheme_name);
	
	sprintf(res_file,"%s%s_%s_%d_%d_cli-%d",RESDIR, e_config.eid, 
			scheme_name, e_config.load, e_config.run, g_config->client_id); 		
	
	
	FILE *res_fp = fopen(res_file,"w");

	for(j_idx=0; j_idx < g_config->min_job_count; j_idx++){
		
		res_buf = malloc(1000);
		for (replica=0; replica<2; replica++){
			t_time[replica] = 
				malloc(g_config->min_job_count * sizeof(long));
			j_time[replica] = 0;
		}
		
		sprintf(res_buf,"%d %d ",j_idx, job_store[j_idx].tasks_count);
		
	
		for(t_idx=0; t_idx< job_store[j_idx].tasks_count; t_idx++){
			t_time[PRIMARY][t_idx] = 
				job_store[j_idx].tasks[t_idx].duration[PRIMARY];
			t_time[SECONDARY][t_idx] = 
				job_store[j_idx].tasks[t_idx].duration[SECONDARY];
	
			bytes_recvd[0] = 
				job_store[j_idx].tasks[t_idx].bytes_recvd[0];
			bytes_recvd[1] = 
				job_store[j_idx].tasks[t_idx].bytes_recvd[1];
			
			server_id[0] =  job_store[j_idx].tasks[t_idx].server_id[0];
			server_id[1] =  job_store[j_idx].tasks[t_idx].server_id[1];
		
			server_load[0] = job_store[j_idx].tasks[t_idx].server_curr_load[0];
			server_load[1] = job_store[j_idx].tasks[t_idx].server_curr_load[1];
			object_size = job_store[j_idx].tasks[t_idx].object_size;

			sprintf( res_buf+strlen(res_buf), 
				"|%d %d %d %d_%d %d_%d %ld_%ld %d_%d", t_idx, 
					job_store[j_idx].tasks[t_idx].object_id,
					object_size,
					server_id[PRIMARY],
					server_id[SECONDARY],
					bytes_recvd[PRIMARY], 
					bytes_recvd[SECONDARY],
					t_time[PRIMARY][t_idx],  
					t_time[SECONDARY][t_idx],
					server_load[PRIMARY],
					server_load[SECONDARY]); 
		}
		fprintf(res_fp, "%s\n",res_buf);
		
		free(res_buf);	
		free(t_time[PRIMARY]);
		free(t_time[SECONDARY]);
		
	}
	fclose(res_fp);
	return 0;
}
