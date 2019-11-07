# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/socket.h>

# include "client.h"


int configParser(struct config *g_config, struct server *server_list)
{
        FILE *fp = NULL;
        char tuple[512] = {0};
        char key[100] = {0};
        int value, count = 0;

        char config_filename[50] = {0};
        char server_filename[50] = {0};


        strcpy(config_filename, CONFIGDIR);
        strcat(config_filename, CONFIGFILE);

        strcpy(server_filename, CONFIGDIR);
        strcat(server_filename, SERVERFILE);

        fp = fopen(config_filename, "r");

        if (!fp){
		//printf("Error:: Failed to open config file\n");
		exit(0);
	}
        while (fgets(tuple, sizeof(tuple), fp) != NULL)
        {
                sscanf(tuple, "%s %d", key, &value);
                if (strcmp(key, "dn_count") == 0)
                        g_config->dn_count = value;
                else if (strcmp(key, "cli_count") == 0)
                        g_config->cli_count = value;
                else if (strcmp(key, "btlnk_bw") == 0)
                        g_config->btlnk_bw = value;
                else if (strcmp(key, "file_size") == 0)
                        g_config->file_size = value;
                else if (strcmp(key, "pLoad") == 0) {
                        g_config->load = malloc(10* sizeof(int));
                        g_config->load_count = get_data_array(tuple, g_config->load, 10);
                }
                else if (strcmp(key, "scheme") == 0){
                        g_config->schemes = malloc(6* sizeof(int));
                        g_config->schemes_count = get_data_array(tuple, g_config->schemes, 6);
		}
                else if (strcmp(key, "sim_time") == 0)
                        g_config->sim_time = value;
                else if (strcmp(key, "min_job_count") == 0)
                        g_config->min_job_count = value;
                else if (strcmp(key, "flow_arrival") == 0){
                        g_config->flow_arrival = value;
                }
                else if (strcmp(key, "file_data_set") == 0){
                        g_config->file_data_set = value;
                }
                else if (strcmp(key, "tasks_per_job") == 0){
                        g_config->tasks_per_job = value;
                }

		else if (strcmp(key, "run_count") == 0){
                        g_config->run_count = value;
                }
                else if (strcmp(key, "hedged_wait") == 0){
                        g_config->hedged_wait = value;
                }
		else if (strcmp(key, "eid") == 0){
			bzero(g_config->eid, 20);
                	sscanf(tuple, "%s %s", key, g_config->eid);
                }
		else if (strcmp(key, "replica_imbalance") == 0){
			bzero(g_config->replica_imbalance, 20);
                	sscanf(tuple, "%s %s", key, g_config->replica_imbalance);
                }
		else if (strcmp(key, "file_read") == 0){
			bzero(g_config->replica_imbalance, 20);
                	sscanf(tuple, "%s %s", key, g_config->file_read);
                }

		else if (strcmp(key, "metadata_file") == 0){
			bzero(g_config->metadata_file, 20);
                	sscanf(tuple, "%s %s", key, g_config->metadata_file);
                }
		else{
                        printf("Error: Unknwon key '%s'\n",key);
                }
        }
        fclose(fp);

        fp = fopen(server_filename, "r");
        if(!fp)
                printf("Error:: Failed to open server file\n");

        printf("HM_Debug: %d %ld\n", g_config->dn_count, sizeof(struct server));
	g_config->servers = malloc(g_config->dn_count * sizeof(struct server));
        count = 0;
        while (fgets(tuple, sizeof(tuple), fp) != NULL)
        {
                bzero(g_config->servers[count].ip_addr, 20);
                sscanf(tuple, "%d %s %d %d", &g_config->servers[count].id,
                                         g_config->servers[count].ip_addr,
                                         &g_config->servers[count].port_no[PRIMARY],
                                         &g_config->servers[count].port_no[SECONDARY]);

                printf("%d %s %d %d\n",  g_config->servers[count].id,
                                         g_config->servers[count].ip_addr,
                                         g_config->servers[count].port_no[PRIMARY],
                                         g_config->servers[count].port_no[SECONDARY]);

                count++;
                if(count > g_config->dn_count) {
                        printf("Error: Inconsistent Configuration files."
                                "Server list does not match dn_count=%d.\n",
				g_config->dn_count);
                        return -1;
                }
        }
        if(count != g_config->dn_count) {
                printf("Error: Inconsistent Configuration files."
                        "Server list does not match dn_count=%d.\n",
			g_config->dn_count);
        	return -1;
	}
        fclose(fp);
	return 0;
}
int get_data_array(char tuple[], int *array, int max_array_size)
{
        int offset = 0, count = 0, value, size;
        char key[25];

        sscanf(tuple + offset, "%s%n", key, &size);
        offset+= size;
        while(sscanf(tuple + offset, "%d%n", &value, &size) == 1){
                array[count] = value;
                offset += size;
                count++;
                if (count > max_array_size)
                        break;
        }
	
        return count;
}

