#define _GNU_SOURCE
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

/*
	POSSIBLE OPERATIONS:
						 		arguments
	dir  ->  4 params 			1
	mkdir  ->  4 params 		1
	rmdir  ->  4 params 		1
	----
	dumpe2fs  ->  0 params 		0
	----
	write  ->  2 params 		2
	read  ->  2 params 			2
	----
	del  ->  2 params 			1



There are 2 types of commands:

	1 => GENERAL COMMAND: 

	fileSystemOper fileSystem.data operation parameters
	1			   1			   1		 2 max			(number of parameters)


	2 => MAKE FILE SYSTEM COMMAND
		makeFileSystem		 4			 mySystem.dat
		command 	   		n KB		file system name

		number of parameters: 3
		1st param = makeFileSystem
		2nd param = integer value
		3rd param = filename
*/
uint32_t n_file_systems = 0;
char** file_system_names = NULL;//should I store this in "/"'s inode maybe? I don't think so because there can't be a "/" folder without a filesystem created.

const char* FILE_SYSTEM_ERROR_STR = "File system Error ->";
const char* DOT_DATA_EXTENSION = ".data";
const char* MAKE_FILE_SYSTEM_COMMAND = "./makeFileSystem";
const char* FILE_SYSTEM_COMMAND = "./fileSystemOper";

void reportError(const char** argv, int argc);
void reportOperationError(Operation operation, int argc);

void isValidCommand(const char** argv, int argc){


	if( !(isMakeFileSystemCommand(argv, argc) || isFileSystemOperationCommand(argv, argc)) ){
		reportError(argv, argc);
		exit(1);
	}
	//otherwise we are good, program's execution will continue
}

// command fileSystemName.data operation 
int isFileSystemOperationCommand(const char** argv, int argc){
	int isFileSystemOperationCommand = 0;
	

	//in case not enough arguments
	if( argc < 3 ) return 0;

	Operation operation = operationFromString(argv[2]);
	//check command, file system name, operation
	if( 
		strcmp(FILE_SYSTEM_COMMAND, argv[0]) != 0 ||
		!hasDotDataExtension( argv[1] ) ||
		operation == NONE
		){ 
		return 0;}

	//check number of parameters
	switch( operation ){
		case mDIR://checked operation is DIR && that .data extension?
		case MKDIR:
		case RMDIR://to delete a directory
		case DEL://to delete a file
			return argc == 4;
			break;
		case DUMPE2FS:
			return argc == 3;
			break;
		case WRITE:
		case READ:
			return argc == 5;
			break;
		case NONE:
			return 0;
			break;
	}
	// printf("HERE");
	return isFileSystemOperationCommand;
}

void reportOperationError(Operation operation, int argc){
	switch( operation ){
		case mDIR://checked operation is DIR && that .data extension?
		case MKDIR:
		case RMDIR://to delete a directory
		case DEL://to delete a file
			printf("| dir | mkdir | rmdir | del | commands require 1 argument.\n");
			break;
		case DUMPE2FS:
			printf("| dmupe2fs | requires must not have any arguments.\n");
			break;
		case WRITE:
		case READ:
			printf("| read | write | commands require 2 arguments.\n");
			break;
		case NONE:
			break;
	}
}

int isMakeFileSystemCommand(const char** argv, int argc){

	return 
		argc == 3 &&
		(strcmp(MAKE_FILE_SYSTEM_COMMAND, argv[0]) == 0) &&
		(isInteger(argv[1])) &&
		!containsSlash(argv[2]) && 
		hasDotDataExtension(argv[2]);
}

int containsSlash(const char* filename){
	return (strchr(filename, '\\') != NULL);
}

//typedef enum Operation{DIR, MKDIR, RMDIR, DUMPE2FS, WRITE, READ, DEL, NONE};	
Operation operationFromString(const char* str){
	Operation operation = NONE;
	if( strcmp( str, "dir") == 0 )
		operation = mDIR;
	else if( strcmp( str, "mkdir") == 0 )
		operation = MKDIR;
	else if( strcmp( str, "rmdir") == 0 )
		operation = RMDIR;
	else if( strcmp( str, "dumpe2fs") == 0 )
		operation = DUMPE2FS;
	else if( strcmp( str, "write") == 0 )
		operation = WRITE;
	else if( strcmp( str, "read") == 0 )
		operation = READ;
	else if( strcmp( str, "del") == 0 )
		operation = DEL;
	
	return operation;
}

