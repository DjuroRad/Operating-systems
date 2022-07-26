/**
 * Your file attributes will include size
 * file creation time
 * last modification date and time
 * and name of the file.
 * */
#include <stddef.h>
#include <time.h>
#include <stdint.h>
#include "utils.h"

#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H

#define FILENAME_LENGTH 14//tracing \0 at the end, not present if file is 14 characters long
#define INODE_SIZE 2
#define MAX_FILESYSTEM_SIZE 16777216 //number of bytes -> it is 2^24 bytes or 16MB
#define BYTES_IN_KB 1024
//maximum number of files is 64K due to these limitations -> 14 + 2(bytes->16bits) => 2^16(16bits) - 1 = 64K
#define INODE_DB_N 10//number of diskblocks contained within a given inode
#define UNIX_MAX_INODES 65535//unix's design separates 2 bytes for each inode, 2^16 makes maximum of 65536 inodes. 
#define ROOT_INODE 0

int BLOCK_SIZE;


typedef enum FileType{DIRECTORY, REGULAR_FILE}FileType;

typedef struct DiskBlock{
	uint8_t* data;//depends on BLOCK SIZE initialization
	uint32_t next_disk_block;
}DiskBlock;

typedef struct FileAttributes{
	uint32_t fileSize;
	time_t timeCreation;
	time_t timeLastModification;
}FileAttributes;

typedef struct DirectoryEntry{
	char filename[FILENAME_LENGTH];
	uint16_t inodeNumber;
}DirectoryEntry;

typedef DirectoryEntry* Directory;//directory is just an array of directory entries


//important

typedef struct Inode{
	FileAttributes attributes;
	FileType type;
	uint16_t diskBlocks[INODE_DB_N];
}Inode;

typedef struct SuperBlock{
	uint32_t fileSystemSize;
	uint32_t blockSize;
	uint32_t usableBlockSizeInodeTable;

	uint16_t nBlocks;
	uint16_t nEmptyBlocks;
	uint16_t nFullBlocks;

	uint16_t nInodes;
	uint16_t sizeInode;
	uint16_t nEmptyInodes;
	uint16_t nFullInodes;
	uint32_t blockCountInodeTable;

	uint32_t blockCountInodeBitmap;
	uint32_t blockCountBlockBitmap;

	uint32_t firstDataBlock;
	uint32_t maxFileSizeDirectAddresing;
	uint32_t maxFileSizeIndirectAddresing;
}SuperBlock;

bitmap_t freeInodes;//due to limitations of the file system, maximum memory required for this would be 16MB/1KB = 16K bits = 2048bytes = 2KB
bitmap_t freeBlocks;//due to limitations of the file system, maximum memory required for this would be 16MB/1KB = 16K bits = 2048bytes = 2KB

Inode createInode(FileType fileType);
DirectoryEntry createDirectoryEntry(uint16_t inodeNumber, const char* filename);

void syncInode(uint16_t inode_num, Inode modifiedInode);
void newInode();

void writeInode(uint16_t i, const Inode* inode, const char* fileSystemName);
void writeInodeOptimized(uint16_t i, const Inode* inode,  FILE* fp);//opening and closing the file is done by caller
void writeToBlock(uint16_t block_num, uint32_t offset, const void* src, uint16_t num_bytes, const char* fileSystemName);
void writeToBlockOptimized(uint16_t block_num, uint32_t offset, const void* src, uint16_t num_bytes, FILE* fileSystemName);//opening and closing the file is done by caller
void fWriteToBlockOptimized(uint16_t block_num, uint32_t offset, const void* src, uint16_t num_bytes, int nmemb, FILE* fp);
void writeEmptyDirectory(uint16_t inode_num, const char* fileSystemName);
void writeToFile(uint16_t inode_num, const void* src, size_t size, const char* fileSystemName);//best to make size factor of 2
void writeToFileOptimized(uint16_t inode_num, const void* src, size_t size, FILE* fp);//best to make size factor of 2
void writeUsingDirectAddressing( Inode* inode, int newFileSize, const void* src, size_t size, FILE* fp);
void writeUsingIndirectAddressing( Inode* inode, int newFileSize, const void* src, size_t size, FILE* fp);//use factor of 2 for size when writing this data
void writeToIndirectBlock( int indirectlyAddresedBlock, const Inode* inode, int newFileSize, const void* src, size_t size, FILE* fp);//adds data to indirect block
int writeInitNewDirectory(uint16_t parentInode, FILE* fp);
int writeInitNewRegularFile(uint16_t parentInode, FILE* fp);//returns new inode number
void writeDirectoryEntry( const DirectoryEntry* directoryEntry, uint16_t inodeNumber, const char* fileSystemName );
void writeDirectoryEntryOptimized( const DirectoryEntry* directoryEntry, uint16_t inodeNumber, FILE* fp );
void syncDirectoryEntryOptimized( const Inode* parentInode, const DirectoryEntry* directoryEntry, FILE* fp );
int addNewDirectoryEntryOptimized(uint16_t inodeNumber, const char* fileName, FileType fileType, FILE* fp);

