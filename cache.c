
#include <asm-generic/errno-base.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>

#include "cache.h"

static int add_cache_entry_at(struct cache *cache, struct cache_entry *entry, int idx);
static void free_cache(struct cache *cache);

struct cache *load_cache()
{
	int fd = 0, bytes = 0;
	struct cache *cache = malloc(sizeof(struct cache));
	struct cache_entry header;
	struct cache_entry *c = NULL;

	if (!cache) {
		fprintf(stderr, "Error allocating memory for cache!\n");
		return NULL;
	}

	cache->entries = NULL;
	cache->entries_len = 0;

	fd = open(".bkp-data/filecache", O_RDONLY);	
	if (fd < 0) 
		goto end; // not an error, it just doesn`t exist yet
			
	while (true) {
		bytes = read(fd, &header, sizeof(struct cache_entry));
		if (bytes <= 0)
			break;		

		c = malloc(sizeof(struct cache_entry) + header.path_len + 1);
		if (!c) {
			fprintf(stderr, "Error allocating memory for cache entry!\n");
			goto err;
		}
		
		memcpy(c, &header, sizeof(struct cache_entry));
		memset(c->path, 0, header.path_len + 1);

		bytes = read(fd, c->path, header.path_len + 1); // read the \0 too
		// TODO - some error check here too

		add_cache_entry_at(cache, c, cache->entries_len);
	}

	goto end;

err: 
	if (c) {
		free(c);
		c = NULL;
	}

	free_cache(cache);

end:
	if (fd >= 0)
		close(fd);

	return cache;
}

int update_cache(struct cache *cache)
{
	int fd = open(".bkp-data/filecache.new", O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (fd < 0) {
		if (errno == EEXIST) 
			fprintf(stderr, "filecache.new already exists! Maybe another cache update in progress?\n");

		return -1;
	}

	if (cache->entries_len > 0) {
		for (int i=0;i<cache->entries_len;i++) {
			//printf("%s\n", cache->entries[i]->path);
			struct cache_entry *c = cache->entries[i];
			
			write(fd, c, sizeof(struct cache_entry));
			write(fd, c->path, c->path_len+1); // we write \0 too
		}
	}

	close(fd);
	rename(".bkp-data/filecache.new", ".bkp-data/filecache");

	return 0;
}

int find_cache_entry_insert_idx(struct cache *cache, char *path)
{
	return find_cache_entry(cache, path, true);
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

bool cache_entry_changed(struct cache_entry *entry, struct stat *stat)
{
	if ((entry->st_mtim.tv_sec != stat->st_mtim.tv_sec) || 
		(entry->st_mtim.tv_nsec != stat->st_mtim.tv_nsec) || 
		(entry->st_ctim.tv_sec != stat->st_ctim.tv_sec) || 
		(entry->st_ctim.tv_nsec != stat->st_ctim.tv_nsec)) {
	
		return true;
	}

	return false;
}

int add_cache_entry(struct cache *cache, struct cache_entry *entry)
{
	int idx = find_cache_entry_insert_idx(cache, entry->path);
	return add_cache_entry_at(cache, entry, idx);	
}

static int add_cache_entry_at(struct cache *cache, struct cache_entry *entry, int idx)
{
	if (!cache->entries) {
		cache->entries = calloc(100, sizeof(struct cache_entry *));
	}
	else {
		if (cache->entries_len % 100 == 0)
			cache->entries = realloc(cache->entries, sizeof(struct cache_entry *) * (cache->entries_len + 100));
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
