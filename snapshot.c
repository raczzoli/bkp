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
#include <fcntl.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "snapshot.h"
#include "cache.h"
#include "tree.h"
#include "sha1-file.h"

static int write_snapshot(unsigned char *tree_sha1, unsigned char *sha1);
static int read_last_sha1(unsigned char *sha1);
static int write_last_sha1(unsigned char *sha1);

int list_snapshots(int limit)
{
	int ret = 0;
	unsigned char sha1[SHA_DIGEST_LENGTH];
	char sha1_hex[40+1];
	struct snapshot snapshot;

	ret = read_last_sha1(sha1);
	if (ret) {
		fprintf(stderr, "No snapshots found!\n");
		return -1;
	}

	while(limit > 0) {
		ret = read_snapshot_file(sha1, &snapshot);
		if (ret)
			return -1;

		sha1_to_hex(snapshot.sha1, sha1_hex);

		printf("Snapshot SHA1: %s\n", sha1_hex);
		printf("Created on: %s\n", snapshot.date);
		printf("----------------------------------------------------------\n");

		if (!sha1_is_valid(snapshot.parent_sha1)) 
			break;

		memcpy(sha1, snapshot.parent_sha1, SHA_DIGEST_LENGTH);
		limit--;
	}

	return 0;
}

int create_snapshot()
{
	int ret = 0;
	unsigned char tree_sha1[SHA_DIGEST_LENGTH];
	unsigned char snap_sha1[SHA_DIGEST_LENGTH];
	char sha1_hex[40+1];

	printf("Loading filecache into memory... ");
	fflush(stdout);

	struct cache *cache = load_cache();
	if (!cache)
		return -1;

	printf("done\n");

	ret = create_tree("./", cache, tree_sha1);

	if (ret) {
		fprintf(stderr, "Error generating tree (code: %d)!\n", ret);
		return ret;
	}

	printf("Writing %d entries to filecache... ", cache->entries_len);
	fflush(stdout);
	update_cache(cache);
	printf("done\n");
	
	ret = write_snapshot(tree_sha1, snap_sha1);
	if (ret)
		goto end;

	sha1_to_hex(tree_sha1, sha1_hex);
	printf("\nTree sha1: %s\n", sha1_hex);

	sha1_to_hex(snap_sha1, sha1_hex);
	printf("Snapshot sha1: %s\n", sha1_hex);

end:
	return ret;
}

static int write_snapshot(unsigned char *tree_sha1, unsigned char *sha1)
{
	unsigned char parent_sha1[SHA_DIGEST_LENGTH];
	char buffer[1024];
	int offset = 0;
	int ret = 0;

	memset(buffer, 0, 1024);

	read_last_sha1(parent_sha1);

	offset = 1 + sprintf(buffer, "snapshot");	
	offset += 1 + sprintf(buffer+offset, "parent ");
	memcpy(buffer+offset, parent_sha1, SHA_DIGEST_LENGTH);
	offset += SHA_DIGEST_LENGTH;

	offset += 1 + sprintf(buffer+offset, "tree ");

	memcpy(buffer+offset, tree_sha1, SHA_DIGEST_LENGTH);
	offset += SHA_DIGEST_LENGTH;

	// writing time in sec
	time_t now = time(NULL);
	struct tm *local = localtime(&now);

	offset += 1 + sprintf(buffer+offset, "time %ld", (long)now);

	// writing date-time in human readable form
    offset += 1 + strftime(buffer+offset, sizeof(buffer)-offset, "date %Y-%m-%d %H:%M:%S", local);

	// hash the buffer
	SHA1((const unsigned char *)buffer, offset, sha1);	
	ret = write_sha1_file(sha1, buffer, offset);

	if (ret) {
		fprintf(stderr, "Error writing snapshot file!\n");
		return -1;
	}

	write_last_sha1(sha1);

	return ret;
}

static int write_last_sha1(unsigned char *sha1)
{
	int fd = open(".bkp-data/last_snapshot", O_WRONLY | O_CREAT, 0666);
	if (fd < 0) {
		fprintf(stderr, "Error writing snpshot hash to last_snapshot!\n");
		return -1;
	}

	write(fd, sha1, SHA_DIGEST_LENGTH);
	close(fd);

	return 0;
}

static int read_last_sha1(unsigned char *sha1)
{
	int bytes = 0;
	int fd = 0;
	
	memset(sha1, 0, SHA_DIGEST_LENGTH);

	fd = open(".bkp-data/last_snapshot", O_RDONLY);
	if (fd < 0)
		return -1;

	bytes = read(fd, sha1, SHA_DIGEST_LENGTH);
	if (bytes != SHA_DIGEST_LENGTH) 
		return -1;

	close(fd);
	return 0;
}

int read_snapshot_file(unsigned char *sha1, struct snapshot *snapshot)
{
	int ret = 0;
	char *buff = NULL;
	int buff_len = 0;
	int offset = 0;

	ret = read_sha1_file(sha1, &buff, &buff_len);
	if (ret)
		return ret;

	memcpy(snapshot->sha1, sha1, SHA_DIGEST_LENGTH);

	/*
	 * Skipping the snapshot file header ("snapshot\0")
	 */
	offset += strlen(buff)+1;

	while(*(buff+offset) != '\0') {
		if (strcmp(buff+offset, "parent ") == 0) {
			offset += 8; // "parent \0"
			memcpy(snapshot->parent_sha1, buff+offset, SHA_DIGEST_LENGTH);
			offset += SHA_DIGEST_LENGTH;
		}
		else if (strcmp(buff+offset, "tree ") == 0) {
			offset += 6; // "trree \0"
			memcpy(snapshot->tree_sha1, buff+offset, SHA_DIGEST_LENGTH);
			offset += SHA_DIGEST_LENGTH;
		}
		else if (strncmp(buff+offset, "date ", 5) == 0) {
			offset += 5; // "date "
			strcpy(snapshot->date, buff+offset);
			offset += strlen(buff+offset);
		}
		else if (strncmp(buff+offset, "time ", 5) == 0) {
			int consumed = 0;
			offset += 5; // "date "
			sscanf(buff+offset, "%ld%n", &snapshot->time, &consumed);
			offset += consumed + 1;
		}
		else {
			offset += strlen(buff+offset)+1;
		}
	}

	free(buff);
	return ret;
}

int print_snapshot_file(unsigned char *sha1)
{
	int ret = 0;
	struct snapshot snapshot;
	char sha1_hex[40+1];

	ret = read_snapshot_file(sha1, &snapshot);
	if (ret)
		return -1;

	sha1_to_hex(sha1, sha1_hex);
	printf("Snapshot SHA1: %s\n", sha1_hex);	

	sha1_to_hex(snapshot.parent_sha1, sha1_hex);
	printf("Parent snapshot: %s\n", sha1_hex);	

	sha1_to_hex(snapshot.tree_sha1, sha1_hex);
	printf("Tree SHA1: %s\n", sha1_hex);	

	printf("Created on: %s\n", snapshot.date);	

	return 0;
}
