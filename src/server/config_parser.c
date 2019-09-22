#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

# include "server.h"

int read_config()
{
	FILE *fp = NULL;
	char tuple[512] 	= {0};
	char key[50]		= {0};
	char config_filename[50] = {0};
	char data[100];
	
	int value;

        strcpy(config_filename, CONFIGDIR);
        strcat(config_filename, CONFIGFILE);

	fp = fopen(config_filename,"r");
	if (!fp){
		printf("Error:: Failed to open config file.\n");
		return 0;
	}
	while(fgets(tuple, sizeof(tuple), fp) != NULL)
	{
		sscanf(tuple, "%s %d", key, &value);
		if (strcmp(key, "pri_port") == 0){
			g_config.pri_port = value;
			printf("Config: pri_port is %d\n", g_config.pri_port);
		}
		else if (strcmp(key, "sec_port") == 0){
			g_config.sec_port = value;
			printf("Config: sec_port is %d\n", g_config.sec_port);
		}
		else if (strcmp(key, "pri_max_conn") == 0){
			g_config.pri_max_conn = value;
			printf("Config: pri_max_conn limit is %d\n", g_config.pri_max_conn);
		}
		else if (strcmp(key, "sec_max_conn") == 0){
			g_config.sec_max_conn = value;
			printf("Config: sec_max_conn limit is %d\n", g_config.sec_max_conn);
		}
		else if (strcmp(key, "chunksize") == 0){
			g_config.chunksize = value;
			printf("Config: chunksize is %d\n", g_config.chunksize);
		}
		else if (strcmp(key, "qmon_interval") == 0){
			g_config.qmon_interval = value;
			printf("Config: qmon_interval is %d\n", g_config.qmon_interval);
		}
		else if (strcmp(key, "caching") == 0){
			g_config.caching = value;
			printf("Config: caching is %d\n", g_config.caching);
		}
		else if (strcmp(key, "pri_prio") == 0){
			g_config.prio[PRIMARY] = value;
			printf("Config: primary priority is %d\n", g_config.prio[PRIMARY]);
		}
		else if (strcmp(key, "sec_prio") == 0){
			g_config.prio[SECONDARY] = value;
			printf("Config: Secondary priority is %d\n", g_config.prio[SECONDARY]);
		}
		else if (strcmp(key, "pri_max_q_len") == 0){
			g_config.max_q_len[PRIMARY] = value;
			printf("Config: Max queue len is %d\n", g_config.max_q_len[PRIMARY]);
		}
		else if (strcmp(key, "sec_max_q_len") == 0){
			g_config.max_q_len[SECONDARY] = value;
			printf("Config: Max queue len is %d\n", g_config.max_q_len[SECONDARY]);
		}
		else if (strcmp(key, "mean_file_size") == 0){
			g_config.mean_file_size = value;
			printf("Config: Mean file size is %d(KB)\n", g_config.mean_file_size);
		}
		else if (strcmp(key, "bottleneck_bw") == 0){
			g_config.bottleneck_bw = value;
			printf("Config: bottleneck bw is %d(KB)\n", g_config.bottleneck_bw);
		}
		else if (strcmp(key, "sched_mode") == 0){
			sscanf(tuple, "%s %s", key, data);
			strcpy(g_config.sched_mode, data);
			printf("Config: Scheduling mode is %s\n", g_config.sched_mode);
		}
		else if (strcmp(key, "data_dir") == 0){
			sscanf(tuple, "%s %s", key, data);
			strcpy(g_config.data_dir, data);
			printf("Config: Data directory is %s\n", g_config.data_dir);
		}

	
		else if (strcmp(key, "sched_mode") == 0){
			sscanf(tuple, "%s %s", key, data);
			strcpy(g_config.sched_mode, data);
			printf("Config: Sched_mode is %s\n", g_config.sched_mode);
		}
		else if (strcmp(key, "hdfs_proxy_port") == 0){
			g_config.hdfs_proxy_port = value;
			printf("Config: hdfs_proxy_port is %d\n", g_config.hdfs_proxy_port);
		}

		else if (strcmp(key, "noise_mfs") == 0){
			sscanf(tuple, "%s %s", key, data);
			g_config.noise = value;
			printf("Config: noise option is %d\n", g_config.noise);
		}
		else
			printf("Warning: Unknown identifier '%s'\n",key);
	}

	fclose(fp);
	return 1;
}
