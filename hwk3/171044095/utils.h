#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

#define BUFF_SIZE_INPUT 1024
#define MAX_PARAMS 5
typedef enum{ mDIR, MKDIR, RMDIR, DUMPE2FS, WRITE, READ, DEL, NONE}Operation;	//mDIR since DIR is not compliant with <dirent.h>
typedef enum{FALSE = 0, TRUE}bool;
/*
	fileSystemOper fileSystem.data operation parameters
*/

/*
	method checks the command line operation
	if the command is not valid, it will exit the program returning error message
*/
uint32_t n_file_systems;
char** file_system_names;
typedef unsigned char* bitmap_t;

void isValidCommand(const char** argv, int argc);

int isMakeFileSystemCommand(const char** argv, int argc);

int isFileSystemOperationCommand(const char** argv, int argc);

int processOperation(const char* operation, char** parameters);

Operation operationFromString(const char* str);

int containsSlash(const char* filename);

int isInteger(const char* str);

int hasDotDataExtension(const char* filename);

int isFileSystemPresent(const char* fileSystemName);

int getUserInput(char** outputStringArray ,char delimiter);

void freeUserInput(char* userInput[MAX_PARAMS], int n);

int isValidFileName(const char* filename);

void reportErrorMakeFileSystem(const char** argv, int argc);

void reportErrorFilSystemCommand(const char** argv, int argc);

void printTime(time_t time);

int stringToInt(const char* str);

FILE* getFile(const char* mode, const char* filename);

void closeFile( FILE* filename );

int seekUtil(FILE* fp, long offset, int whence);

//bitmap related functions
void setBitmap(bitmap_t b, int i);

void unsetBitmap(bitmap_t b, int i);

int getBitmap(bitmap_t b, int i);

bitmap_t createBitmap(int n);

uint32_t sizeBitmap(int n);

size_t fwriteUtil(const void *ptr, size_t size, size_t nmemb, FILE *stream);

int writeUtil(int fd, const void *buf, size_t count);

int readUtil(int fd, void *buf, size_t count);

int filenoUtil(FILE *stream);

size_t freadUtil(void *ptr, size_t size, size_t nmemb, FILE *stream);

void printBitmap(bitmap_t bitmap, int n);

void intToCharArray(char dest[4], int num);

uint32_t charArrayToInt(char dest[4]);

int countCharOccurencesInString(const char* str, char ch);

#endif