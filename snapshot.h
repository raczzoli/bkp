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

#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <openssl/sha.h>
#include <time.h>
#include <sys/stat.h>

struct snapshot {
	unsigned char sha1[SHA_DIGEST_LENGTH];
	unsigned char parent_sha1[SHA_DIGEST_LENGTH];
	unsigned char tree_sha1[SHA_DIGEST_LENGTH];
	char date[20]; // YYYY-MM-DD HH:ii:ss\0
	time_t time;
};

int create_snapshot();
int list_snapshots(int limit);
int read_snapshot_file(unsigned char *sha1, struct snapshot *snapshot);
int read_snapshot_buffer(char *buff, struct snapshot *snapshot);
int print_snapshot_buffer(unsigned char *sha1, char *buff);

#endif
