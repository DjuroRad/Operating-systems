#include "utils.h"
#include "memoryManagement.h"
#include "makeFileSystem.h"
#include <stdio.h>
#include <stdlib.h>

void setupSuperBlock(const char* fileSystemName);
void writeBootBlock(const char* fileSystemName);
void writeEmptyInodes(const char* fileSystemName);
void createEmptyFile(const char* fileSystemName);
void setUpFreeSpace(const char* fileSystemName);
void setUpRootDirectory(const char* fileSystemName);
void writeBlockAndInodeBitmap(const char* fileSystemName);
void writeBlockAndInodeBitmapOptimized(FILE* fp);//writes bitmap of free elements
void writeRootDirectory(const char* fileSystemName);
void writeSuperBlock(const char* fileSystemName);
void writeSuperBlockOptimized(FILE* fp);

void initGlobal(int blockSize);
void freeGlobal();
void initSuperBlock();
void initBlockAndInodeBitmap();

void mountSuperBlock(const char* fileSystemName);
void mountBlockAndInodeBitmap(const char* fileSystemName);
void mountRootDirectory(const char* fileSystemName);
void mountBootBlock(const char* fileSystemName);

void claimBitmapBlocks();

void createFileSystem(int blockSize, const char* fileSystemName){

	initGlobal(blockSize);//initializes all necessery variables that will be written

	createEmptyFile(fileSystemName);//writes a 16Mb file representing our secondary memory
	writeBootBlock(fileSystemName);//writes an empty block for the first block
	writeEmptyInodes(fileSystemName);//write empty inodes into the file system, 
	writeRootDirectory(fileSystemName);
	writeSuperBlock(fileSystemName);

	claimBitmapBlocks();
	writeBlockAndInodeBitmap(fileSystemName);//writes bitmap of free elements

	freeGlobal();//frees initialized variables to avoid memory leak
}

void claimBitmapBlocks(){

	for( int i = 0; i<superBlock.blockCountInodeBitmap; ++i )
		claimBlock(1 + 1 + superBlock.blockCountInodeTable + i);

	for( int i = 0; i<superBlock.blockCountBlockBitmap; ++i )
		claimBlock(1 + 1 + superBlock.blockCountInodeTable + superBlock.blockCountInodeBitmap + i);

}


void unmountFileSystem(const char* fileSystemName){
	FILE* fp = getFile("r+", fileSystemName);
		unmountFileSystemOptimized(fp);
	closeFile(fp);
}

void unmountFileSystemOptimized(FILE* fp){
	writeSuperBlockOptimized(fp);
	writeBlockAndInodeBitmapOptimized(fp);//writes bitmap of free elements
	free(freeInodes);
	free(freeBlocks);
}

void writeRootDirectory(const char* fileSystemName){//claim root inode, get free block in which data will be stored
	
	claimInode(ROOT_INODE);//claim root inode
	writeInode(0, &rootInode, fileSystemName);

	DirectoryEntry directoryEntry1 = { ".", ROOT_INODE };
	DirectoryEntry directoryEntry2 = { "..", ROOT_INODE };

	writeDirectoryEntry( &directoryEntry1, ROOT_INODE, fileSystemName );
	writeDirectoryEntry( &directoryEntry2, ROOT_INODE, fileSystemName );

	// writeToFile(ROOT_INODE, &directoryEntry1, sizeof(DirectoryEntry), fileSystemName);//claims blocks if neccessary, updates the inode
	// writeToFile(ROOT_INODE, &directoryEntry2, sizeof(DirectoryEntry), fileSystemName);//claims blocks if neccessary, updates the inode
}


void writeBlockAndInodeBitmapOptimized(FILE* fp){//writes bitmap of free elements

	uint32_t freeInodesSize = sizeBitmap(superBlock.nInodes);
	uint32_t freeBlocksSize = sizeBitmap(superBlock.nBlocks);

	int freeInodesBlocksRequired = freeInodesSize/BLOCK_SIZE;// 1-2 blocks in our case
	int freeBlocksBlocksRequired = freeBlocksSize/BLOCK_SIZE;// 1-2 blocks in our case

	int start_block_bitmap_inode = 1 + 1 + superBlock.blockCountInodeTable;//((superBlock.nInodes*sizeof(Inode))/superBlock.blockSize);// boot block + super block + inode blocks 
	int start_block_bitmap_blocks = 1 + 1 + superBlock.blockCountInodeTable + superBlock.blockCountInodeBitmap;//((superBlock.nInodes*sizeof(Inode))/superBlock.blockSize) + freeInodesBlocksRequired;// boot block + super block + inode blocks 
	if( start_block_bitmap_inode == start_block_bitmap_blocks )//put them in separate blocks
		start_block_bitmap_blocks ++;
	
	for( int i = 0; i<=freeInodesBlocksRequired && freeInodesSize>0; ++i ){//write bitmap of free inodes
			uint32_t bytes_to_write = 0;
			if( freeInodesSize > superBlock.blockSize )
				bytes_to_write = superBlock.blockSize;
			else
				bytes_to_write = freeInodesSize;

			writeToBlockOptimized(start_block_bitmap_inode+i, 0, (const void*)(freeInodes + i*superBlock.blockSize), bytes_to_write, fp);

			freeInodesSize -= bytes_to_write;
	}

	for( int i = 0; i<=freeBlocksBlocksRequired && freeBlocksSize>0; ++i ){//write bitmap of free blocks
			uint32_t bytes_to_write = 0;
			if( freeBlocksSize > superBlock.blockSize )
				bytes_to_write = superBlock.blockSize;
			else
				bytes_to_write = freeBlocksSize;

			writeToBlockOptimized(start_block_bitmap_blocks+i, 0, (freeBlocks + i*superBlock.blockSize), bytes_to_write, fp);

			freeBlocksSize -= bytes_to_write;
	}
}


