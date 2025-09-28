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

#include <linux/limits.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "restore.h"
#include "snapshot.h"
#include "tree.h"
#include "file.h"
#include "sha1-file.h"

static int restore_tree(unsigned char *sha1, char *out_path, char *sub_path, int sub_path_len);
static int restore_dir(struct tree_entry *entry, char *out_path, char *sub_path, int sub_path_len);
static int restore_file(struct tree_entry *entry, char *out_path);

int restore_snapshot(unsigned char *sha1, char *path, char *sub_path)
{
	int ret = 0;
	DIR *dir = NULL;
	struct dirent *dentry;
	struct snapshot snapshot;

	ret = read_snapshot_file(sha1, &snapshot);
	if (ret)
		return -1;

	dir = opendir(path);	
	if (!dir) {
		fprintf(stderr, "Error opening %s - %s!\n", path, strerror(errno));
		return -1;
	}

	/*
	 * Check if output directory is empty.
	 *
	 * TODO - in the future we will need to accept
	 * directories which are not empty, even the same
	 * directory we are backing up
	 */
	while ((dentry = readdir(dir)) != NULL) {
		if (strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0)
			continue;
		
		fprintf(stderr, "Output directory not empty! For now only empty output directories are accepted!\n");
		closedir(dir);
		return -1;
	}
	closedir(dir);

	/*
	 * TODO - here we need to sanitize path and sub_path so 
	 * we can be sure they have the same structure (regarding
	 * slashes at the beginning, end etc.) no matter what the 
	 * user wrote
	 */
	char full_sub_path[PATH_MAX];
	snprintf(full_sub_path, PATH_MAX, "%s%s", path, sub_path);

	ret = restore_tree(snapshot.tree_sha1, path, full_sub_path, strlen(full_sub_path));
	if (ret)
		return -1;

	return 0;
}

static int restore_tree(unsigned char *sha1, char *out_path, char *sub_path, int sub_path_len)
{
	int ret = 0;
	struct tree tree;
	struct tree_entry *entry = NULL;
	char full_out_path[PATH_MAX];
	int full_out_path_len = 0;
	
	ret = read_tree_file(sha1, &tree);
	if (ret)
		return -1;

	for (int i=0;i<tree.entries_len;i++) {
		entry = tree.entries[i];

		full_out_path_len = snprintf(full_out_path, PATH_MAX, "%s%s", out_path, entry->name);

		if (sub_path) {
			/*
			 * Here we do two comparations because:
			 * 1. if sub_path is a directory we check: 
			 *    strncmp(full_out_path, sub_path, sub_path_len)
			 *    so we know that sub_path is a substring (prefix) of full_path
			 * 2. if sub_path is a file, we do the opposite
			 *    strncmp(full_out_path, sub_path, full_out_path_len)
			 *    meaning full_out_path is a prefix of sub_path
			 */
			if (memcmp(full_out_path, sub_path, sub_path_len) != 0 &&
				memcmp(full_out_path, sub_path, full_out_path_len) != 0)
				continue;
		}

		if (S_ISDIR(entry->st_mode)) {
			strcat(full_out_path, "/");

			if (restore_dir(entry, full_out_path, sub_path, sub_path_len)) {
				ret = -1;
				goto end;
			}
		}
		else if (S_ISREG(entry->st_mode)) {
			if (restore_file(entry, full_out_path)) {
				ret = -1;
				goto end;
			}
		}
		else {
			/*
			 * This shouldn`t happen. We only store 
			 * files and directories
			 */
			ret = -1;
			goto end;
		}
	}

end:
	free_tree_entries(&tree);
	return ret;
}

static int restore_dir(struct tree_entry *entry, char *out_path, char *sub_path, int sub_path_len)
{
	int perms = entry->st_mode & 0777;
	
	if (mkdir(out_path, perms)) {
		fprintf(stderr, "Error creating directory: %s\n", out_path);
		return -1;
	}

	return restore_tree(entry->sha1, out_path, sub_path, sub_path_len);
}

static int restore_file(struct tree_entry *entry, char *out_path)
{
	int fd = 0;
	int ret = 0;
	unsigned char *chunks_buff = NULL;
	char *blob_buff = NULL;
	int num_chunks = 0;
	int offset = 0;
	size_t bytes = 0;
	size_t blob_size = 0;
	int perms = entry->st_mode & 0777;

	ret = read_chunks_file(entry->sha1, &chunks_buff, &num_chunks);
	if (ret)
		return -1;

	fd = open(out_path, O_WRONLY | O_CREAT, perms);
	if (fd < 0) {
		ret = -1;
		fprintf(stderr, "Error opening output file: %s - %s\n", out_path, strerror(errno));
		goto end;
	}

	while(num_chunks > 0) {
		unsigned char *sha1 = chunks_buff+offset;
		offset += SHA_DIGEST_LENGTH;
		num_chunks--;

		/*
		 * Let`s write the blob chunks
		 */	
		ret = read_blob(sha1, &blob_buff, &blob_size);
		
		if (ret) 
			goto end;
	
		if (blob_size > 0) {
			bytes = write(fd, blob_buff, blob_size);
			if (bytes != blob_size) {
				ret = -1;
				fprintf(stderr, "Error writing to output file: %s - %s\n", out_path, strerror(errno));
				goto end;
			}
		}

		free(blob_buff);
		blob_buff = NULL;
	}

end:
	if (chunks_buff)
		free(chunks_buff);

	if (blob_buff)
		free(blob_buff);

	if (fd >= 0)
		close(fd);

	return ret;
}

