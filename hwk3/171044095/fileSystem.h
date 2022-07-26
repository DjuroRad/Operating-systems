#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H
#include "utils.h"

bool dir(const char* path, FILE* fp);

bool mkdir(const char* path, FILE* fp);

bool rmdir(const char* path, FILE* fp);

void dumpe2fs(FILE* fp);

bool write(const char* path, const char* srcFile, FILE* fp);

bool read(const char* path, const char* destFileName, FILE* fp);

bool del(const char* path, FILE* fp);

#endif