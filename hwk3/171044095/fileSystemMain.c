#include "utils.h"
#include "memoryManagement.h"
#include "makeFileSystem.h"
#include "fileSystem.h"
#include <string.h>
#include <dirent.h> //required for current directory searches

bool fileExists(const char* fileSystemName);

int main(int argc, const char **argv){

	if( isFileSystemOperationCommand(argv, argc) ){
		if( fileExists(argv[1]) ){
			
			const char* fileSystemName = argv[1];	
  			mountFileSystem(argv[1]);//mount the file system, mounts superblock and free block bitmaps

  			Operation operation = operationFromString(argv[2]);
  			switch(operation){
  				case mDIR:{//checked operation is DIR && that .data extension?
	  					FILE* fp = getFile("r", fileSystemName);
	  					dir(argv[3], fp);
	  					unmountFileSystem(argv[1]);//sync the file system, mounts superblock and free block bitmaps
	  				}
  					break;
				case MKDIR:{
						FILE* fp = getFile("r+", fileSystemName);
						mkdir(argv[3], fp);
						unmountFileSystem(argv[1]);//sync the file system, mounts superblock and free block bitmaps
					}
					break;
				case RMDIR:{//to delete a directory
						FILE* fp = getFile("r+", fileSystemName);
						rmdir(argv[3], fp);						
						unmountFileSystemOptimized(fp);//sync the file system, mounts superblock and free block bitmaps
					}
					break;

				case DUMPE2FS:{	
						FILE* fp = getFile("r", fileSystemName);
						dumpe2fs(fp);
						unmountFileSystemOptimized(fp);//sync the file system, mounts superblock and free block bitmaps
					}
					break;

				case WRITE:{
						FILE* fp = getFile("r+", fileSystemName);
						write(argv[3], argv[4], fp);
						unmountFileSystemOptimized(fp);//sync the file system, mounts superblock and free block bitmaps
					}
					break;

				case READ:{
						FILE* fp = getFile("r+", fileSystemName);
						read(argv[3], argv[4], fp);
						unmountFileSystemOptimized(fp);//sync the file system, mounts superblock and free block bitmaps
					}
					break;

				case DEL:{//to delete a file
						FILE* fp = getFile("r+", fileSystemName);
						del(argv[3], fp);
						unmountFileSystemOptimized(fp);//sync the file system, mounts superblock and free block bitmaps
					}
					break;	

				case NONE:
					//do nothing
					break;
  			}

		}
		else
			printf("Error: file system with such a name doesn't exist.\n");

	}else
		reportErrorFilSystemCommand(argv, argc);
		
	return 0;
}

bool fileExists(const char* fileSystemName){
	DIR *currentDirectory;
	
	struct dirent *directoryEntry;
	currentDirectory = opendir(".");
	
	if (currentDirectory) {
		while ((directoryEntry = readdir(currentDirectory)) != NULL) 
			if( strcmp(directoryEntry->d_name, fileSystemName)==0 )
				return TRUE;
		
		closedir(currentDirectory);
	}

	return FALSE;
}