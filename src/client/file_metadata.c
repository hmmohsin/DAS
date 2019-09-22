# include <stdio.h>
# include <stdlib.h>
# include "client.h"
# include <string.h>

int load_file_metadata(int file_count)
{
	char tuple[1000];
	int file_id, file_size, rep_1, rep_2, rep_3, count=0;
	FILE *meta_fp = fopen(g_config->metadata_file,"r");
	printf("Info: Opening file %s\n", g_config->metadata_file);
	while ((fgets(tuple, sizeof(tuple), meta_fp) != NULL) &&
		(count < file_count))
        {
                if (strcmp(tuple,"") != 0){
			sscanf(tuple, "%d %d %d %d %d", &file_id, &file_size, &rep_1, &rep_2, &rep_3);
			
			//printf("count=%d/%d, File-%d %d %d %d\n", 
			//	count, file_count, file_id, rep_1, rep_2, rep_3);
			g_file_meta[file_id].replicas[0] = rep_1;
			g_file_meta[file_id].replicas[1] = rep_2;
			g_file_meta[file_id].replicas[2] = rep_3;

			count++;
		}
	}
	return count;
}
