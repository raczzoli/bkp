#ifndef TREE_H
#define TREE_H

#include <stdint.h>
#include <openssl/sha.h>
#include "bkp.h"

enum tree_entry_type {
	ENTRY_TYPE_DIR=1,
	ENTRY_TYPE_FILE,
	ENTRY_TYPE_BLOB
};

struct tree_entry {
	uint8_t type;
	int st_mode;
	char name[FILENAME_MAX];
	int name_len;
	unsigned char sha1_ref[SHA_DIGEST_LENGTH];
};

typedef struct tree {
	struct tree_entry **entries;
	int entries_len;
} tree_t;

int scan_tree(char *path, unsigned char *sha1);

#endif 
