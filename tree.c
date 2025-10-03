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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "tree.h"
#include "cache.h"
#include "sha1-file.h"

static int scan_tree(char *path, struct cache *cache, unsigned char *sha1);
static int add_tree_entry(struct tree *tree, struct tree_entry *entry);
static int write_tree(struct tree *tree, unsigned char *sha1);

int create_tree(char *path, struct cache *cache, unsigned char *sha1)
{
	return scan_tree(path, cache, sha1);
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
		strncpy(entry->name, dirent->d_name, NAME_MAX+1);
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
			int c_idx = find_cache_entry(cache, full_path, 0);

			if (c_idx > -1) {
				c_entry = cache->entries[c_idx];
				
				int changed = cache_entry_changed(c_entry, &sb);
				if (changed) {
					if (changed & CE_MODE_CHANGED) 
						c_entry->st_mode = sb.st_mode;

					if (changed & CE_TIME_CHANGED) {
						c_entry->st_mtim.tv_sec = sb.st_mtim.tv_sec;
						c_entry->st_mtim.tv_nsec = sb.st_mtim.tv_nsec;
						c_entry->st_ctim.tv_sec = sb.st_ctim.tv_sec;
						c_entry->st_ctim.tv_nsec = sb.st_ctim.tv_nsec;	
					}
					
					if (changed & CE_SIZE_CHANGED) {
						c_entry->st_size = sb.st_size;
						// TODO update file
					}
				}
			}
			else {
				c_entry = malloc(sizeof(struct cache_entry) + path_len + 1);
				if (!c_entry) {
					ret = -ENOMEM;
					fprintf(stderr, "Error allocating memory for cache entry!\n");

					goto end;
				}

				memset(c_entry->sha1, 0, SHA_DIGEST_LENGTH);
				c_entry->st_mode = sb.st_mode;
				c_entry->st_size = sb.st_size;
				c_entry->st_mtim = sb.st_mtim;
				c_entry->st_ctim = sb.st_ctim;
				c_entry->path_len = strlen(full_path);
				strcpy(c_entry->path, full_path);
				//printf("%s .... %s\n", full_path, c_entry->path);	
				add_cache_entry(cache, c_entry);
			}
			// add file sha1 to tree entry
			memcpy(entry->sha1, c_entry->sha1, SHA_DIGEST_LENGTH);
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
		offset += sprintf(buffer+offset, "%d %s", entry->st_mode, entry->name);
		offset += 1; // we want to keep the \0

		memcpy(buffer+offset, entry->sha1, SHA_DIGEST_LENGTH);
		offset += SHA_DIGEST_LENGTH; 
	}

	ret = write_sha1_file(sha1, buffer, offset);

	free(buffer);
	return ret;
}

int read_tree_file(unsigned char *sha1, struct tree *tree)
{
	int ret = 0;
	char *buff = NULL;
	int buff_len = 0;

	ret = read_sha1_file(sha1, "tree", &buff, &buff_len);
	if (ret)
		return -1;
	
	return read_tree_buffer(buff, buff_len, tree);
}

int read_tree_buffer(char *buff, int buff_len, struct tree *tree)
{
	int ret = 0;
	struct tree_entry *entry = NULL;
	int offset = 0;
	int consumed = 0;

	tree->entries = NULL;
	tree->entries_len = 0;

	while(offset < buff_len) {
		entry = malloc(sizeof(struct tree_entry));	

		// read file mode + path + \0
		sscanf(buff+offset, "%d %n", &entry->st_mode, &consumed);
		offset += consumed;
		offset += snprintf(entry->name, FILENAME_MAX, "%s", buff+offset)+1;

		// read referenced sha1 hash
		memcpy(entry->sha1, buff+offset, 20);
		offset += SHA_DIGEST_LENGTH;

		ret = add_tree_entry(tree, entry);
		if (ret) {
			free(entry);
			goto end;
		}
	}

end:
	free(buff);
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

	return 0;
}

void free_tree_entries(struct tree *tree)
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

int print_tree_buffer(char *buff, int buff_len)
{
	int ret = 0;
	struct tree tree;
	struct tree_entry *entry;
	char sha1_hex[40+1];

	ret = read_tree_buffer(buff, buff_len, &tree);
	if (ret)
		return -1;

	if (tree.entries_len > 0)
		for (int i=0;i<tree.entries_len;i++) {
			entry = tree.entries[i];
			sha1_to_hex(entry->sha1, sha1_hex);
			printf("%-8o %-50s %s\n", entry->st_mode, entry->name, sha1_hex);
		}

	free_tree_entries(&tree);
	return 0;

}
