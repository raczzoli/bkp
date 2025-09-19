
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>

#include "cache.h"

struct cache *load_cache()
{
	struct cache *cache = malloc(sizeof(struct cache));
	if (!cache) {
		fprintf(stderr, "Error allocating memory for cache!\n");
		return NULL;
	}

	cache->entries = NULL;
	cache->entries_len = 0;

	return cache;
}

int find_cache_entry(struct cache *cache, char *path)
{
	int num_iter = 0;
	int low = 0, high = cache->entries_len - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2; 
        int cmp = strcmp(cache->entries[mid]->path, path);
        if (cmp == 0) {
			printf("Number of iterations: %d\n", num_iter);
            return mid;
		}
        else if (cmp < 0)
            low = mid + 1;   // discard left half including mid
        else
            high = mid - 1;  // discard right half including mid
		
		num_iter++;
    }


	return -1;
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

	cache->entries[cache->entries_len] = entry;
	cache->entries_len++;

	return 0;
}


