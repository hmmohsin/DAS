# include <stdio.h>
# include <stdlib.h>
# include <ncurses.h>
# include <pthread.h>

# include <sys/stat.h>
# include <sys/signal.h>
# include <syslog.h>
# include <unistd.h>
# include "client.h"
# include "queue.h"

int main(int argc, char* argv[])
{

	int client_id = 0;
	if (argc < 2){
		printf("Usage: ./client <Client-ID>\n");
		exit(0);
	}
	else{
		client_id = atoi(argv[1]);
	}

	g_config = malloc(sizeof(struct config));
	if (g_config == NULL){
		printf("Error: Client: Malloc failed to allocate memory for g_config\n");
		exit(0);
	}
	else{
		g_config->client_id = client_id;
	}

	struct server *server_list=NULL;
	struct job *job_store;
        struct  Queue *queue = malloc(sizeof(struct Queue));
	if (queue == NULL){
		printf("Error: Client: Malloc failed to allocate memory for queue\n");
		exit(0);
	}

	int job_count, scheme, load, run;

	pthread_t mon_handle;
	g_curr_job_count_inc = 0;
	g_curr_job_count_dec = 0;	
        create_queue(queue);
	
	configParser(g_config, server_list);
	bootstrap();
	g_counter_stats = malloc(g_config->dn_count * sizeof(struct counters));	
	if (g_counter_stats == NULL){
		printf("Error: Client: Malloc failed to allocate memory g_counters\n");
		exit(0);
	}

	
	g_exp_status = ST_RUNNING;
	g_file_meta = malloc(g_config->file_data_set * sizeof (struct metadata));
	if (g_file_meta == NULL){
		printf("Error: Client: Malloc failed to allocate memory for g_file_meta\n");
		exit(0);
	}

	g_file_count = load_file_metadata(g_config->file_data_set);
        printf ("g_file_count=%d\n",g_file_count);
	

	pthread_create(&mon_handle, NULL, monitor, (void*)NULL);
	
	for (scheme=0; scheme<g_config->schemes_count; scheme++){
		for (load=0; load<g_config->load_count; load++){
			for(run=0; run<g_config->run_count; run++){

				e_config.scheme = g_config->schemes[scheme];
				e_config.load = g_config->load[load];
				e_config.run = run;
				strcpy(e_config.eid, g_config->eid);

				printf("Starting load generation\n");
				loadGenerator(g_config, e_config);
				printf("Load is generated\n");
				job_count = g_config->min_job_count;
				job_store = malloc(g_config->min_job_count * 
							sizeof (struct job));
				e_config.job_count = job_count;
				if (job_store == NULL){
					printf("Error: Client: Malloc failed to allocate memory for job_store\n");
					exit(0);
				}

				job_loader(g_config, job_store, e_config);
				dispatcher(g_config, job_store, e_config);
				res_compiler(g_config, job_store, e_config);
				
				flush_data_struct(job_store, g_config->min_job_count);	
				g_config->min_job_count = 0;
			
				sleep(10);

			}
		}
	}
	
	g_exp_status = ST_FINISHED;
	sleep(2);
	
	free(queue);
	free(g_config);
	free(g_counter_stats);
	return 0;
}
void bootstrap()
{
	struct stat sb;
	
	signal(SIGPIPE, sigpipe_callback_handler);
	if (stat(CONFIGDIR, &sb) == -1){
		printf("Error: Config file not found.\n");
		exit(0);
	}
	if (stat(RESULT, &sb) == -1){
		mkdir(RESULT,0700);
		printf("Info: RESULTS directory not found. Creating..\n");

	}
	if (stat(LOADDIR, &sb) == -1){
		mkdir(LOADDIR,0700);
		printf("Info: LOAD directory not found. Creating..\n");

	}
	if (stat(RESDIR, &sb) == -1){
		mkdir(RESDIR,0700);
		printf("Info: ../RESULTS/RES directory not found. Creating..\n");

	}
	if (stat(LOGDIR, &sb) == -1){
		mkdir(LOGDIR,0700);
		printf("Info: ../RESULTS/LOG directory not found. Creating..\n");

	}
	if (stat(DISPDIR, &sb) == -1){
		mkdir(DISPDIR,0700);
		printf("Info: ../RESULTS/RES/DISP directory not found. Creating..\n");

	}
	if (stat(CDFDIR, &sb) == -1){
		mkdir(CDFDIR,0700);
		printf("Info: ../RESULTS/CDF directory not found. Creating..\n");
	}

}
void sigpipe_callback_handler(int signum){

        syslog(LOG_INFO, "Info: Caught signal SIGPIPE %d\n",signum);
}