void printFileSystemInfo(){
	
	printf("\nfree inodes\n");
	printBitmap(freeInodes, superBlock.nInodes);
	printf("\n");

	printf("\nfree blocks\n");
	printBitmap(freeBlocks, superBlock.nBlocks);
	printf("\n");

	printf("\nRoot inode\n");
	printInode(&rootInode);
	printf("\n");

	printf("\nSuper block\n");
	printSuperBlock();
}


void writeBlockAndInodeBitmap(const char* fileSystemName){

	FILE* fp = getFile("r+", fileSystemName);
		writeBlockAndInodeBitmapOptimized(fp);
	closeFile(fp);
}

void writeSuperBlock(const char* fileSystemName){

	claimBlock(1);
	writeToBlock(1, 0, &superBlock, sizeof(SuperBlock), fileSystemName);

	//super block needs to be written inside
}

void writeSuperBlockOptimized(FILE* fp){
	writeToBlockOptimized(1, 0, &superBlock, sizeof(SuperBlock), fp);
}

int blocksNeeded(int byteCount){
	return byteCount/BLOCK_SIZE;
}

void createEmptyFile(const char* fileSystemName){
	
	char* diskData = NULL;

	if( (diskData = (char*)calloc(1, 16777216)) == NULL ) exit(1);//allocate all the data needed for disk blocks

	FILE* fpFileSystem = NULL;
	if( (fpFileSystem = fopen(fileSystemName, "w")) == NULL ) exit(1);//create file as a disk, new data is always appended
	fwriteUtil(diskData, 16777216, 1, fpFileSystem);//will write an empty file, each entry will be 0
	closeFile(fpFileSystem);
}

void writeBootBlock(const char* fileSystemName){//stub function, implementing this block is not part of the course

	claimBlock(0);//claim block 0
	char* emptyBlock;
	if( (emptyBlock = (char*)calloc(1, superBlock.blockSize)) == NULL ) exit(1);
	
	intToCharArray(emptyBlock, superBlock.blockSize);

	writeToBlock(0, 0, emptyBlock, superBlock.blockSize, fileSystemName);//super block to be written in the very first block

	free(emptyBlock);

}



void writeEmptyInodes(const char* fileSystemName){
	
	int inodeBlocks = (superBlock.sizeInode*superBlock.nInodes / superBlock.usableBlockSizeInodeTable) + 1;

	for( int i = 0; i<inodeBlocks; ++i )//claim blocks needed for inodes
		claimBlock(i+2);// 1st - boot block, 2nd for superblock

	Inode emptyInode = createInode(REGULAR_FILE);

	FILE* fp = getFile("r+", fileSystemName);

	for( uint16_t i = 0; i<superBlock.nInodes; ++i )
		writeInodeOptimized(i, &emptyInode, fp);//
		

	closeFile(fp);
}

void syncInode(uint16_t inode_num, Inode modifiedInode){
	// inodes always start at the 3rd block
	// 1st block -> boot block
	// 2nd block -> supernode
	


	// int startBlock = 2*superBlock.blockSize;//offset


	// char* block = diskData + block_number*BLOCK_SIZE;
	// char* inodeBlock

}

uint32_t geInodeStartPosition(uint16_t inode_num){

	uint16_t block_number = (inode_num * sizeof(Inode)) / superBlock.blockSize;
	uint32_t offset = (inode_num * sizeof(Inode)) % superBlock.blockSize;

	return (block_number*BLOCK_SIZE + offset);
}

void initGlobal(int blockSize){//block size initialized beforehand
	
	BLOCK_SIZE = blockSize*BYTES_IN_KB;//set block size
	rootInode = createInode(DIRECTORY);
	
	initSuperBlock();
	initBlockAndInodeBitmap();
}

void initBlockAndInodeBitmap(){
	
	freeInodes = createBitmap(superBlock.nInodes);
	freeBlocks = createBitmap(superBlock.nBlocks);
	//all are empty at the begining
	for( int i = 0; i<superBlock.nInodes; ++i )
		setBitmap(freeInodes, i);

	for( int i = 0; i<superBlock.nBlocks; ++i )
		setBitmap(freeBlocks, i);
}


void freeGlobal(){
	free(freeBlocks);
	free(freeInodes);
}