void readInode(FILE* fp, Inode* inode);
void readFromBlock(uint16_t blockNumber, uint32_t offset, void* dest, size_t size, const char* fileSystemName);
void readFromBlockOptimized(uint16_t blockNumber, uint32_t offset, void* dest, uint16_t size, FILE* fp);

Inode getInode( uint16_t inode_num, const char* fileSystemName );
Inode getInodeOptimized(uint16_t inode_num, FILE* fp);
uint16_t getInodeBlock(uint16_t inode_num);
uint32_t getInodeBlockOffset(uint16_t inode_num);
int getFreeBlockPosition(const Inode* inode);
int getPartiallyFilledBlockNumber(const Inode* inode);//returns last, partially filled block. When no blocks allocated returns -1
int getBlockNumberFromIndex(const Inode* inode, int i, FILE* fp);//filesystemname required since it can be indirectly addresses which would mean access to memory is a must in order to figure out what is the block number
int getBlockNumberFromIndirectBlock(const Inode* inode, int blockNumber, FILE* fp);//assumes previous check for boundaries
int getDirectoryEntryInodeNumber(const Inode* parentInode, const char* targetFileName, FILE* fp);
int getDirectoryEntryBlockIndex(int directoryEntryIndex);
int getDirectoryEntryBlockOffset(int directoryEntryIndex);
Inode getDirectoryEntryInode(int i, const Inode* inode, FILE* fp);
int getInodeByPath(char* path, FILE* fp);//returns inode number of directory, -1 otherwise
DirectoryEntry getDirectoryEntry( uint16_t blockNumber, int offset, const char* fileSystemName );
DirectoryEntry getDirectoryEntryFromPosition(const Inode* parentInode, int i, FILE* fp);
DirectoryEntry getDirectoryEntryFromFilename(const Inode* parentInode, const char* filename, FILE* fp);
DirectoryEntry getDirectoryEntryOptimized( uint16_t blockNumber, uint32_t offset, FILE* fp );
int getLastIndirectBlockNumber(const Inode* inode, FILE* fp);//assumes previous check for indirect addressing
int getLastBlockNumberFromIndirectBlock(int indirectBlock, FILE* fp);//assumes previous check for indirect addressing

void printDirectoryContent(uint16_t inodeNumber, const char* fileSystemName);
void printDirectoryContentOptimized(uint16_t inodeNumber, FILE* file);

void claimBlock(uint16_t block_num);
void claimInode(uint16_t inode_num);
void unclaimBlock(uint16_t block_num);
void unclaimInode(uint16_t inode_num);
void unclaimFile(int inodeNumber, FILE* fp);
int getAndClaimEmptyBlock();//gets and claims an empty block
int getAndClaimEmptyInode();//gets and claims an empty block
void syncAndUnclaimInode(int inodeNumber, const Inode* inode, FILE* fp);
void invalidateDirectoryEntry(const char* path , FILE* fp);

uint32_t countBlocks(const Inode* inode);
int countIndirectBlocks(int indirectBlock, FILE* fp);//assumes previous check for indirect addressing of the inode
uint32_t countDirectoryEntries(const Inode* inode);

int isInodeClaimed(uint16_t inode_num);
bool isIndirectlyAddreessed(const Inode* inode);

int fillPartialFilledBlock(int blockNumber, const Inode* inode,const void* src, size_t size, FILE* fp);
int fillNewBlocks(Inode* inode, int newBlocksCoount, const void* src, size_t size, FILE* fp);
int fillPartialFilledBlockDirectAddressing(const Inode* inode, const void* src, size_t size, FILE* fp);
int fillPartialFilledBlockIndirectAddressing(int indirectlyAddresedBlock, const Inode* inode, const void* src, size_t size, FILE* fp);
int fillNewBlocksIndirectAddressing(int indirectBlockNumber, int newBlocksCoount, const void* src, size_t size, FILE* fp);

bool fileNameCmp(const char* fn1, const char* fn2);
void fileNameCopy(char* dest, const char* src);
int fileNameLength(const char* fn);

bool containsFile(uint16_t inodeNumber, const char* filename, FILE* fp);
void printInode(const Inode* inode);
void printSuperBlock();
void printFileName(const char* filename);

SuperBlock superBlock;
Inode rootInode;

#endif
