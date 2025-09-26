#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "restore.h"
#include "snapshot.h"
#include "tree.h"
#include "sha1-file.h"

static int restore_tree(unsigned char *sha1, char *in_path, char *out_path);

int restore_snapshot(unsigned char *sha1, char *path)
{
	int ret = 0;
	int fd = 0;
	DIR *outdir = NULL;
	struct dirent *dentry;
	struct snapshot snapshot;

	ret = read_snapshot_file(sha1, &snapshot);
	if (ret)
		return -1;

	outdir = opendir(path);	
	if (!outdir) {
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
	while ((dentry = readdir(outdir)) != NULL) {
		if (strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0)
			continue;
		
		fprintf(stderr, "Output directory not empty! For now only empty output directories are accepted!\n");
		closedir(outdir);
		return -1;
	}
	closedir(outdir);

	ret = restore_tree(snapshot.tree_sha1, "/home/rng89/", path);
	if (ret)
		return -1;

	return 0;
}

static int restore_tree(unsigned char *sha1, char *in_path, char *out_path)
{
	int ret = 0;
	struct tree tree;
	struct tree_entry *entry = NULL;
	char full_in_path[PATH_MAX];
	char full_out_path[PATH_MAX];

	ret = read_tree_file(sha1, &tree);
	if (ret)
		return -1;

	for (int i=0;i<tree.entries_len;i++) {
		entry = tree.entries[i];

		snprintf(full_in_path, PATH_MAX, "%s%s", in_path, entry->name);
		snprintf(full_out_path, PATH_MAX, "%s%s", out_path, entry->name);

		if (S_ISDIR(entry->st_mode)) {
			int perms = entry->st_mode & 0777;
			mkdir(full_out_path, perms);

			strcat(full_in_path, "/");
			strcat(full_out_path, "/");

			ret = restore_tree(entry->sha1, full_in_path, full_out_path);
		}
	}

	return 0;
}
