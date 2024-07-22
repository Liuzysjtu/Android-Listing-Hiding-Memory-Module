# Android-Listing-Hiding-Memory-Module

Traverse:
1. Use /proc/<pid>/maps to traverse all memory modules
2. Use lsof to traverse all memory modules
3. Use dl_iterate_phdr to traverse all loaded so libraries
4. Use the soinfo structure to traverse all loaded so libraries


hide:
1. Implement link breaking for the injected so library in the linked list that maintains soinfo information.
2. After using dlopen to load so, mmap a new memory, copy the loaded so memory with memmove, and then mremap to map the original address to the new address so that there is no name of this so in the maps.
3. Use seccomp+sigaction to hijack the open system call, modify the return values ​​of all system calls that open proc/<pid>/maps, and hide the injected so library