int isInteger(const char* str){
    for (int i = 0; str[i]!= '\0'; i++)
        if (!isdigit(str[i]))
              return 0;
    return 1;
}

int hasDotDataExtension(const char* filename){

	return 
			(strlen(filename) > 5) && 
			(strcmp( filename + strlen(filename) - strlen(DOT_DATA_EXTENSION), DOT_DATA_EXTENSION ) == 0);
}


void reportError(const char** argv, int argc){
	errno =1;
	char error_message[100];

	if( argc<3 )
		sprintf(error_message, "%s Each command will have at least 3 arguments\n", FILE_SYSTEM_ERROR_STR);
	else if( strcmp(FILE_SYSTEM_COMMAND, argv[0]) != 0 && strcmp(MAKE_FILE_SYSTEM_COMMAND, argv[0]) != 0 )
		sprintf(error_message, "%s 1st command should be either | %s | or | %s |\n", FILE_SYSTEM_ERROR_STR, MAKE_FILE_SYSTEM_COMMAND, FILE_SYSTEM_COMMAND);
	else if( !hasDotDataExtension(argv[2]) || isInteger(argv[1]) )
		sprintf(error_message, "%s  %s => File system's name needs to have .data extension | %s => Second argument is an integer\n", FILE_SYSTEM_ERROR_STR, MAKE_FILE_SYSTEM_COMMAND, FILE_SYSTEM_COMMAND);
	else if( !isFileSystemPresent(argv[1]) )
		sprintf(error_message, "%s Targeted file system not present\n", FILE_SYSTEM_ERROR_STR);
	else if ( operationFromString(argv[2]) == NONE )
		sprintf(error_message, "%s Targeted operation not supported in %s\n", FILE_SYSTEM_ERROR_STR, FILE_SYSTEM_COMMAND);
	else
		sprintf(error_message, "%s provided parameters are invalid\n", FILE_SYSTEM_ERROR_STR);

	perror(error_message);
}

void reportErrorMakeFileSystem(const char** argv, int argc){

	errno = 1;
	char error_message[100];

	if( argc!=3 )
		sprintf(error_message, "%s Each command will have at exactly 2 arguments\n", FILE_SYSTEM_ERROR_STR);
	else if( !isInteger(argv[1]) )
		sprintf(error_message, "%s First argument needs to be an integer\n", FILE_SYSTEM_ERROR_STR);
	else if( !hasDotDataExtension(argv[2]) )
		sprintf(error_message, "%s  => File system's name needs to have .data extension\n", FILE_SYSTEM_ERROR_STR);

	perror(error_message);
}

void reportErrorFilSystemCommand(const char** argv, int argc){
	errno = 1;
	char error_message[100];

	if( argc<3 )
		sprintf(error_message, "%s Each command will have at least 2 arguments\n", FILE_SYSTEM_ERROR_STR);	
	else{//check command is file systeem operation
		Operation operation = operationFromString(argv[2]);

		if( !hasDotDataExtension(argv[1]) )
			sprintf(error_message, "%s  => File system's name (1st argument) needs to have .data extension\n", FILE_SYSTEM_ERROR_STR);
		else if( operation == NONE )
			sprintf(error_message, "%s  => Operation(2nd argument) not valid\n", FILE_SYSTEM_ERROR_STR);
		else{
			reportOperationError( operation, argc );
		}
	}
	

	perror(error_message);
}

int isFileSystemPresent(const char* fileSystemName){
	for( int i = 0; i<n_file_systems; ++i )
		if( strcmp(fileSystemName, file_system_names[i]) == 0 )
			return 1;
	return 0;
}

//max params are set to 5, meannig at most 5 strings separated by ' ' can be given
int getUserInput(char* outputStringArray[MAX_PARAMS] ,char delimiter){

	for( int i = 0; i<MAX_PARAMS; ++i )
		outputStringArray[i] = NULL;


	char* input = NULL;
	size_t size = 0;
	getline(&input, &size, stdin);
	input[strlen(input)-1] = '\0';//remove newline character!

	int param_count = 0;
	int substring_length = 0;	


	int i = 0;
	int exit_flag = 0;

	while( !exit_flag ){
		++substring_length;
		if( input[i] == delimiter || input[i] == '\0' ){		
				
			outputStringArray[param_count] = (char*)malloc(sizeof(char)*substring_length);//substring_lenght is with \0 character

			if(input[i] == '\0')
				exit_flag = 1;
			else
				input[i] = '\0';//cut it in order to copy it
		
			strcpy(outputStringArray[param_count], (input + i - (substring_length - 1)));

			param_count++;
			substring_length = 0;
		}

		++i;
	}

	free(input);

	return param_count;
	
}

