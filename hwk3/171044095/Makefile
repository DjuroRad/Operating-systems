FILE_SYSTEM:
	gcc -std=c99 -Wall utils.c memoryManagement.c makeFileSystem.c fileSystem.c fileSystemMain.c -o fileSystemOper

MAKE_FILE_SYSTEM: 
	gcc -std=c99 -Wall utils.c memoryManagement.c makeFileSystem.c makeFileSystemMain.c -o makeFileSystem

clear: 
	rm makeFileSystem fileSystemOper *.data 

all: MAKE_FILE_SYSTEM FILE_SYSTEM