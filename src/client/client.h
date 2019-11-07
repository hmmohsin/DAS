#ifndef CLIENT_H
#define CLIENT_H

# include <stdlib.h>
# include <stdio.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <pthread.h>
# include <sys/resource.h>

struct server
{
	char ip_addr[20];
        int id;
        int port_no[2];
};

struct config
{	
	char replica_imbalance[50];
	char file_read[50];
	char metadata_file[50];
	char eid[50];

        int *load;
        int *schemes;                   //0->Base, 1->Duplicate, 2->Priority
	struct server* servers;
 
        int dn_count;
        int cli_count;
        int btlnk_bw;
        int file_size;
       	int flow_arrival;               //0->ClosedLoop, 1->POISSON, 2->UNIFORM, 3->Pri-CL, Sec-POISSON, 4->Pri-POISSON, Sec-CL
        
	int schemes_count;
	int load_count;

	int sim_time;
        int min_job_count;
	int total_task_count;
        int file_data_set;

	int tasks_per_job;	
        int run_count;
	int hedged_wait;

	int client_id;
};

struct exp_config
{
	char eid[50];
	int scheme;
	int load;
	int run;
	int job_count;
};

//for the time client class and client id are same
struct thread_args
{
	int server_id;
        int load_index;
        int file_id;
        int sched_class;
	int scheme;
	int flow_class;
};


struct file_fetch_stats
{
	int recvd_file_size;
	long int read_lock_penalty;
};


struct task{
	
	struct sockaddr_in serv_addr[2];
	long int duration[2];
	int bytes_recvd[2];

	int server_id[2];
	int com_sock[2];
	int task_prog[2];
	int status[2];

	int task_id;
	int job_id;
	int object_id;
	int object_size;
	
 	// SUPPORT FOR ANALYSIS
	int bytes_read;
	int bytes_read_perc;
	int server_curr_load[2];
	
	
	pthread_mutex_t task_lock;

};

struct job{

	struct task *tasks;	
	
	int job_id; 
	int tasks_count;
	int sleep;
	int job_sem; 
	
	volatile int curr_tasks_count;
	volatile int job_status;       
	
	pthread_mutex_t status_lock;
	pthread_mutex_t tasks_count_lock;
	
};

struct flow{

	struct task* task_ptr;
	struct job* job_ptr;
	int job_idx;
	int task_idx;
	int flow_prio;
	int flow_type;

};

struct metadata{
	int replicas[3];
};

struct skewed{
	int min;
	int max;
};

struct __attribute__((__packed__)) header{

        int type;	//Message Type
        int prio;	//Priority of the replica
        int object_id;	//File-ID or Object-ID. Needs to be replaced with a string
        int start_idx;	//Read start point
        int size;	//Size of the data to read
};

struct counter
{
	volatile int value;
	pthread_mutex_t lock;
};
struct counters
{
	struct counter in_flight[2];
	struct counter completed[2];
	struct counter purged[2];
	struct counter failed[2];
	struct counter bytes_read[2];
	
};

enum priority
{
	primary, secondary
};

# define WL_PS_CLOSEDLOOP		0
# define WL_PS_POISSON 			1
# define WL_PS_UNIFORM			2	
# define WL_P_CLOSEDLOOP_S_POISSON	3
# define WL_P_POISSON_S_CLOSEDLOOP	4

# define MEANTASKCOUNT			4

# define INCREMENT			1
# define DECREMENT			-1


# define JOB_IN_FLIGHT			1
# define JOB_PURGED			2
# define JOB_FINISHED			3
# define JOB_ERROR			4 

# define OBJ_ST_ISSUED			0
# define OBJ_ST_WAIT			1
# define OBJ_ST_INPROG			2
# define OBJ_ST_FINISHED		3
# define OBJ_ST_PURGED			4
# define OBJ_ST_ERROR			5


# define WL_CLOSEDLOOP			0
# define WL_POISSON			1
# define WL_UNIFORM			2

# define SCHEME_BASE			0
# define SCHEME_DUP			1
# define SCHEME_PRIO			2
# define SCHEME_PURGE			3
# define SCHEME_BYTEAGG			4
# define SCHEME_HEDGEDREQ		5
# define SCHEME_APPTO                   7
# define SCHEME_TIED                    9



