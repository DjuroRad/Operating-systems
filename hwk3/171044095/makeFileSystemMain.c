#include "makeFileSystem.h"
#include "utils.h"
#include "memoryManagement.h"

int main(int argc, const char** argv){

	if( isMakeFileSystemCommand(argv, argc) ){

		int passedNumber = stringToInt(argv[1]);
		const char* fileSystemName = argv[2];

		printf("Formating your hard disk. Please wait, it might take a while.\n");
		createFileSystem(passedNumber, fileSystemName);
		printf("Format finished. File system is mounted.\n");

		// mountFileSystem(fileSystemName);
		
		// printFileSystemInfo();

	}	
	else
		reportErrorMakeFileSystem(argv, argc);
}