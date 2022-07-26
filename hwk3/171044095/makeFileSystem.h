#ifndef MAKE_FILE_SYSTEM_H
#define MAKE_FILE_SYSTEM_H
#include <stdio.h>

void createFileSystem(int blockSize, const char* fileSystemName);//creates the file system and writes an empty file system to a file
void mountFileSystem(const char* fileSystemName);//reads the whole file system from give file
void syncFileSystem(const char* fileSystemName);//updates gobal variables like superblock and bitmaps
void unmountFileSystem(const char* fileSystemName);
void unmountFileSystemOptimized(FILE* fp);
void printFileSystemInfo();

#endif