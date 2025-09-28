# bkp
Space efficient incremental file-level backup utility for Linux

### TODO
- [x] When restoring a snapshot it would be very nice to be able to restore only a subtree or even only a file, like:  
      --restore-snapshot [sha1] [output_dir] [/var/lib/some_folder] or   
      --restore-snapshot [sha1] [output_dir] [/home/user/workspace/file1.zip]  
- [ ] Add check command to verify all stored objects (rehash & compare SHA1).  
- [ ] Improve error handling (separate fatal vs. warning cases).  
- [ ] Add basic progress reporting (e.g., “Processed 124/5000 files, 3.2 GB”).  
- [ ] Allow configurable thread count and chunk size via CLI.  
- [ ] Introduce packfile/segment storage: batch many objects into container files.  
- [ ] Add an index file per pack for fast lookups.  
- [ ] Add restore progress feedback.  
- [ ] Partial restore: support multiple subpaths in one restore.  
- [ ] Add exclude/include patterns (--exclude *.tmp, --include src/**).  
- [ ] Add config file support for default settings.  
- [ ] Add compression options (none, fast, high).  


