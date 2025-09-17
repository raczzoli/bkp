#include <asm-generic/errno-base.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/sha.h>

#include "tree.h"
#include "sha1.h"

int scan_tree(char *path);
static int add_tree_entry(struct tree *tree, struct tree_entry *entry);
static int write_tree(struct tree *tree);
static void free_tree_entries(struct tree *tree);

int main()
{
	scan_tree("./test-data/");

	return 0;
}

int scan_tree(char *path)
{
	DIR *dirp = opendir(path);
	struct dirent *dirent = NULL;
	struct stat sb;
	int ret = 0;
	char full_path[PATH_MAX];
	struct tree tree;
	struct tree_entry *entry;
	
	if (!dirp) {
		printf("Error opening directory!\n");
		return -1;
	}

	tree.entries = NULL;
	tree.entries_len = 0;

	while ((dirent = readdir(dirp)) != NULL) {
		if (strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0)
			continue;
		
		snprintf(full_path, PATH_MAX, "%s%s", path, dirent->d_name);

		ret = lstat(full_path, &sb);
		if (ret) {
			printf("Error calling stat on: %s\n", full_path);
			goto free_entry;
		}
 
		entry = malloc(sizeof(struct tree_entry));
		if (!entry) {
			fprintf(stderr, "Error allocating memory for tree entry!\n");
			goto free_entry;
		}

		strncpy(entry->name, dirent->d_name, FILENAME_MAX);
		entry->name_len = strlen(dirent->d_name);
		entry->st_mode = sb.st_mode;

		if (S_ISDIR(sb.st_mode)) {
			entry->type = ENTRY_TYPE_DIR;
			strcat(full_path, "/");

			ret = scan_tree(full_path);
			if (ret)
				goto free_entry;
		} 
		else if (S_ISREG(sb.st_mode)) {
			entry->type = ENTRY_TYPE_FILE;
		}
	
		ret = add_tree_entry(&tree, entry);

		if (ret) 
			goto free_entry;
	}

	ret = write_tree(&tree);

	goto end;

free_entry:
	if (entry)
		free(entry);
	
end:
	free_tree_entries(&tree);

	closedir(dirp);
	return ret;
}

static int write_tree(struct tree *tree)
{
	int ret = 0;
	struct tree_entry *entry;
	char *buffer = malloc(4096 * 2);
	int offset = 0;
	unsigned char sha1[SHA_DIGEST_LENGTH];
	
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

		// TODO
		// copy 20 byte hash
	}

	SHA1((const unsigned char *)buffer, offset, sha1);	
	ret = write_sha1_file(sha1, buffer, offset);
	
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
