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

# include "uprio.h"
# include "server.h"

void bootstrapper()
{
	int i, dispatch_limit=0;
	struct stat sb;
	accept_count = 0;
	accept_count_err = 0;


	if (!read_config()){
		printf("Error: Failed to read configuration.\n");
		exit(0);
	}

	if (!(stat(g_config.data_dir, &sb) == 0 && S_ISDIR(sb.st_mode))
				&& (strcmp(g_config.sched_mode,"disk")==0)){
		printf("Error: Data directory does not exist. Exiting now.\n");
		exit(0);
	}
	
	for(i=0; i< DUPLICATES; i++){
		curr_conn_count[i] = 0;
		queue_len[i] = 0;
	
		pthread_mutex_init (&QUEUE_LOCK[i], NULL);
		pthread_mutex_init (&COUNT_LOCK[i], NULL);
		pthread_mutex_init (&CONN_QUEUE_MOD_LOCK[i], NULL);
		pthread_mutex_init (&EPOLL_CTL_LOCK[i], NULL);

		Req_Queue[i] = malloc(sizeof(struct Queue));
		
		PRIORITY_CLASS[i] = g_config.prio[i];		
	}
	create_queue(Req_Queue[PRIMARY], DISK_CLASS_RT);
	create_queue(Req_Queue[SECONDARY], DISK_CLASS_IDLE);
	dispatch_limit = get_dispatch_limit();
	
	sem_init(&job_in_sys_mutex, 0, 0);
	sem_init(&job_limit_mutex, 0, dispatch_limit);
		
	g_req_q_len[PRIMARY] = 0;
	g_req_q_len[SECONDARY] = 0;
}

int main(int argc, char *argv[])
{
	if (argc == 5 && (strcmp(argv[1], "-s") == 0))
	{
		bzero(g_config.ip_addr, 20);
		strcpy(g_config.ip_addr, argv[2]);
	}
	if (argc == 5 && (strcmp(argv[3], "-i") == 0))
	{
		g_config.server_id = atoi(argv[4]);
	}

	else{
		printf("Invalid Usage: type ./server -s <IP Address> -i <Server ID>\n");
		exit(0);
		
	}
 	
	pthread_t m_threads[4];
	struct server *server_inst[DUPLICATES];
	int i, id=0;

	openlog("slog", LOG_PID|LOG_CONS, LOG_LOCAL0);
	bootstrapper();

	for(i = 0; i<DUPLICATES; i++)
		server_inst[i] = malloc(sizeof(struct server));

	server_inst[PRIMARY]->port = g_config.pri_port;
	server_inst[PRIMARY]->serv_class = PRIMARY;
	strncpy(server_inst[PRIMARY]->ip_addr, g_config.ip_addr, 20);
	
	server_inst[SECONDARY]->port = g_config.sec_port;
	server_inst[SECONDARY]->serv_class = SECONDARY;
	strcpy(server_inst[SECONDARY]->ip_addr, g_config.ip_addr);


	if (pthread_create(&m_threads[id++], NULL, server, (void*)server_inst[PRIMARY])<0)
            printf("Error: Failed to start primary server\n");
	
	if (pthread_create(&m_threads[id++], NULL, server, (void*)server_inst[SECONDARY])<0)
            printf("Error: Failed to start primary server\n");

	if (pthread_create(&m_threads[id++], NULL, disk_scheduler, NULL) < 0)
            printf("Error: Failed to start scheduler\n");

	printf("Server Load: Primary(%d/%d) Secondary(%d/%d)\n",
		curr_conn_count[PRIMARY], queue_len[PRIMARY],
		curr_conn_count[SECONDARY], queue_len[SECONDARY]);
	
	while(1){
		printf("\rServer Load: Primary(%d/%d) Secondary(%d/%d)",
			curr_conn_count[PRIMARY], queue_len[PRIMARY], 
			curr_conn_count[SECONDARY], queue_len[SECONDARY]);
		usleep(10000);
	}
	closelog();
	printf("Info: Closing main\n");
}
int get_dispatch_limit()
{
	int limit = g_config.pri_max_conn + g_config.sec_max_conn;
	return limit;
}
