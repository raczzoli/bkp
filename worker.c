/* 
 * Copyright (C) 2025 Zoltán Rácz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.  
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "worker.h"

static int running = 0;
static pthread_mutex_t running_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t jobs_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t jobs_cond = PTHREAD_COND_INITIALIZER;

static struct worker_job **jobs = NULL;
static int jobs_len = 0;

static int add_job(struct worker_job *job);
static struct worker_job *get_next_job();

void *worker(void *arg)
{
	struct worker_job *job = NULL;

	while ((job = get_next_job()) != NULL) {
		job->handler(job->data);

		free(job);
		job = NULL;
	}

	pthread_mutex_lock(&running_lock);
	running--;
	pthread_mutex_unlock(&running_lock);

	return NULL;
}

int reg_worker_job(void *job_data, int (*handler)(void *data))
{
	int ret = 0;
	struct worker_job *job = malloc(sizeof(struct worker_job));

	if (!job) {
		fprintf(stderr, "Error allocating memory for worker job!\n");
		return -ENOMEM;
	}

	job->data = job_data;
	job->handler = handler;
	ret = add_job(job);

	if (ret) {
		free(job);
		return ret;
	}

	pthread_mutex_lock(&running_lock);

	if (running < MAX_THREADS) {
		running++;

		pthread_t thread;
		pthread_create(&thread, NULL, worker, NULL);
		pthread_detach(thread);	
	}

	pthread_mutex_unlock(&running_lock);

	return 0;
}

static struct worker_job *get_next_job()
{
	int idx = 0;
	struct worker_job *job = NULL;
	pthread_mutex_lock(&jobs_lock);

	if (jobs_len > 0) {
		idx = jobs_len - 1;
		job = jobs[idx];

		jobs[idx] = NULL;
		jobs_len--;
	}

	pthread_cond_broadcast(&jobs_cond);
	pthread_mutex_unlock(&jobs_lock);	

	return job;
}

static int add_job(struct worker_job *job)
{
	int ret = 0;
	pthread_mutex_lock(&jobs_lock);

	while (jobs_len >= MAX_JOBS) 
	    pthread_cond_wait(&jobs_cond, &jobs_lock); // releases lock here

	if (!jobs) 
		jobs = calloc(100, sizeof(struct worker_job *));
	else 
		if (jobs_len % 100 == 0)
			jobs = realloc(jobs, (jobs_len + 100) * sizeof(struct worker_job *));

	if (!jobs) {
		fprintf(stderr, "Error allocating memory for worker jobs array!\n");
		ret = -ENOMEM;
		goto end;
	}

	jobs[jobs_len] = job;
	jobs_len++;

end:
	pthread_mutex_unlock(&jobs_lock);

	return ret;
}
