# include <stdio.h>
# include <stdlib.h>
#include <semaphore.h>

# include "queue.h"
struct config
{
	int server_id;
	int pri_port;
	int sec_port;
	
	int pri_max_conn;
	int sec_max_conn;
	
	int qmon_interval;
	int data_chunk_size;

	int chunksize;
	int caching;

	int max_q_len[2];
	int noise;
	int prio[2];
	int mean_file_size;
	int bottleneck_bw;
	int hdfs_proxy_port;

	int *noise_vector;	
	
	char ip_addr[20];
	char sched_mode[20];
	
	char data_dir[100];
	char noise_data_dir[100];
	char noise_cdf[100];	
	char noise_cdf_file[1000];
};

struct proto
{
	int type;
	int prio;
	int object_id;
	int start_idx;
	int size;
};
struct job_data
{
        int sock;
        int prio;
        int object_id;
        int start_idx;
        int size;
};

struct server
{
	int port;
	int file_id;
	int serv_class;
	char ip_addr[20];
	
};
struct dispatcher_data
{
	int serv_class;	
};
struct read_channel
{
	int prio;
	int max_sock;
	int mon_interval;
};


# define DEBUG 1
# define TRUE 1
# define FALSE 0

# define PRIMARY 0
# define SECONDARY 1
# define DUPLICATES 2

# define DISK_CLASS_RT		1
# define DISK_CLASS_IDLE	2
# define DISK_CLASS_BE		0

# define REQUEST_GETFILE 1
# define REQUEST_PURGE   2
# define REQUEST_ACCEPT  11
# define REQUEST_REJECT  12
# define REQUEST_NOTFOUND  13

# define ST_ACCEPT		2
# define ST_PURGE_RECVD		1
# define ST_ERR_CONN_CLOSED	0
# define ST_ERR_QUEUE_OVERFLOW 	-1
# define ST_ERR_COM_FAILURE	-2
# define ST_ERR_FILE_NOT_FOUND	-3


# define PRI_MAX_QUEUE_LEN	2000
# define SEC_MAX_QUEUE_LEN	50

# define FILESIZE	10000000
# define CHUNKSIZE	64000
# define MSGSIZE sizeof(struct proto)

# define TRANSFER 1
# define ERROR -1

# define CONFIGDIR	"../CONFIG/"
# define CONFIGFILE	"server.conf"
# define DATADIR	"/mnt/extra/data/"

# define MAXEPOLLEVENTS	2000
# define MAXCONNECTBACKLOG 100
# define MAXCDFRES 100

struct noise *noise_data;
struct config g_config;
struct Queue *Req_Queue[2];
//struct Queue *Req_Queue_Sec;

int g_job_in_prog[2];
int curr_conn_count[DUPLICATES];
int g_req_q_len[DUPLICATES];
int epoll_fd[DUPLICATES];
int PRIORITY_CLASS[DUPLICATES];
int queue_len[DUPLICATES];
int g_noise_cdf[1000];
int accept_count, accept_count_err;

pthread_mutex_t QUEUE_LOCK[DUPLICATES];
pthread_mutex_t EPOLL_CTL_LOCK[DUPLICATES];
pthread_mutex_t COUNT_LOCK[DUPLICATES];
pthread_mutex_t CONN_QUEUE_MOD_LOCK[DUPLICATES];

char ip_addr[20];

sem_t job_in_sys_mutex;
sem_t job_limit_mutex;

void bootstrapper();
int read_config();
int dans_send(int sock, char *msg, int size);
int dans_recv(int sock, char* msg, int msgsize);
int send_file (struct job_data* j_data);

void make_msg_hdr(char *hdr_buf, int, int, int, int, int);
void parse_msg_hdr (struct proto *msg_hdr, char *hdr_buf);
void make_queue_obj(struct queue_data *q_data, int, int, int, int, int);

int enqueue_data (struct queue_data data, int priority);
int get_data_array(char tuple[], int *array, int max_array_size);
void close_worker (int priority);
int get_noise_latency(int object_size);
int make_blocking (int socket);
int make_non_blocking (int new_sock);
int dans_accept(int listen_sock);
int dans_com_handler(int com_sock, int priority);
int get_dispatch_limit();

int handle_hdfs_get(int com_sock, struct proto *msg_hdr, int priority);
int handle_not_found(int com_sock, struct proto *msg_hdr);
int handle_enqueue_failure(int com_sock, struct proto *msg_hdr, int priority);
int handle_enqueue_success(int com_sock, struct proto *msg_hdr, int filesize);

void* server(void *ptr);
void* disk_worker(void* ptr);
void* hdfs_worker(void* ptr);
void* memory_worker(void* ptr);
void* qmon(void* ptr);
void* sockmon(void* ptr);
void* disk_scheduler(void* ptr);
void* noise_generator(void* args);
