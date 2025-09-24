#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "print-file.h"
#include "tree.h"
#include "snapshot.h"

int print_sha1_file(char *sha1_hex)
{
	int ret = 0;
	int bytes = 0;
	int fd = 0;
	char path[PATH_MAX];
	char ftype[20];
	struct stat stat;

	sprintf(path, ".bkp-data/%s", sha1_hex);
	
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "SHA1 file %s doesn`t exist!\n", sha1_hex);
		return -1;
	}

	if (fstat(fd, &stat)) {
		ret = -1;
		fprintf(stderr, "fstat error!\n");
		goto end;
	}

	bytes = read(fd, ftype, 20);
	if (bytes < 0) {
		ret = -1;
		fprintf(stderr, "Error reading from SHA1 file!\n");
		goto end;
	}
	
	int len = strlen(ftype);

	/*
	 * We position the file cursor rightafter "type\0" so 
	 * when we pass the fd to the function which needs to 
	 * parse the respective type, read will be at the 
	 * position where the file`s actual content starts
	 */
	lseek(fd, len+1, SEEK_SET);

	printf("----------- %s -----------\n", ftype);

	if (strcmp(ftype, "snapshot") == 0) 
		print_snapshot_file(fd, &stat);
	else if (strcmp(ftype, "tree") == 0) 
		print_tree_file(fd, &stat);
	// blob
	// chunks
	// etc.

end:
	close(fd);
	return ret;
}

