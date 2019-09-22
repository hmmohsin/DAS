# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <pthread.h>
# include <ncurses.h>
# include <unistd.h>
# include "client.h"

void* monitor(void *arg)
{
	int idx, x,y, job_count=0;
	char *status_buffer = malloc(1000), *scheme_name=malloc(100);
	int scheme, load, run;
	
	initscr();
	do{	
		
		job_count = e_config.job_count;
		scheme = e_config.scheme;
		load = e_config.load;
		run = e_config.run;
		get_scheme_name(scheme, scheme_name);
		
		memset(status_buffer, 0, 1000);
		printw("================ EXPERIMENT INFO =================\n");
		printw("      SCHEME=%s, LOAD=%d, RUN=%d COUNT=%d\n",
						scheme_name,load,run, job_count);
		printw("===================== STATUS =====================\n");
		
		getyx(stdscr, y, x);
		move(y+1, 10);
		printw("IN-FLIGHT");
		getyx(stdscr, y,x);
		move(y, x+5);
		printw("COMPLETED");
		getyx(stdscr, y,x);
		move(y, x+5);
		printw("PURGED");
		getyx(stdscr, y,x);
		move(y, x+5);
		printw("DROPPED");

		getyx(stdscr, y,x);
		for(idx = 1; idx <= g_config->dn_count; idx++){
			move(y+idx, 0);
			printw("Server-%d", idx);
			//getyx(stdscr, y,x);
			move(y+idx, 12);
			printw("%d_%d",g_counter_stats[idx-1].in_flight[PRIMARY].value,
					g_counter_stats[idx-1].in_flight[SECONDARY].value);

			//getyx(stdscr, y,x);
			move(y+idx, 25);
			printw("%d_%d", g_counter_stats[idx-1].completed[PRIMARY].value,
					g_counter_stats[idx-1].completed[SECONDARY].value);

			//getyx(stdscr, y,x);
			move(y+idx, 39);
			printw("%d_%d",g_counter_stats[idx-1].purged[PRIMARY].value,
					g_counter_stats[idx-1].purged[SECONDARY].value);

			//getyx(stdscr, y,x);
			move(y+idx, 52);
			printw("%d_%d",g_counter_stats[idx-1].failed[PRIMARY].value,
					g_counter_stats[idx-1].failed[SECONDARY].value);

		}
		refresh();
		sleep(1);
		clear();
	}while(g_exp_status == ST_RUNNING);

	endwin();
	system("clear");

	printf("================ EXPERIMENT INFO =================\n");
	printf("          SCHEME=%s, LOAD=%d, RUN=%d\n",scheme_name,load,run);
	printf("===================== STATUS =====================\n");

	for(idx = 0; idx < g_config->dn_count; idx++){
		printf("Server-%d\t%d_%d\t%d_%d\t%d_%d\t%d_%d\n",idx,
			g_counter_stats[idx].in_flight[PRIMARY].value,
			g_counter_stats[idx].in_flight[SECONDARY].value,
			g_counter_stats[idx].completed[PRIMARY].value,
			g_counter_stats[idx].completed[SECONDARY].value,
			g_counter_stats[idx].purged[PRIMARY].value,
			g_counter_stats[idx].purged[SECONDARY].value,
			g_counter_stats[idx].failed[PRIMARY].value,
			g_counter_stats[idx].failed[SECONDARY].value);

	}

	printf("Experiment finished\n");
	
	free(status_buffer);
	free(scheme_name);
	pthread_detach(pthread_self());
	return NULL;
}
