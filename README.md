# bkp - Space-efficient incremental file-level backup utility for Linux

`bkp` is a command-line tool designed to perform incremental backups at the file level on Linux systems. It aims to minimize storage usage while keeping track of changes across multiple snapshots.

## Usage examples

- **Create a new snapshot of the current directory:**  
Regardless if it is the initial backup or the 100th incremental one, this same command is used. If it is the first backup, --create-snapshot will create all the necessary files and data directories without the need to execute any other --init commands.
```bash
bkp --create-snapshot
```

- **List existing snapshots, optionally limiting the number shown:**
```bash
bkp --snapshots [LIMIT]
```

- **Restore a specific snapshot to an output directory, optionally restoring only a sub-path:**
```bash
bkp --restore-snapshot [SHA1] [OUTPUT_DIR] [SUB_PATH]
```

- **Show details or content of a specific file stored in the backup by its SHA1 hash:**
```bash
bkp --show-file [SHA1]
```
