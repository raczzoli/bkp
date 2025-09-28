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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "cache.h"
#include "file.h"

static int add_cache_entry_at(struct cache *cache, struct cache_entry *entry, int idx);
static void free_cache(struct cache *cache);

struct cache *load_cache()
{
	int fd = 0;
	int offset = 0;
	struct stat cstat;
	struct cache *cache = malloc(sizeof(struct cache));
	struct cache_entry *c = NULL;
	void *cmap = NULL;

	if (!cache) {
		fprintf(stderr, "Error allocating memory for cache!\n");
		return NULL;
	}

	cache->entries = NULL;
	cache->entries_len = 0;

	fd = open(".bkp-data/filecache", O_RDONLY);	
	if (fd < 0) 
		goto end; // not an error, it just doesn`t exist yet
	
	if (fstat(fd, &cstat)) {	
		fprintf(stderr, "Error calling fstat on filecache!\n");
		goto err;
	}
	
	cmap = mmap(NULL, cstat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (cmap == MAP_FAILED) {
		fprintf(stderr, "mmap failed while mapping filecache into memory!\n");
		goto err;
	}

	while(offset < cstat.st_size) {
		c = cmap + offset;
		offset += sizeof(struct cache_entry) + c->path_len+1;

		add_cache_entry_at(cache, c, cache->entries_len);
	}

	goto end;

err:
	free_cache(cache);
	return NULL;

end:
	if (fd >= 0)
		close(fd);

	return cache;
}

int update_cache(struct cache *cache)
{
	int fd = open(".bkp-data/filecache.new", O_WRONLY | O_CREAT | O_EXCL, 0666);
	int size = 0;

	if (fd < 0) {
		if (errno == EEXIST) 
			fprintf(stderr, "filecache.new already exists! Maybe another cache update in progress?\n");

		return -1;
	}

	if (cache->entries_len > 0) {
		for (int i=0;i<cache->entries_len;i++) {
			struct cache_entry *c = cache->entries[i];
			size = sizeof(struct cache_entry) + c->path_len + 1;		
			write(fd, c, size);
		}
	}

	close(fd);
	rename(".bkp-data/filecache.new", ".bkp-data/filecache");

	return 0;
}

int find_cache_entry_insert_idx(struct cache *cache, char *path)
{
	return find_cache_entry(cache, path, 1);
}

int find_cache_entry(struct cache *cache, char *path, int ret_insert_idx)
{
	int num_iter = 0;
	int low = 0, high = cache->entries_len - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2; 
        int cmp = strcmp(cache->entries[mid]->path, path);
        if (cmp == 0) {
            return mid;
		}
        else if (cmp < 0)
            low = mid + 1;   // discard left half including mid
        else
            high = mid - 1;  // discard right half including mid
		
		num_iter++;
    }

	return ret_insert_idx ? low : -1;
}

int cache_entry_changed(struct cache_entry *entry, struct stat *stat)
{
	int change = 0;
	if ((entry->st_mtim.tv_sec != stat->st_mtim.tv_sec) || 
		(entry->st_mtim.tv_nsec != stat->st_mtim.tv_nsec) || 
		(entry->st_ctim.tv_sec != stat->st_ctim.tv_sec) || 
		(entry->st_ctim.tv_nsec != stat->st_ctim.tv_nsec)) {
		
		change |= CE_TIME_CHANGED;
	}

	if (entry->st_mode != stat->st_mode)
		change |= CE_MODE_CHANGED;

	if (entry->st_size != stat->st_size)
		change |= CE_SIZE_CHANGED;

	return change;
}

int add_cache_entry(struct cache *cache, struct cache_entry *entry)
{
	int ret = 0;

	ret = write_file(entry->path, entry->st_size, entry->sha1);
	if (ret)
		return -1;

	int idx = find_cache_entry_insert_idx(cache, entry->path);
	return add_cache_entry_at(cache, entry, idx);	
}

static int add_cache_entry_at(struct cache *cache, struct cache_entry *entry, int idx)
{
	if (!cache->entries) {
		cache->entries = calloc(1000, sizeof(struct cache_entry *));
	}
	else {
		if (cache->entries_len % 1000 == 0)
			cache->entries = realloc(cache->entries, sizeof(struct cache_entry *) * (cache->entries_len + 1000));
	}
	
	if (!cache->entries) {
		fprintf(stderr, "Error allocating memory for cache entries!\n");
		return -ENOMEM;
	}

	if (cache->entries_len > idx) 
		memmove(&cache->entries[idx+1], &cache->entries[idx], (cache->entries_len - idx) * sizeof(struct cache_entry *));	
 
	cache->entries[idx] = entry;
	cache->entries_len++;

	return 0;
}

static void free_cache(struct cache *cache)
{
	if (!cache)
		return;

	if (cache->entries_len > 0)
		for (int i=0;i<cache->entries_len;i++) {
			free(cache->entries[i]);
			cache->entries[i] = NULL;
		}

	free(cache);
}
