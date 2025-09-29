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

#ifndef FILE_H
#define FILE_H

#define FILE_CHUNK_SIZE (10 * (1024 * 1024))

int write_file(char *path, off_t size, unsigned char *sha1);
int read_blob(unsigned char *sha1, char **out_buff, int *out_size);
int read_chunks_file(unsigned char *sha1, unsigned char **out_buff, int *num_chunks);
int print_chunks_file(unsigned char *sha1);
#endif
