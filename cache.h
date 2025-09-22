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

#ifndef CACHE_H
#define CACHE_H

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdbool.h>
#include <openssl/sha.h>

struct cache_entry {
	mode_t st_mode;
	off_t st_size;
	struct timespec st_mtim;
	struct timespec st_ctim;
	unsigned char sha1[SHA_DIGEST_LENGTH];
	int path_len;
	char path[0];
};

struct cache {
	struct cache_entry **entries;
	int entries_len;
};

int update_cache(struct cache *cache);
struct cache *load_cache();
int find_cache_entry(struct cache *cache, char *path, int ret_insert_idx);
int find_cache_entry_insert_idx(struct cache *cache, char *path);
bool cache_entry_changed(struct cache_entry *entry, struct stat *stat);
int add_cache_entry(struct cache *cache, struct cache_entry *entry);


#endif
