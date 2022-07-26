/*this library works on a mounted system, undefined behaviour otherwise*/
#include "fileSystem.h"
#include "utils.h"
#include "memoryManagement.h"
#include "makeFileSystem.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// int getInodeByFileName(char* path, const char* fileSystemName);//returns inode number of directory, -1 otherwise
// bool dirExistsCurrent(const char* directoryName, const char* fileSystemName);
const char* getNewFileName(char* path);
bool validCommandPath(const char* path);
void rmDirHelper(int inodeNumber, FILE* fp);
void writeContentToFile(int inodeNumber, const char* srcFile, FILE* fp);
void outputInodeFileContentToFile(int inodeNumber, const char* srcFileName, FILE* fp);

bool dir(const char* path, FILE* fp){

	char pathCopy[strlen(path)+1];
	strcpy(pathCopy, path);

	int inodeNumber = getInodeByPath(pathCopy, fp);

	if( (int16_t)inodeNumber != -1 )
		printDirectoryContentOptimized(inodeNumber, fp);
	else
		printf("dir: cannot access \'%s\': No such file or directory\n", path);

	return inodeNumber == -1 ? FALSE : TRUE;
}

bool mkdir(const char* path, FILE* fp){
	
	if( !validCommandPath(path) ) return FALSE;//only 1 character( root ), discard it	

	char pathCopy[strlen(path)+1];//copy the path, will use it in strtok
	strcpy(pathCopy, path);

	const char* newDirectoryName = getNewFileName(pathCopy);//get file name
	char* targetPath = pathCopy;//get path

	int inodeNumber = getInodeByPath(targetPath, fp);//get parent inode
	if( (int16_t)inodeNumber != -1 ){
		if( !containsFile(inodeNumber, newDirectoryName, fp) ){
			addNewDirectoryEntryOptimized(inodeNumber, newDirectoryName, DIRECTORY, fp);
		}
		else{
			printf("mkdir: cannot create directory ‘%s’: File exists\n", newDirectoryName);
			return FALSE;
		}
	}else{
		printf("dir: cannot access \'%s\': No such file or directory\n", path);
		return FALSE;
	}

	return TRUE;
}


bool rmdir(const char* path, FILE* fp){

	if( fp == NULL ) return FALSE;
	if( !validCommandPath(path) ) return FALSE;//only 1 character( root ), discard it	
	
	char pathCopy[strlen(path)+1];//copy the path, will use it in strtok
	strcpy(pathCopy, path);
	int inodeNumber = getInodeByPath(pathCopy, fp);//get inode of target directory
	
	if( (int16_t)inodeNumber != -1 ){
		const char* newDirectoryName = getNewFileName(pathCopy);//get file name
		if( !containsFile(inodeNumber, newDirectoryName, fp) ){

			addNewDirectoryEntryOptimized(inodeNumber, newDirectoryName, DIRECTORY, fp);
		}
		else{
			printf("rmdir: cannot remove directory ‘%s’: File exists\n", newDirectoryName);
			return FALSE;
		}
	}else{
		printf("dir: cannot access \'%s\': No such file or directory\n", path);
		return FALSE;
	}
	
	
	rmDirHelper(inodeNumber, fp);//recursively invalidates all of the inner files' blocks

	invalidateDirectoryEntry(path ,fp);
	return TRUE;
}

void invalidateDirectoryEntry(const char* path , FILE* fp){
	if( !validCommandPath(path) ) return;//only 1 character( root ), discard it	

	char pathCopy[strlen(path)+1];//copy the path, will use it in strtok
	strcpy(pathCopy, path);

	const char* newDirectoryName = getNewFileName(pathCopy);//get file name
	char* targetPath = pathCopy;//get path

	int parentInodeNumber = getInodeByPath(targetPath, fp);//get parent inode
	Inode parentInode = getInodeOptimized(parentInodeNumber, fp);
	// int parentInodeNumber = getInodeByPath()
	DirectoryEntry directoryEntry = getDirectoryEntryFromFilename(&parentInode, newDirectoryName, fp);
	directoryEntry.inodeNumber = -1;
	syncDirectoryEntryOptimized(&parentInode, &directoryEntry, fp);

}

void rmDirHelper(int inodeNumber, FILE* fp){//recursively delete all the in the directory
	
	Inode inode = getInodeOptimized(inodeNumber, fp);
	int nDirectoryEntires = countDirectoryEntries(&inode);

	for( int i = 2; i<nDirectoryEntires; ++i){//skip "." and ".." -> start from 2
		
		Inode dirEntryInode = getDirectoryEntryInode(i, &inode, fp);//needed to aquire inode type
		DirectoryEntry dirEntry = getDirectoryEntryFromPosition(&inode, i, fp);//needed to aquire inode number
		int directoryEntryInodeNumber = dirEntry.inodeNumber;

		if( (int16_t)dirEntry.inodeNumber != -1 ){//skip invalidated ones that make fragmentation
			if( dirEntryInode.type == DIRECTORY )
				rmDirHelper( directoryEntryInodeNumber, fp );
			else
				unclaimFile(directoryEntryInodeNumber, fp);
		}
	}
	unclaimFile(inodeNumber, fp);//unclaim this directory itself
}