void freeUserInput(char* userInput[MAX_PARAMS], int n){
	for( int i = 0; i<n; ++i )
		free(userInput[i]);
}

int isValidFileName(const char* filename){
	if( strlen(filename) > 14 )
		return 0;

	for( int i = 0; filename[i]; ++i )
		if( filename[i] == '/' )
			return 0;
	
	return 1;
}

void printTime(time_t time){
	struct tm tm = *localtime(&time);
	printf("%d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

int stringToInt(const char* str){
	return (int)strtol(str, (char **)NULL, 10);
}

FILE* getFile(const char* mode, const char* filename){
	FILE* fp = NULL;
	if( (fp = fopen(filename, mode)) == NULL ){ 
		printf("Cannot open '%s'. Make sure you have the right priviledges to do so\n", filename);
		exit(1);//create file as a disk, new data is always appended

	}
	return fp;
}

void closeFile( FILE* fp ){
	if (fclose(fp) == EOF) exit(1);
}

int seekUtil(FILE* fp, long offset, int whence){
	int ret_value;
	if( (ret_value = fseek(fp, offset, whence)) != 0 ) exit(1);
	return ret_value;
}

void setBitmap(bitmap_t b, int i) {
    b[i / 8] |= 1 << (i & 7);
}

void unsetBitmap(bitmap_t b, int i) {
    b[i / 8] &= ~(1 << (i & 7));
}

int getBitmap(bitmap_t b, int i) {
    return b[i / 8] & (1 << (i & 7)) ? 1 : 0;
}

bitmap_t createBitmap(int n) {
    return malloc((n + 7) / 8);
}

uint32_t sizeBitmap(int n){
	return ((n + 7) / 8);
}

size_t fwriteUtil(const void *ptr, size_t size, size_t nmemb, FILE *stream){
	if(size == 0 || nmemb == 0) return 0;

	size_t bytesWritten;
	bytesWritten = fwrite( ptr, size, nmemb, stream);
	
	return bytesWritten;
}

int writeUtil(int fd, const void *buf, size_t count){
	int written = write(fd, buf, count);

	if( written == -1 ){
		printf("Error while writing to file");
		exit(1);
	}

	return written;
}

int readUtil(int fd, void *buf, size_t count){

	printf("WRITTEN %d\n", fd);
	// exit(1);

	int written = read(fd, buf, 1);
	
	
	if( written == -1 ){
		printf("Error while writing to file");
		exit(1);
	}

	return written;
}

int filenoUtil(FILE *stream){
	int fd = fileno(stream);
	if( fd == -1 ){
		printf("Stream can't be identified with any of the file descriptors\n");
		exit(1);
	}

	return fd;
}

size_t freadUtil(void *ptr, size_t size, size_t nmemb, FILE *stream){
	if(size == 0 || nmemb == 0) return 0;
	size_t bytesWritten;
	
	if( (bytesWritten = fread( ptr, size, nmemb, stream)) == 0 ) exit(1);

	return bytesWritten;
}

void printBitmap(bitmap_t bitmap, int n){
	for( int i = 0; i<n; ++i ){
		printf("%d ",getBitmap(bitmap, i));
	}
}

void intToCharArray(char dest[4], int num){
	dest[0] = (num >> 24) & 0xFF;
	dest[1] = (num >> 16) & 0xFF;
	dest[2] = (num >> 8) & 0xFF;
	dest[3] = num & 0xFF;
}

uint32_t charArrayToInt(char bytes[4]){

	uint32_t myNum = ((uint8_t)bytes[0] << 24) + ((uint8_t)bytes[1] << 16) + ((uint8_t)bytes[2] << 8) + (uint8_t)bytes[3];

	return myNum;
}

int countCharOccurencesInString(const char* str, char ch){
	int count = 0;

	for( int i = 0; str[i]; ++i )
		if( str[i] == ch )
			++count;

	return count;
}