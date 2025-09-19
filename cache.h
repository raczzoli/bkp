#ifndef CACHE_H
#define CACHE_H

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

struct cache_entry {
	mode_t st_mode;
	off_t st_size;
	struct timespec st_mtim;
	struct timespec st_ctim;
	int path_len;
	char path[PATH_MAX];
};

struct cache {
	struct cache_entry **entries;
	int entries_len;
};

int update_cache();
struct cache *load_cache();
int find_cache_entry(struct cache *cache, char *path);
bool cache_entry_changed(struct cache_entry *entry, struct stat *stat);
int add_cache_entry(struct cache *cache, struct cache_entry *entry);


#endif