void initSuperBlock(){

	superBlock.fileSystemSize = MAX_FILESYSTEM_SIZE;//file system's size
	superBlock.blockSize = BLOCK_SIZE;//block size

	uint16_t n_blocks = MAX_FILESYSTEM_SIZE/BLOCK_SIZE;//number of blocks
	uint16_t n_inodes = UNIX_MAX_INODES;//number of inodes in UNIX is 2^16 since 2 bytes are reserved for inode number part of the directory entry
	if( n_inodes > n_blocks )//in our case this would be a complete overkill, so in order to balance this, I am decreasing number of inodes if necessary, it doesn't make sense to have more possible files than blocks
		n_inodes = n_blocks;

	superBlock.nBlocks = n_blocks;
	superBlock.nInodes = n_inodes;

	superBlock.nEmptyBlocks = superBlock.nBlocks;
	superBlock.nEmptyInodes = superBlock.nInodes;

	uint32_t sizeOfAttributes = sizeof(uint32_t) + 2*sizeof(time_t);//avoid aligment overhead
	superBlock.sizeInode = sizeOfAttributes + sizeof(FileType) + sizeof(uint16_t)*INODE_DB_N;

	superBlock.maxFileSizeDirectAddresing = INODE_DB_N*superBlock.blockSize;
	superBlock.maxFileSizeIndirectAddresing = (INODE_DB_N - 1 + superBlock.blockSize/sizeof(uint16_t))*superBlock.blockSize;
	superBlock.usableBlockSizeInodeTable = ( superBlock.blockSize / superBlock.sizeInode ) * superBlock.sizeInode;
	
	superBlock.blockCountInodeTable = superBlock.sizeInode*superBlock.nInodes / superBlock.usableBlockSizeInodeTable + 1;
	superBlock.blockCountInodeBitmap = (sizeBitmap(superBlock.nInodes)/superBlock.blockSize) + 1;
	superBlock.blockCountBlockBitmap = (sizeBitmap(superBlock.nBlocks)/superBlock.blockSize) + 1;

	superBlock.firstDataBlock = 1 + 1 + superBlock.blockCountInodeTable + superBlock.blockCountInodeBitmap + superBlock.blockCountBlockBitmap;
}

void mountFileSystem(const char* fileSystemName){
	mountBootBlock(fileSystemName);
	mountSuperBlock(fileSystemName);
	mountBlockAndInodeBitmap(fileSystemName);
	mountRootDirectory(fileSystemName);
}

void mountSuperBlock(const char* fileSystemName){
	//read block size only first, than continue
	superBlock.blockSize = BLOCK_SIZE;
	readFromBlock(1, 0, &superBlock, sizeof(SuperBlock), fileSystemName);
}

void mountBlockAndInodeBitmap(const char* fileSystemName){

	freeInodes = createBitmap(superBlock.nInodes);
	freeBlocks = createBitmap(superBlock.nBlocks);

	uint32_t freeInodesSize = sizeBitmap(superBlock.nInodes);
	uint32_t freeBlocksSize = sizeBitmap(superBlock.nBlocks);

	int freeInodesBlocksRequired = freeInodesSize/BLOCK_SIZE;// 1-2 blocks in our case
	int freeBlocksBlocksRequired = freeBlocksSize/BLOCK_SIZE;// 1-2 blocks in our case

	int start_block_bitmap_inode = 1 + 1 + superBlock.blockCountInodeTable;//((superBlock.nInodes*sizeof(Inode))/superBlock.blockSize);// boot block + super block + inode blocks 
	int start_block_bitmap_blocks = 1 + 1 + superBlock.blockCountInodeTable + superBlock.blockCountInodeBitmap;//((superBlock.nInodes*sizeof(Inode))/superBlock.blockSize) + freeInodesBlocksRequired;// boot block + super block + inode blocks 
	
	if( start_block_bitmap_inode == start_block_bitmap_blocks )//put them in separate blocks
		start_block_bitmap_blocks ++;


	for( int i = 0; i<=freeInodesBlocksRequired && freeInodesSize>0; ++i ){//write bitmap of free inodes
			uint32_t bytes_to_read = freeInodesSize > superBlock.blockSize ? superBlock.blockSize : freeInodesSize;
			readFromBlock(start_block_bitmap_inode+i, 0, freeInodes + i*superBlock.blockSize, bytes_to_read, fileSystemName);
			freeInodesSize -= bytes_to_read;
	}



	for( int i = 0; i<=freeBlocksBlocksRequired && freeBlocksSize>0; ++i ){//write bitmap of free inodes
			uint32_t bytes_to_read = freeBlocksSize > superBlock.blockSize ? superBlock.blockSize : freeBlocksSize;
			readFromBlock(start_block_bitmap_blocks+i, 0, freeBlocks + i*superBlock.blockSize, bytes_to_read, fileSystemName);
			freeBlocksSize -= bytes_to_read;
	}
}

void mountRootDirectory(const char* fileSystemName){
	rootInode = getInode(0, fileSystemName);
}

void mountBootBlock(const char* fileSystemName){//finds and sets the BLOCK_SIZE

	char numAsCharArray[4];
	readFromBlock(0, 0, numAsCharArray, 4, fileSystemName);
		
	BLOCK_SIZE = charArrayToInt(numAsCharArray);
}
