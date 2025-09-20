#include <asm-generic/errno-base.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <stdbool.h>

#include "tree.h"
#include "cache.h"
#include "sha1.h"

static int scan_tree(char *path, struct cache *cache, unsigned char *sha1);
static int add_tree_entry(struct tree *tree, struct tree_entry *entry);
static int write_tree(struct tree *tree, unsigned char *sha1);
static void free_tree_entries(struct tree *tree);

int gen_tree(char *path, unsigned char *sha1)
{
	int ret = 0;
	struct cache *cache = load_cache();

	ret = scan_tree(path, cache, sha1);

	printf("Cache updated, number of entries: %d\n", cache->entries_len);
	
	/*
	 * TODO - This update, or maybe the whole gen_tree should be 
	 * placed somewhere else. Maybe a separate snapshot.c or backup.c
	 * where we call load_cache(), scan_tree() and if successful
	 * update_cache()
	 */
	update_cache(cache);

	return ret;
}

static int scan_tree(char *path, struct cache *cache, unsigned char *sha1)
{
	DIR *dirp = opendir(path);
	struct dirent *dirent = NULL;
	struct stat sb;
	int ret = 0;
	char full_path[PATH_MAX];

	struct cache_entry *c_entry = NULL;
	struct tree tree;
	struct tree_entry *entry;
	
	if (!dirp) {
		printf("Error opening directory!\n");
		return -1;
	}

	tree.entries = NULL;
	tree.entries_len = 0;

	while ((dirent = readdir(dirp)) != NULL) {
		if (strcmp(dirent->d_name, ".") == 0 || 
			strcmp(dirent->d_name, "..") == 0 ||
			strcmp(dirent->d_name, ".bkp-data") == 0)
			continue;
		
		snprintf(full_path, PATH_MAX, "%s%s", path, dirent->d_name);

		ret = lstat(full_path, &sb);
		if (ret) {
			printf("Error calling stat on: %s\n", full_path);
			goto end;
		}

		if (!S_ISDIR(sb.st_mode) && !S_ISREG(sb.st_mode))
			continue;
		
		entry = malloc(sizeof(struct tree_entry));
		if (!entry) {
			fprintf(stderr, "Error allocating memory for tree entry!\n");
			goto end;
		}

		memset(entry->sha1, 0, SHA_DIGEST_LENGTH);
		strncpy(entry->name, dirent->d_name, FILENAME_MAX);
		entry->name_len = strlen(dirent->d_name);
		entry->st_mode = sb.st_mode;

		if (S_ISDIR(sb.st_mode)) {
			strcat(full_path, "/");
			ret = scan_tree(full_path, cache, entry->sha1);
			if (ret)
				goto end;
		} 
		else if (S_ISREG(sb.st_mode)) {
			int path_len = strlen(full_path);
			int c_idx = find_cache_entry(cache, full_path);
			if (c_idx > -1) {
				c_entry = cache->entries[c_idx];
				
				bool changed = cache_entry_changed(c_entry, &sb);
				if (changed) {
					// TODO update cache entry
				}
			}
			else {
				c_entry = malloc(sizeof(struct cache_entry) + path_len + 1);
				if (!c_entry) {
					ret = -ENOMEM;
					fprintf(stderr, "Error allocating memory for cache entry!\n");

					goto end;
				}

				c_entry->st_mode = sb.st_mode;
				c_entry->st_size = sb.st_size;
				c_entry->st_mtim = sb.st_mtim;
				c_entry->st_ctim = sb.st_ctim;
				c_entry->path_len = strlen(full_path);
				strncpy(c_entry->path, full_path, c_entry->path_len);

				add_cache_entry(cache, c_entry);
			}
		}
	
		ret = add_tree_entry(&tree, entry);
		if (ret) 
			goto end;
	}

	ret = write_tree(&tree, sha1);

	goto end;

/*
 * TODO - free entry and c_entry
 */
end:
	free_tree_entries(&tree);

	closedir(dirp);
	return ret;
}

static int write_tree(struct tree *tree, unsigned char *sha1)
{
	int ret = 0;
	struct tree_entry *entry;
	int size = 100 + (sizeof(struct tree_entry) * tree->entries_len);
	char *buffer = malloc(size); 
	int offset = 0;
	
	if (!buffer) {
		fprintf(stderr, "Error allocating memory for tree buffer!\n");
		return -1;
	}

	offset = sprintf(buffer, "tree");
	offset += 1; // we want to keep the \0

	for (int i=0;i<tree->entries_len;i++) {
		entry = tree->entries[i];
		offset += sprintf(buffer+offset, "%d|%s", entry->st_mode, entry->name);
		offset += 1; // we want to keep the \0

		memcpy(buffer+offset, entry->sha1, SHA_DIGEST_LENGTH);
		offset += SHA_DIGEST_LENGTH; 
	}

	SHA1((const unsigned char *)buffer, offset, sha1);	
	ret = write_sha1_file(sha1, buffer, offset);

	free(buffer);
	return ret;
}

static int add_tree_entry(struct tree *tree, struct tree_entry *entry)
{
	if (!tree->entries) {
		tree->entries = calloc(100, sizeof(struct tree_entry *));
	}
	else {
		if (tree->entries_len % 100 == 0)
			tree->entries = realloc(tree->entries, sizeof(struct tree_entry *) * (tree->entries_len + 100));
	}

	if (!tree->entries) {
		fprintf(stderr, "Error allocating memory for tree entries!\n");
		return -ENOMEM;
	}

	tree->entries[tree->entries_len++] = entry;
	//printf("Adding entry: %s (%d) to tree (%d children)...\n", entry->name, entry->name_len, tree->entries_len);	

	return 0;
}

static void free_tree_entries(struct tree *tree)
{
	if (tree->entries_len == 0) 
		return;

	for (int i=0;i<tree->entries_len;i++) {
		free(tree->entries[i]);
		tree->entries[i] = NULL;
	}

	free(tree->entries);
	tree->entries = NULL;
	tree->entries_len = 0;
}
