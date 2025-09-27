#ifndef WORKER_H
#define WORKER_H

#define MAX_THREADS 6
#define MAX_JOBS 50

struct worker_job {
	void *data;
	int (*handler)(void *data);
};

int reg_worker_job(void *job_data, int (*handler)(void *data));

#endif
