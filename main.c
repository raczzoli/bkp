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
#include <string.h>
#include <getopt.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>


#include "snapshot.h"
#include "restore.h"
#include "sha1-file.h"
#include "print-file.h"

static struct option cmdline_options[] = {
	{"create-snapshot",  no_argument,       0, 0},
	{"snapshots",        no_argument,       0, 0},
	{"restore-snapshot", required_argument, 0, 0},
	{"show-file", required_argument, 0, 0},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};

static int init();
static void print_help();
static int handle_cmdline_args(int argc, char **argv);

int main(int argc, char **argv)
{
	if (init())
		return -1;

	if (handle_cmdline_args(argc, argv))
		return -1;
	
	return 0;
}

static int init()
{
	DIR *dir = opendir(".bkp-data");
	if (dir) 
		closedir(dir);
	else {
		printf(".bkp-data doesn`t exist, creating it...\n");
		mkdir(".bkp-data", 0755);
	}

	return 0;
}

static int handle_cmdline_args(int argc, char **argv)
{
	int opt_idx = 0;
	int opt = 0;

	while ((opt = getopt_long(argc, argv, "h", cmdline_options, &opt_idx)) != -1) {
		switch(opt) {
			case 0:
				if (strcmp(cmdline_options[opt_idx].name, "create-snapshot") == 0) {
					return create_snapshot();
				}
				else if (strcmp(cmdline_options[opt_idx].name, "snapshots") == 0) {
					int limit = (opt_idx + 1) < argc ? atoi(argv[opt_idx+1]) : 10; 
					printf("Listing a maximum number of %d created snapshots.\n"
							"To change the limit use \"bkp --snapshots [LIMIT]\"\n\n", limit);
					return list_snapshots(limit);
				}
				else if (strcmp(cmdline_options[opt_idx].name, "restore-snapshot") == 0) {
					if (argc < 4) {
						printf("Invalid usage of --restore-snapshot!\n"
								"Command should be: \""
								"bkp --restore-snapshot [SHA1] [OUTPUT_DIR] [optional: SUB_PATH]\"\n");
						return -1;
					}
					
					char *sha1_hex = argv[opt_idx];
					char *out_path = argv[opt_idx+1];
					char *sub_path = argc > 4 ? argv[opt_idx+2] : NULL;
					unsigned char sha1[SHA_DIGEST_LENGTH];

					hex_to_sha1(sha1_hex, sha1);
					return restore_snapshot(sha1, out_path, sub_path);
				}
				else if (strcmp(cmdline_options[opt_idx].name, "show-file") == 0) {
					return print_sha1_file(optarg);
				}

			break;
			case '?': // unknown option or missing required argument
				printf("Unknown option or missing required argument!\n");
				// fall through
			case 'h':
				print_help();
				break;
			default:
				printf("Invalid command line option!\n");
		}
	}

	return 0;
}

static void print_help()
{
	printf("\n");
    printf("Usage: bkp [OPTIONS]\n");
    printf("A lightweight and space efficient (incremental) file-level backup utility for Linux.\n\n");
    printf("Options:\n");
    printf("  --create-snapshot                                   Create a new snapshot of the backed up directory\n");
    printf("  --snapshots [LIMIT]                                 Print a list of snapshots done so far\n");
    printf("  --restore-snapshot [SHA1] [OUTPUT_DIR] [SUB_PATH]   Restores the snapshot with SHA1 to OUTPUT_DIR with the optional possibility\n");
    printf("                                                      to restore only a SUB_PATH of the snapshot like /home/user/only_this_file \n");
	printf("\n");
	printf("  -h, --help                                      Show this help message and exit\n");
	printf("\n");
}
