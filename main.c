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

static void print_help();
static void parse_cmdline_args(int argc, char **argv);

int main(int argc, char **argv)
{
	parse_cmdline_args(argc, argv);
	return 0;
}

static void parse_cmdline_args(int argc, char **argv)
{
	int opt_idx = 0;
	int opt = 0;

	while ((opt = getopt_long(argc, argv, "h", cmdline_options, &opt_idx)) != -1) {
		switch(opt) {
			case 0:
				if (strcmp(cmdline_options[opt_idx].name, "create-snapshot") == 0) {
					create_snapshot();
				}
				else if (strcmp(cmdline_options[opt_idx].name, "snapshots") == 0) {
					int limit = (opt_idx + 1) < argc ? atoi(argv[opt_idx+1]) : 10; 
					printf("Listing a maximum number of %d created snapshots.\n"
							"To change the limit use \"bkp --snapshots [LIMIT]\"\n\n", limit);
					list_snapshots(limit);
				}
				else if (strcmp(cmdline_options[opt_idx].name, "restore-snapshot") == 0) {
					if ((argc - opt_idx) != 2) {
						printf("Invalid usage of --restore-snapshot!\nCommand should be: \"bkp --restore-snapshot [SNAPSHOT SHA1] [DIRECTORY TO RESTORE TO]\"\n");
						return;
					}
					
					unsigned char sha1[SHA_DIGEST_LENGTH];
					hex_to_sha1(argv[opt_idx], sha1);

					restore_snapshot(sha1, argv[opt_idx+1]);
				}
				else if (strcmp(cmdline_options[opt_idx].name, "show-file") == 0) {
					print_sha1_file(optarg);
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
}

static void print_help()
{
	printf("\n");
    printf("Usage: bkp [OPTIONS]\n");
    printf("A lightweight and space efficient (incremental) file-level backup utility for Linux.\n\n");
    printf("Options:\n");
    printf("  --create-snapshot                               Create a new snapshot of the backed up directory\n");
    printf("  --snapshots [LIMIT]                             Print a list of snapshots done so far\n");
    printf("  --restore-snapshot [SHA1] [OUTPUT_DIR]          Restores the snapshot identified by the passed SHA1 to OUTPUT_DIR\n");
	printf("\n");
	printf("  -h, --help                                      Show this help message and exit\n");
	printf("\n");
}