void dumpe2fs(FILE* fp){
	printf("Block count: %d\n", superBlock.nBlocks);
	printf("Free blocks: %d\n", superBlock.nEmptyBlocks);
	printf("Used blocks: %d\n", superBlock.nFullBlocks);
	printf("Block size: %d\n", superBlock.blockSize);
	printf("Number of files and directories: %d\n", superBlock.nFullInodes);
	//stub
}

bool write(const char* path, const char* srcFile, FILE* fp){

	if( !validCommandPath(path) ) return FALSE;//only 1 character( root ), discard it	

	char pathCopy[strlen(path)+1];//copy the path, will use it in strtok
	strcpy(pathCopy, path);

	const char* newFileName = getNewFileName(pathCopy);//get file name
	char* targetPath = pathCopy;//get path


	int inodeNumber = getInodeByPath(targetPath, fp);//get parent inode
	if( (int16_t)inodeNumber != -1 ){
		if( !containsFile(inodeNumber, newFileName, fp) ){
			int newFileInode = addNewDirectoryEntryOptimized(inodeNumber, newFileName, REGULAR_FILE, fp);//adds new file into desired directory
			writeContentToFile(newFileInode, srcFile, fp);
		}
		else{
			printf("write: cannot write file ‘%s’: File exists\n", newFileName);
			return FALSE;
		}
	}else{
		printf("dir: cannot access \'%s\': No such file or directory\n", targetPath);
		return FALSE;
	}

	return TRUE;
	
}

void writeContentToFile(int inodeNumber, const char* srcFile, FILE* fp){//reads content byte by byte
	
	FILE* fpSrcFile = getFile("r", srcFile);
	if( fpSrcFile == NULL ){
		printf("File named: %s doesn't exist\n", srcFile);
		exit(1);
	}
	char ch;

	while( (ch=fgetc(fpSrcFile))!=EOF )
		writeToFileOptimized(inodeNumber, &ch, 1, fp);

	closeFile(fpSrcFile);
}


bool read(const char* path, const char* destFileName, FILE* fp){//reading data 1 block at a time
	
	if( !validCommandPath(path) ) return FALSE;//only 1 character( root ), discard it	

	char pathCopy[strlen(path)+1];//copy the path, will use it in strtok
	strcpy(pathCopy, path);

	int inodeNumber = getInodeByPath(pathCopy, fp);//get file's inode
	outputInodeFileContentToFile(inodeNumber, destFileName,fp);
	
	return TRUE;
}

void outputInodeFileContentToFile(int inodeNumber, const char* destFileName, FILE* fp){//reading data block by block
	FILE* fpDestFile = getFile("w+", destFileName);

	Inode inode = getInodeOptimized(inodeNumber, fp);
	int totalBytesToWrite = inode.attributes.fileSize;

	if( fpDestFile == NULL ){
		printf("File named: %s doesn't exist\n", destFileName);
		exit(1);
	}
	
	int blockIndex = 0;//in files, data is contagious, there is not shrinking / expanding in this homework
	int offset = 0;

	while( totalBytesToWrite > 0 ){

		int block = -1;
		if( isIndirectlyAddreessed(&inode) && blockIndex >= INODE_DB_N - 1 ) {
			int indirectBlockIndex = blockIndex - (INODE_DB_N - 1);
			int indirectBlock = inode.diskBlocks[INODE_DB_N-1];
			uint16_t dataBlock;
			readFromBlockOptimized(indirectBlock, indirectBlockIndex*sizeof(uint16_t), &dataBlock, sizeof(uint16_t), fp);
			block = dataBlock;
		}else
			block = inode.diskBlocks[blockIndex];
	
		char ch;
		readFromBlockOptimized(block, offset, &ch, 1, fp);
		fputc(ch, fpDestFile);
		--totalBytesToWrite;
		++offset;
		offset %= superBlock.blockSize;
		blockIndex = (inode.attributes.fileSize - totalBytesToWrite)/superBlock.blockSize;
	}

	closeFile(fpDestFile);
}

void writeDirectlyAddressed(){

}


bool del(const char* path, FILE* fp){

	char pathCopy[strlen(path)+1];//copy the path, will use it in strtok
	strcpy(pathCopy, path);

	int inodeNumber = getInodeByPath(pathCopy,fp);
	if( (int16_t)inodeNumber != -1 ){
		const char* newDirectoryName = getNewFileName(pathCopy);//get file name
		if( !containsFile(inodeNumber, newDirectoryName, fp) ){

			addNewDirectoryEntryOptimized(inodeNumber, newDirectoryName, DIRECTORY, fp);
		}
		else{
			printf("del: cannot remove directory ‘%s’: File exists\n", newDirectoryName);
			return FALSE;
		}
	}else{
		printf("dir: cannot access \'%s\': No such file or directory\n", path);
		return FALSE;
	}


	Inode inode = getInodeOptimized(inodeNumber, fp);




	if( inode.type == REGULAR_FILE )
		return rmdir(path, fp);
	else
		printf("rm: cannot remove '%s': Is a directory\n", path);
	
	return FALSE;
}


const char* getNewFileName(char* path){//get last name, usr\ysf -> ysf is returned
	
	if( validCommandPath(path) && countCharOccurencesInString(path, '\\') >= 1 ){
		int i = strlen(path)-1;

		while( path[i] != '\\' ) --i;
		path[i] = '\0';
		return path + i + 1;
	}

	return NULL;
}

bool validCommandPath(const char* path){
	if( path[0] != '\\' || strlen(path) == 1 ) return FALSE;//only 1 character( root ), discard it

	return TRUE;
}
