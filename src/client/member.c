# include <stdio.h>
# include <stdlib.h>

struct FLOW
{
	int fid;
	int tid;
	int jid;	
};

struct TASK 
{
	int tid;
	int data[2];
};
struct JOB 
{
	struct TASK* tasks;
	struct FLOW* flow;
	int jid;
	int data[2];
};

int main()
{
	struct JOB *jobs = malloc(100*sizeof(struct JOB));
	int i,j,t;
	for(i = 0; i < 100; i++){
		jobs[i].tasks = malloc(10*sizeof(struct TASK));
		jobs[i].flow = malloc(sizeof(struct FLOW));
	}
	
	for(j=0; j<100; j++){
		jobs[j].jid = j;
		for(t=0; t < 10; t++){
			jobs[j].tasks[t].tid = t;
			jobs[j].tasks[t].data[0] = 100*j+t;
			jobs[j].tasks[t].data[0] = 100*j+t+1;
			
		}
		jobs[j].flow->fid = 1;
		jobs[j].flow->tid = 1;
		jobs[j].flow->jid = j;

		jobs[j].data[0] = 100+j;
		jobs[j].data[1] = 200+j;
	}

	struct JOB *job = &jobs[10];

	printf("Test: Job->jid = %d\n", job->jid);
	printf("Test: Job->tasks[5].tid = %d\n", job->tasks[5].tid);
	printf("Test: Job->flow->fid = %d\n", job->flow->fid);
	printf("Test: Job->data[0] = %d\n", job->data[0]);
	
	job->tasks[5].tid = -5;


	struct TASK *task = &jobs[10].tasks[5];
	printf("Test: task->tid = %d\n", task->tid);
	return 0;
}
