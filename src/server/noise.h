# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <math.h>

struct noise
{
	int *bucket;
	double *bucket_value;
	
	char noise_data_dir[100];
	char noise_cdf_file[100];
};

struct load
{
	int file_id;
	int chunk_size;
	int noise_data_size;
	char noise_data_dir[20];
};

int load_noise_cdf(struct noise *noise_data);
int get_index(int r_num, int *bucket);
double get_time(double min, double max);
double get_noise_iat(struct noise *noise_data);
void *make_noise(void*);
