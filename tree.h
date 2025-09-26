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

#ifndef TREE_H
#define TREE_H

#include <stdint.h>
#include <sys/stat.h>
#include <openssl/sha.h>

#include "bkp.h"
#include "cache.h"

enum tree_entry_type {
	ENTRY_TYPE_DIR=1,
	ENTRY_TYPE_FILE,
	ENTRY_TYPE_BLOB
};

struct tree_entry {
	int st_mode;
	unsigned char sha1[SHA_DIGEST_LENGTH];
	int name_len;
	char name[NAME_MAX+1];
};

typedef struct tree {
	struct tree_entry **entries;
	int entries_len;
} tree_t;

int create_tree(char *path, struct cache *cache, unsigned char *sha1);
int read_tree_file(unsigned char *sha1, struct tree *tree);
int print_tree_file(unsigned char *sha1);
void free_tree_entries(struct tree *tree);

#endif 
