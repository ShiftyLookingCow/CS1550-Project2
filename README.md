# CS1550-Project2

This project extends the Linux kernel to include an implementation of semaphores, 
which is used to by prodcons.c to solve the producer/consumer problem for shared resources in a multi-threaded program. Designed for use with QEMU.

Usage:

sys.c, syscall_table.S, and unistd.h are all modified kernel files for a Linux system which implement the semaphore functionality

prodcons.c is the source for a program that can be run as "./prodcons c p bsize", where    
  'c' is the number of consumers,    
  'p' is the number of producers,    
  'bsize' is the size of the buffer to be used
