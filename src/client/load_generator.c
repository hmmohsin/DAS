# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <math.h>
# include <syslog.h>
# include "client.h"

void loadGenerator(struct config *g_config, struct exp_config e_config)
{
	
        int job_count = 0, sleep=0, total_task_count = 0;
        int idx =0, itr = 0, task =0;
        int file_id=0, file_id_offset, weight=0, file_status=0;

        int server_id[2], rep_id;
	int job_id, task_count;
	double mean_iat = 0.0;
        char load_filename[50];
	char tasks_list[500]={0};
	char task_str[20];
	FILE *fp=NULL;
	int *file_replicas=malloc(3*sizeof(int));
	int min, max, skewed_idx, index;
    	
	struct skewed *sk_access;
	
	srand(1001);
	
	if (file_replicas == NULL){
		exit(0);
	}

	if(strcmp(g_config->file_read, "skewed")==0){

		FILE *sk_fp = fopen("CONFIG/skewed.txt","r");
		char tuple[100];
	
		if (sk_fp == NULL){
			syslog(LOG_INFO, "Error: Failed to open CONFIG/skewed.txt\n");
			exit(0);
		}
		
		sk_access = malloc(1000*sizeof(struct skewed));
		
		for (idx=0; idx<1000; idx++){
			if(fgets(tuple, sizeof(tuple), sk_fp) == NULL){
				syslog(LOG_INFO,"Error: loadGenerator() "
						" fgets failed. exiting now.\n");
				exit(0);
			}
			sscanf(tuple,"%d%d%d", &index, &sk_access[idx].min, &sk_access[idx].max);
		}
	}
	
	for (idx=0; idx < g_config->load_count; idx++){
                
		memset(load_filename, 0, 50);

		sprintf(load_filename, "%sload_%d", LOADDIR, e_config.load);

		fp = fopen(load_filename,"w+");
		
		mean_iat = get_mean_iat(e_config.load, g_config->file_size, 
					g_config->btlnk_bw, g_config->cli_count, 
					g_config->dn_count, g_config->tasks_per_job);
        	
		if ((g_config->sim_time/mean_iat) > g_config->min_job_count)
                	job_count = g_config->sim_time/mean_iat;
        	else
                	job_count = g_config->min_job_count;

		g_config->min_job_count = job_count;
        
		
		for (itr = 0; itr < job_count; itr++)
        	{

			memset(tasks_list, 0, 500);

                	job_id=itr;
			
			if (g_config->tasks_per_job == 0){
				task_count = (rand_expo(1/(MEANTASKCOUNT*1.0))) + 1;
				task_count=(task_count%8)+1;
			}
			else
				task_count=g_config->tasks_per_job;

			sprintf(tasks_list, "%d %d",job_id, task_count);
			
			for (task = 0; task < task_count; task++){
				memset(task_str, 0, 20);
				if (strcmp(g_config->file_read,"random") == 0){
					file_id = rand()%g_config->file_data_set;
				}
				else if (strcmp(g_config->file_read, "sequential")==0){
					
					file_id_offset = rand()%50;
                			if ((file_id_offset + file_id) >= g_config->file_data_set)
                	        		file_id = 0;
                			else
                	        		file_id += file_id_offset;
				}
				else if(strcmp(g_config->file_read, "skewed")==0){
					
					skewed_idx = rand()%1000;
					min = sk_access[skewed_idx].min;
					max = sk_access[skewed_idx].max-1;
			
					file_id = genRandRange(min, max);
						
				}

				
				if (g_config->dn_count == 1){
                        		server_id[PRIMARY] = 0;
					server_id[SECONDARY] = 0;
				}
				else{
					file_status = get_file_replicas(file_id, file_replicas);
					if (file_status == -1){
						printf("Error: get_file_replicas(): file F%d not found\n",
											file_id);
						exit(0);
					}	
						
					//printf("File-%d replicas are %d %d %d\n",file_id,
					//				file_replicas[0],
					//				file_replicas[1],
					//				file_replicas[2]);
					// SERVER IMBALANCE LOGIC GOES HERE
					// ONLY FOR 3 SERVER EXPERIMENTS
					if (strcmp(g_config->replica_imbalance,"random") == 0){
						rep_id = rand() % 3;
						server_id[PRIMARY] = file_replicas[rep_id];
					}
					else if (strcmp(g_config->replica_imbalance,"weighted") == 0){
						weight = rand()%100;
						if (weight < 50)
							server_id[PRIMARY] = file_replicas[0];
						else if (weight < 75)
							server_id[PRIMARY] = file_replicas[1];
						else
							server_id[PRIMARY] = file_replicas[2];
					}
					//else if (strcmp(g_config->replica_imbalance,"zipf") == 0){
					//	int zipf[3] = {66, 33, 22};
					//}
					// SERVER IMBALANCE LOGIC ENDS HERE
        				        			


					/* Makes sure that selected primary and 
					  secondary Replicas are different*/
	                        	

					do{
						rep_id = rand() % 3;
						server_id[SECONDARY] = file_replicas[rep_id];
	                        	        /*printf("FileID-%d, rep_id=%d, replicas=%d, %d, %d\n", file_id,
							rep_id, server_id[0], server_id[1],server_id[2]);*/

					
					}while(server_id[PRIMARY] == server_id[SECONDARY]);
        			}
				        		
				sprintf(task_str," %d %d %d", file_id, 
					server_id[PRIMARY], server_id[SECONDARY]);
				strcat(tasks_list, task_str);
				total_task_count++;
			}
                	if (g_config->flow_arrival == FLOW_ARRIVAL_POISSON)
                	       	sleep = (rand_expo(1/mean_iat)*1000000);
                	else if (g_config->flow_arrival == FLOW_ARRIVAL_UNIFORM)
                	        sleep = mean_iat*1000000;

                	fprintf(fp,"%s %d\n", tasks_list, sleep);
        	}
	}
	g_config->total_task_count = total_task_count;
        fclose(fp);
}

double get_mean_iat(int load, int file_size, int btlnk_bw, int cn_count,
				int dn_count, int mean_tasks_per_job){

	int mean_task_count = 0;
	if (mean_tasks_per_job == 0)
		mean_task_count = MEANTASKCOUNT;
	else
		mean_task_count = mean_tasks_per_job;

        double mean_IAT = ((cn_count * mean_task_count *file_size * 8.0 * 100) / 
				(1000 * load * (dn_count*btlnk_bw) * 8.0));

        return mean_IAT;
}
double rand_expo(double lambda){
        double u;
        u = rand() / (RAND_MAX + 1.0);
	return -log(1- u) / lambda;
}