# define REQUEST_GETFILE 1
# define REQUEST_PURGE   2
# define REQUEST_ACCEPT  11
# define REQUEST_REJECT  12
# define REQUEST_NOTFOUND  13

# define PRIMARY 0
# define SECONDARY 1

# define FLOW_ARRIVAL_POISSON		1
# define FLOW_ARRIVAL_UNIFORM		2

# define CONFIGDIR	"../CONFIG/"
# define LOADDIR	"LOAD/"
# define RESULT		"../RESULTS"
# define RESDIR		"../RESULTS/RES/"
# define LOGDIR 	"../RESULTS/LOG/"	 
# define CDFDIR		"../RESULTS/CDF"
# define DATADIR	"/mnt/extra/data/F"
# define DISPDIR	"../RESULTS/LOG/DISP/"
# define ANALYSISDIR	"../RESULTS/ANALYSIS/"

# define CONFIGFILE	"exp.conf" 
# define SERVERFILE	"servers.list"
# define PRILOADFILE	"pri_load.txt"
# define SECLOADFILE	"sec_load.txt"

# define MSGSIZE sizeof(struct header)

# define FETCH_BUFF_SIZE	64000
# define FILEDATASET		5000

# define ST_RUNNING		1
# define ST_FINISHED		2


int g_exp_status;
volatile int g_curr_job_count_inc;
volatile int g_curr_job_count_dec;
int g_file_count;
int *g_server_queue_count[2];

struct config *g_config;
struct exp_config e_config;
struct counters *g_counter_stats;
struct metadata *g_file_meta;
pthread_mutex_t g_job_count_lock;
pthread_mutex_t *g_server_queue_lock;


void *monitor(void *args);
void *com_handler(void *args);
void *job_manager(void *args);
void loadGenerator(struct config*, struct exp_config);
int configParser(struct config*, struct server*);
void job_loader(struct config*, struct job*, struct exp_config);
void dispatcher (struct config*, struct job*, struct exp_config);
int res_compiler (struct config*, struct job*, struct exp_config);
int get_counter_value(int st_type, int cnt_type, int server_id);

int get_connection (struct task*, int type, int prio);
void flush_data_struct(struct job *job_store, int job_count);

struct server get_server_info(int server_id);
void* com_handler (void *args);
int make_pkt (char* buf, int type, int prio, int obj_id, 
				int start_idx, int size);
int dans_send(int sock, char* msg_buf);
int dans_recv(int sock, char* msg_buf);
void dans_parse(char* msg_buff, struct header* header_data);
void get_scheme_name(int scheme, char *scheme_name);

void read_config();
int gen_load (int scheme, int load);
int gen_load_2 (int scheme, int load);
int gen_flows(int scheme_id, int load, int run, 
			int total_fetch_count);
double get_mean_iat(int load, int file_size, int btlnk_bw, 
			int cn_count, int dn_count, int tasks_per_job);
double rand_expo(double lambda);
int recv_file(struct flow *flow_info, char *buf, int scheme);
void *client_connection (void *args_ptr);
void get_class(char *class_name, int class_id);
void get_scheme(char *scheme_name, int scheme_id);
void get_file_name(char *filename, int scheme, int load, int run);
void compile_res(int scheme, int load, int run, int total_fetch_count);
int get_data_array(char tuple[], int *array, int max_array_size);

int purge_replica(struct flow* flow_info);
void set_status (struct flow*, int type, int status);
int get_status (struct flow*, int type);
int update_status(struct flow*, int scheme, int bytes_recvd, int obj_size);
void sigpipe_callback_handler(int signum);

int handle_appto_req(struct flow* flow_info, int self_type, int rep_type);
int handle_tied_req(struct flow* flow_info, int self_type, int rep_type);
int handle_hedged_req(struct flow* flow_info, int self_type, int rep_type);
int update_server_load(int server_id, int req_type, int update);

int get_rep_type(int type);
int get_file_replicas(int file_id, int *file_replicas);
int load_file_metadata(int file_count);
int update_global_counters(int server, int status, int type, int change);

int genRandRange(int lower, int upper);
int get_array_size(int array[]);
void bootstrap();
void test();
#endif
