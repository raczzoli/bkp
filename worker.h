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

#ifndef WORKER_H
#define WORKER_H

#define MAX_THREADS 6
#define MAX_JOBS 50

struct worker_job {
	void *data;
	int (*handler)(void *data);
};

int reg_worker_job(void *job_data, int (*handler)(void *data));
int get_nr_running_workers();
int wait_for_workers();

#endif
