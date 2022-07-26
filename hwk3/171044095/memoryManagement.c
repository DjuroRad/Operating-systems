#include "memoryManagement.h"
#include <stdlib.h>
#include <string.h>

Inode createInode(FileType fileType){
	Inode newINode;

	newINode.attributes.fileSize = 0;//set filesize 
	newINode.attributes.timeCreation = time(NULL);//set time related attributes
	newINode.attributes.timeLastModification = newINode.attributes.timeCreation;

	newINode.type = fileType;

	for( int i=0; i < INODE_DB_N; ++i )
		newINode.diskBlocks[i] = -1;//points each block to -1 as inode is not allocated at the begining

	return newINode;
}

void addNewInode(){
	superBlock.nFullInodes ++;//root directory is stored within this
	superBlock.nEmptyInodes = superBlock.nInodes - superBlock.nFullInodes;
}

void removeInode(){
	superBlock.nFullInodes --;//root directory is stored within this
	superBlock.nEmptyInodes = superBlock.nInodes - superBlock.nFullInodes;
}


void writeInode(uint16_t i, const Inode* inode, const char* fileSystemName){
	FILE* fp = getFile("r+", fileSystemName);
		writeInodeOptimized(i, inode, fp);
	closeFile(fp);
}

void writeInodeOptimized(uint16_t i, const Inode* inode, FILE* fp){
	uint16_t block_num = getInodeBlock(i);
	uint32_t offset = getInodeBlockOffset(i);

	writeToBlockOptimized(block_num, offset, &(inode->attributes.fileSize), sizeof(uint32_t), fp);//write fileSize
	offset += sizeof(uint32_t);
	
	writeToBlockOptimized(block_num, offset, &(inode->attributes.timeCreation), sizeof(time_t), fp);//write fileSize
	offset += sizeof(time_t);

	writeToBlockOptimized(block_num, offset, &(inode->attributes.timeLastModification), sizeof(time_t), fp);//write fileSize
	offset += sizeof(time_t);

	writeToBlockOptimized(block_num, offset, &(inode->type), sizeof(FileType), fp);//write type
	offset += sizeof(FileType);
	
	writeToBlockOptimized(block_num, offset, inode->diskBlocks, INODE_DB_N*sizeof(uint16_t), fp);
}

uint16_t getInodeBlock(uint16_t inode_num){//counting starts from 0
	uint32_t usableBlockSpace = ( superBlock.blockSize / superBlock.sizeInode ) * superBlock.sizeInode;//inode size wasn't given as a factor of 2, meaning there could be some internal fragmentation
	
	uint32_t size = (inode_num)*superBlock.sizeInode;
	uint16_t inode_block_number = size/usableBlockSpace;

	return inode_block_number + 1 + 1; // + bootblock + superblock
}

uint32_t getInodeBlockOffset(uint16_t inode_num){//counting starts from 0
	uint32_t usableBlockSpace = ( superBlock.blockSize / superBlock.sizeInode ) * superBlock.sizeInode;//inode size wasn't given as a factor of 2, meaning there could be some internal fragmentation
	
	uint32_t size = (inode_num)*superBlock.sizeInode;
	uint32_t inode_offset_in_block = (uint32_t)size%usableBlockSpace;
	return inode_offset_in_block;
}

void writeToBlock(uint16_t block_num, uint32_t offset, const void* src, uint16_t num_bytes, const char* fileSystemName){
	FILE* fp = getFile("r+", fileSystemName);
	writeToBlockOptimized(block_num, offset, src, num_bytes, fp);
	closeFile(fp);
}

void writeToBlockOptimized(uint16_t block_num, uint32_t offset, const void* src, uint16_t num_bytes, FILE* fp){
	seekUtil(fp, (block_num*superBlock.blockSize) + offset, SEEK_SET);
	fwriteUtil(src, num_bytes, 1, fp);
}

void fWriteToBlockOptimized(uint16_t block_num, uint32_t offset, const void* src, uint16_t num_bytes, int nmemb, FILE* fp){
	seekUtil(fp, (block_num*superBlock.blockSize) + offset, SEEK_SET);
	fwriteUtil(src, num_bytes, nmemb, fp);
}


int getAndClaimEmptyInode(){//start from 1, first inode reserved for root directory
	for( int i = 1; i<superBlock.nInodes; ++i ){
		if( getBitmap(freeInodes,i) ){
			claimInode(i);
			return i;
		}
	}

	return -1;
}

int getAndClaimEmptyBlock(){//start from i = 2, first 2 blocks are reserved for super block and boot block

	for( int i = superBlock.firstDataBlock; i<superBlock.nBlocks; ++i ){
		if( getBitmap(freeBlocks,i) ){
			claimBlock(i);
			return i;
		}
	}

	return -1;
}

void claimBlock(uint16_t block_num){

	if( getBitmap( freeBlocks,block_num ) ){
		unsetBitmap(freeBlocks, block_num);
		superBlock.nEmptyBlocks --;
		superBlock.nFullBlocks ++;
	}else{
			// printf("Attempt to claim an already claimed block %d\n", block_num);
		}
}

void unclaimBlock(uint16_t block_num){
	if( getBitmap( freeBlocks, block_num ) == 0 ){
		setBitmap(freeBlocks, block_num);
		superBlock.nEmptyBlocks ++;
		superBlock.nFullBlocks --;
	}else{
			// printf("Attempt to unclaim an unclaimed block %d\n", block_num);
		}
}

void claimInode(uint16_t inode_num){

	if( getBitmap( freeInodes, inode_num ) ){
		unsetBitmap(freeInodes, inode_num);
		superBlock.nEmptyInodes --;
		superBlock.nFullInodes ++;
	}else{
			// printf("Attempt to claim an already claimed inode %d \n", inode_num);
		}
}

void unclaimInode(uint16_t inode_num){
	if( getBitmap( freeInodes, inode_num ) == 0 ){
		setBitmap(freeInodes, inode_num);
		superBlock.nEmptyInodes ++;
		superBlock.nFullInodes --;
	}else{
			// printf("Attempt to unclaim an unclaimed node %d\n", inode_num);
		}
}

void syncAndUnclaimInode(int inodeNumber, const Inode* inode, FILE* fp){
	unclaimInode(inodeNumber);
	writeInodeOptimized(inodeNumber, inode, fp);
}

void writeDirectoryEntry( const DirectoryEntry* directoryEntry, uint16_t inodeNumber, const char* fileSystemName ){
	
	if( directoryEntry == NULL || fileSystemName == NULL ) return;

	FILE* fp = getFile("r+", fileSystemName);
		writeDirectoryEntryOptimized(directoryEntry, inodeNumber, fp);//claims blocks if neccessary, updates the inode
	closeFile(fp);
}

void writeDirectoryEntryOptimized( const DirectoryEntry* directoryEntry, uint16_t inodeNumber, FILE* fp ){
	writeToFileOptimized(inodeNumber, directoryEntry->filename, FILENAME_LENGTH, fp);//claims blocks if neccessary, updates the inode
	writeToFileOptimized(inodeNumber, &(directoryEntry->inodeNumber), sizeof(uint16_t), fp);//claims blocks if neccessary, updates the inode
}

DirectoryEntry getDirectoryEntry( uint16_t blockNumber, int offset, const char* fileSystemName ){

	FILE* fp = getFile("r+", fileSystemName);
		DirectoryEntry dirEntry = getDirectoryEntryOptimized(blockNumber, offset, fp);//claims blocks if neccessary, updates the inode
	closeFile(fp);

	return dirEntry;
}

DirectoryEntry getDirectoryEntryOptimized( uint16_t blockNumber, uint32_t offset, FILE* fp ){
	DirectoryEntry dirEntry;
	readFromBlockOptimized(blockNumber, offset, dirEntry.filename, FILENAME_LENGTH, fp);//claims blocks if neccessary, updates the inode
	readFromBlockOptimized(blockNumber, offset + FILENAME_LENGTH, &(dirEntry.inodeNumber), sizeof(uint16_t), fp);//claims blocks if neccessary, updates the inode
	return dirEntry;
}

void syncDirectoryEntryOptimized( const Inode* parentInode, const DirectoryEntry* directoryEntry, FILE* fp ){

	const char* filename = directoryEntry->filename;

	if( strlen(filename) > FILENAME_LENGTH ) return;//maximum file length is defined
	if( parentInode -> type != DIRECTORY ) return;//has to be a directory in order to traverse it

	bool synced = FALSE;
	for( int i = 0; i<countDirectoryEntries(parentInode) && !synced; ++i ){

		DirectoryEntry currentDirectoryEntry = getDirectoryEntryFromPosition( parentInode, i, fp );

		if( fileNameCmp( currentDirectoryEntry.filename, filename ) == TRUE ){//compare read filename with target filename

			int blockIndex = getDirectoryEntryBlockIndex(i);
			int blockNumber = parentInode->diskBlocks[blockIndex];
			int offset = getDirectoryEntryBlockOffset(i);

			DirectoryEntry invalidDirectoryEntry = createDirectoryEntry(-1, "");
			writeToBlockOptimized(blockNumber, offset, invalidDirectoryEntry.filename, FILENAME_LENGTH, fp);//rewrite name
			writeToBlockOptimized(blockNumber, offset + FILENAME_LENGTH, &(invalidDirectoryEntry.inodeNumber), sizeof(uint16_t), fp);//rewrite inode number
			synced = TRUE;
		}
	}
}

Inode getInode(uint16_t inode_num, const char* fileSystemName){
	FILE* fp = getFile("r+", fileSystemName);

	int inode_block_num = getInodeBlock(inode_num);
	int inode_block_offset = getInodeBlockOffset(inode_num);


	Inode requestedInode;
	
	seekUtil(fp, (inode_block_num*superBlock.blockSize) + inode_block_offset, SEEK_SET);
		readInode(fp, &requestedInode);
	closeFile(fp);

	return requestedInode;
}

void readInode(FILE* fp, Inode* inode){

	freadUtil(&(inode->attributes.fileSize), sizeof(uint32_t), 1, fp);//read attributes
	freadUtil(&(inode->attributes.timeCreation), sizeof(time_t) , 1, fp);//read attributes
	freadUtil(&(inode->attributes.timeLastModification), sizeof(time_t), 1, fp);//read attributes

	freadUtil(&(inode->type), sizeof(FileType), 1, fp);//read type
	freadUtil(&(inode->diskBlocks), INODE_DB_N, sizeof(uint16_t), fp);//read type
}

Inode getInodeOptimized(uint16_t inode_num, FILE* fp){

	int inode_block_num = getInodeBlock(inode_num);
	int inode_block_offset = getInodeBlockOffset(inode_num);

	Inode requestedInode;
	seekUtil(fp, (inode_block_num*superBlock.blockSize) + inode_block_offset, SEEK_SET);
	readInode(fp, &requestedInode);

	return requestedInode;
}

void writeToFile(uint16_t inode_num, const void* src, size_t size, const char* fileSystemName){//for indirect addressing to work properly, using factor of 2 as size is recommended
	
	if( size == 0 || src == NULL ) return;
	if( !isInodeClaimed(inode_num) ) return;

	FILE* fp = getFile("r+", fileSystemName);
		writeToFileOptimized(inode_num, src, size, fp);
	closeFile(fp);
}

void writeToFileOptimized(uint16_t inode_num, const void* src, size_t size, FILE* fp){//for indirect addressing to work properly, using factor of 2 as size is recommended
	
	if( size == 0 || src == NULL ) return;
	if( !isInodeClaimed(inode_num) ) return;

	Inode fileInode = getInodeOptimized(inode_num, fp);
	uint32_t newFileSize = fileInode.attributes.fileSize + size;

	if( newFileSize <= superBlock.maxFileSizeDirectAddresing ){//go with direct addressing, there is enough place for it
		writeUsingDirectAddressing(&fileInode, newFileSize, src, size, fp);}
	else if( newFileSize <= superBlock.maxFileSizeIndirectAddresing	 )
		writeUsingIndirectAddressing(&fileInode, newFileSize, src, size, fp);
	else{
		printf("File size is too large | MAX: %d\n", superBlock.maxFileSizeIndirectAddresing);
		exit(1);
	}
	fileInode.attributes.fileSize = newFileSize;
	writeInodeOptimized(inode_num, &fileInode, fp);	//sync inode
}

void printInode(const Inode* inode){
	printf("file size: %u, type: %d, ", inode->attributes.fileSize, inode->type);
	for( int i = 0; i<INODE_DB_N; ++i )
		printf( "%d ", inode->diskBlocks[i] );
}

void writeUsingIndirectAddressing( Inode* inode, int newFileSize, const void* src, size_t size, FILE* fp){//use factor of 2 for size when writing this data
	
	int indirectlyAddresedBlock = -1;//find indirectlly addressed block

	if( inode->attributes.fileSize == superBlock.maxFileSizeDirectAddresing ){//set up for indirect addressing if here for the first time
		
		indirectlyAddresedBlock = getAndClaimEmptyBlock();//claim new block to be a block whose entries are block pointers containing block data 
		int lastDataBlock = inode->diskBlocks[INODE_DB_N-1];
		inode->diskBlocks[INODE_DB_N-1] = indirectlyAddresedBlock;//update last block to point to block of block pointers that contain data		
		writeToBlockOptimized( indirectlyAddresedBlock, 0, &lastDataBlock, sizeof(uint16_t), fp);//first entry of indirect block is previous last inode's block
		int16_t empty_entry_flag = -1;//set other block entires to -1 to be able to distinguish them
		int emptyEntriesCount = (superBlock.blockSize	/ sizeof(uint16_t)) -1 ;//-1 because of the added previously last block
		for(int i = 1; i<=emptyEntriesCount; ++i) writeToBlockOptimized( indirectlyAddresedBlock, sizeof(uint16_t)*i, &empty_entry_flag,  sizeof(uint16_t), fp);//all other entries empty indicated by 
	}else//already set up for indirect addressing
		indirectlyAddresedBlock = inode->diskBlocks[INODE_DB_N-1];
	
	writeToIndirectBlock(indirectlyAddresedBlock, inode, newFileSize, src, size, fp);//write to found indirectlly addressed block

}

void writeToIndirectBlock( int indirectlyAddresedBlock, const Inode* inode, int newFileSize, const void* src, size_t size, FILE* fp){//adds data to indirect block
	uint32_t blocksNeeded = (newFileSize / superBlock.blockSize) + 1; //+ 1;//get needed blocks
	int currentBlocks = countIndirectBlocks(indirectlyAddresedBlock, fp);//get blocks
	int newBlocks = blocksNeeded - currentBlocks;

	
	// printf("need:%d curr:%d\n", blocksNeeded, currentBlocks);
	
	if( newBlocks > 0 && newFileSize%superBlock.blockSize != 0 ){//fill the gap and start filling new blocks

		if( newBlocks>1 && newFileSize%superBlock.blockSize == 0 ){//meaning there is space to be filled
			
			// printf("\n\n\n%c, HERE, NEVER ?!?!?!??!?!?!: %d, %d, %d, %d\n\n\n\n", *(char*)src, newFileSize, superBlock.blockSize, blocksCurrent, blocksNeeded);	
			int bytesWritten = fillPartialFilledBlockIndirectAddressing(indirectlyAddresedBlock, inode, src, size, fp);
			src += bytesWritten;//move pointer of the source data
			size -= bytesWritten;//size left to be written
		}
		fillNewBlocksIndirectAddressing(indirectlyAddresedBlock, newBlocks, src, size, fp);//write while claiming new blocks
	}else
		fillPartialFilledBlockIndirectAddressing(indirectlyAddresedBlock, inode, src, size, fp);
}

int countIndirectBlocks(int indirectBlockNumber, FILE* fp){//assumes previous check for indirect addressing
	int maxIndirectBlocks = superBlock.blockSize/sizeof(uint16_t);

	int count = 0;
	for( int i = 1; i<maxIndirectBlocks; ++i, ++count ){//skip first block, it is full anyways
		
		int	nextBlockNumber;
		readFromBlockOptimized(indirectBlockNumber, i*sizeof(uint16_t), &nextBlockNumber, sizeof(uint16_t), fp);
		if( (int16_t)nextBlockNumber == -1 )
			break;
	}

	return count + INODE_DB_N;
}

int getLastIndirectBlockNumber(const Inode* inode, FILE* fp){//assumes previous check for indirect addressing
	
	int indirectBlock = inode->diskBlocks[INODE_DB_N-1];
	return getLastBlockNumberFromIndirectBlock(indirectBlock, fp);
}

int getLastBlockNumberFromIndirectBlock(int indirectBlock, FILE* fp){//assumes previous check for indirect addressing
	
	int maxIndirectBlocks = superBlock.blockSize/sizeof(uint16_t);
	uint16_t prevBlockNumber = -1;

	for( int i = 1; i<maxIndirectBlocks; ++i ){//skip first block, it is full anyways

		
		uint16_t nextBlockNumber;
		readFromBlockOptimized(indirectBlock, i*sizeof(uint16_t), &nextBlockNumber, sizeof(uint16_t), fp);
		if( (int16_t)nextBlockNumber == -1 )
			return prevBlockNumber;

		prevBlockNumber = nextBlockNumber;

	}

	return prevBlockNumber;
}

void writeUsingDirectAddressing( Inode* inode, int newFileSize, const void* src, size_t size, FILE* fp){
		
	uint32_t blocksNeeded = (newFileSize / superBlock.blockSize) + 1;//NOTE: if there is a partially filled block, it is always the last block.
	uint32_t blocksCurrent = countBlocks(inode);
	int newBlocks = blocksNeeded - blocksCurrent;//fill in new blocks

	if( newBlocks > 0 && newFileSize%superBlock.blockSize != 0 ){//fill the gap and start filling new blocks

		if( newBlocks>1 && newFileSize%superBlock.blockSize == 0 ){//meaning there is space to be filled
			
			// printf("\n\n\n%c, HERE, NEVER ?!?!?!??!?!?!: %d, %d, %d, %d\n\n\n\n", *(char*)src, newFileSize, superBlock.blockSize, blocksCurrent, blocksNeeded);	
			int bytesWritten = fillPartialFilledBlockDirectAddressing(inode, src, size, fp);
			src += bytesWritten;//move pointer of the source data
			size -= bytesWritten;//size left to be written
		}
		fillNewBlocks( inode, newBlocks, src, size, fp);

	}else{//fill in the gap, no need for additional blocks
		fillPartialFilledBlockDirectAddressing( inode, src, size, fp );
	}

}

int fillNewBlocks(Inode* inode, int newBlocksCoount, const void* src, size_t size, FILE* fp){	

	while( newBlocksCoount > 0 ){//fill new blocks
		int toWriteBytes = (size > superBlock.blockSize) ? superBlock.blockSize : size;				
		int emptyBlock = getAndClaimEmptyBlock();//find new block
		if( (int16_t)emptyBlock == -1 ){ printf("All blocks are full, not enough space\n"); exit(1);}

		(inode->diskBlocks)[ getFreeBlockPosition(inode) ] = emptyBlock;//update inode
		writeToBlockOptimized( emptyBlock, 0, src, toWriteBytes, fp );//set new block's data

		src += toWriteBytes;//move the source data
		size -= toWriteBytes;
		--newBlocksCoount;
	}

	return size;
}

int fillNewBlocksIndirectAddressing(int indirectBlockNumber, int newBlocksCoount, const void* src, size_t size, FILE* fp){

	int indirectBlocKEntryCount = countIndirectBlocks(indirectBlockNumber, fp) - INODE_DB_N;//returns all block count

	while( newBlocksCoount > 0 ){//fill new blocks
		int toWriteBytes = (size > superBlock.blockSize) ? superBlock.blockSize : size;				
		int emptyBlock = getAndClaimEmptyBlock();//find new block
		// printf("\n\n\nNEW DATA BLOCK %d\n\n\n", emptyBlock);
		if( (int16_t)emptyBlock == -1 ){ printf("All blocks are full, not enough space\n"); exit(1);}

		writeToBlockOptimized( indirectBlockNumber, (indirectBlocKEntryCount + 1)*sizeof(uint16_t), &emptyBlock, sizeof(uint16_t), fp );//update indirect block
		writeToBlockOptimized( emptyBlock, 0, src, toWriteBytes, fp );//set new block's data

		src += toWriteBytes;//move the source data
		size -= toWriteBytes;
		--newBlocksCoount;
	}

	return size;
}

int fillPartialFilledBlockDirectAddressing(const Inode* inode, const void* src, size_t size, FILE* fp){

	int block_number = getPartiallyFilledBlockNumber(inode);//find partially filled block
	return fillPartialFilledBlock(block_number, inode, src, size, fp);	
}

int fillPartialFilledBlockIndirectAddressing(int indirectlyAddresedBlock, const Inode* inode, const void* src, size_t size, FILE* fp){
	uint16_t lastDataBlock = getLastBlockNumberFromIndirectBlock(indirectlyAddresedBlock, fp);//either partially filled or full
	return fillPartialFilledBlock(lastDataBlock, inode, src, size, fp);
}

int fillPartialFilledBlock(int blockNumber, const Inode* inode,const void* src, size_t size, FILE* fp){
	int offset = (inode->attributes).fileSize % superBlock.blockSize;
	int availableSpace = superBlock.blockSize - offset;

	int toWrite = size > availableSpace ? availableSpace : size;
	writeToBlockOptimized( blockNumber, offset, src, toWrite, fp );
			
	return toWrite;
}

void writeNewData(int n_blocks){
	//write n blocks
}

int isInodeClaimed(uint16_t inode_num){
	return getBitmap(freeInodes, inode_num) == 0; 
}

uint32_t countBlocks(const Inode* inode){
	
	if( inode->attributes.fileSize == 0) return 0;


	return (inode->attributes.fileSize % superBlock.blockSize) == 0 ? 
				inode->attributes.fileSize / superBlock.blockSize :
				inode->attributes.fileSize / superBlock.blockSize + 1; 
}

int getFreeBlockPosition(const Inode* inode){
	for( int i = 0; i<INODE_DB_N; ++i )
		if( ((int16_t)inode->diskBlocks[i]) == -1 )
			return i;

	return -1;
}

int getPartiallyFilledBlockNumber(const Inode* inode){

	int i = 0;
	while( (int16_t)inode->diskBlocks[i] != -1 && i<INODE_DB_N )
		++i;
	
	return (inode->diskBlocks)[i-1];
}

void readFromBlock(uint16_t blockNumber, uint32_t offset, void* dest, size_t size, const char* fileSystemName){

	// if( blockNumber	< 500)
	// 		printf("bn: %d, off: %d\n", blockNumber, offset);

	FILE* fp = getFile("r", fileSystemName);
		readFromBlockOptimized(blockNumber, offset, dest, size, fp);
	closeFile(fp);
}

void readFromBlockOptimized(uint16_t blockNumber, uint32_t offset, void* dest, uint16_t size, FILE* fp){

	// printf("%d, %d, ", blockNumber, offset );
	seekUtil(fp, (blockNumber*superBlock.blockSize) + offset, SEEK_SET);
	freadUtil(dest, size, 1, fp);

	// printf(" %c\n", *(char*)dest);
}

void printSuperBlock(){
	printf("fileSystemSize: %u\n", superBlock.fileSystemSize);
	printf("blockSize: %u\n", superBlock.blockSize);
	printf("usableBlockSizeInodeTable: %u\n\n", superBlock.usableBlockSizeInodeTable);

	printf("nBlocks: %u\n", superBlock.nBlocks);
	printf("nEmptyBlocks: %u\n", superBlock.nEmptyBlocks);
	printf("nFullBlocks: %u\n\n", superBlock.nFullBlocks);


	printf("nInodes: %u\n", superBlock.nInodes);
	printf("sizeInode: %u\n", superBlock.sizeInode);
	printf("nEmptyInodes: %u\n", superBlock.nEmptyInodes);
	printf("nFullInodes: %u\n\n", superBlock.nFullInodes);

	printf("blockCountInodeTable: %u\n", superBlock.blockCountInodeTable);
	printf("blockCountInodeBitmap: %u\n", superBlock.blockCountInodeBitmap);
	printf("blockCountBlockBitmap: %u\n\n", superBlock.blockCountBlockBitmap);

	printf("firstDataBlock: %u\n", superBlock.firstDataBlock);
	printf("maxFileSizeDirectAddresing: %u\n", superBlock.maxFileSizeDirectAddresing);
	printf("maxFileSizeIndirectAddresing: %u\n", superBlock.maxFileSizeIndirectAddresing);
	
}

bool isIndirectlyAddreessed(const Inode* inode){

	return ( (inode->attributes.fileSize > superBlock.maxFileSizeDirectAddresing) && (inode->type == REGULAR_FILE) )? TRUE : FALSE;
}

int getBlockNumberFromIndex(const Inode* inode, int i, FILE* fp){

	if(  i < countBlocks(inode) && i>= 0  ){
		if( isIndirectlyAddreessed(inode) ){
			return (i < INODE_DB_N - 1 ) ? 
						inode->diskBlocks[i] : 
						getBlockNumberFromIndirectBlock(inode, i, fp);
		}
		else//direct addressing, returns what's in block or -1 if out of boundaries
			return inode->diskBlocks[i];
	}else
		return -1;
	
}

int getBlockNumberFromIndirectBlock(const Inode* inode, int blockNumber, FILE* fp){//assumes previous check for boundaries

	if( countBlocks(inode) < INODE_DB_N ) return -1;//no indirect block
	if(  blockNumber < INODE_DB_N - 1 ) return -1;//no indirect addressing


	int indirectBlockNumber = inode->diskBlocks[INODE_DB_N - 1];
	int blockNumberOffset = (blockNumber - (INODE_DB_N - 1));
	int offset = blockNumberOffset*sizeof(uint16_t);

	uint16_t block_num;
	readFromBlockOptimized( indirectBlockNumber, offset, &block_num, sizeof(uint16_t), fp );

	return block_num;
}

uint32_t countDirectoryEntries(const Inode* inode){
	return (inode->type == DIRECTORY) ? ( inode->attributes.fileSize / sizeof(DirectoryEntry) ) : 0;
}

int getDirectoryEntryInodeNumber(const Inode* parentInode, const char* targetFileName, FILE* fp){//no indirect addressing for directories

	if( strlen(targetFileName) > FILENAME_LENGTH ) return -1;//maximum file length is defined
	if( parentInode -> type != DIRECTORY ) return -1;//has to be a directory in order to traverse it

	DirectoryEntry currentDirectoryEntry;

	for( int i = 0; i<countDirectoryEntries(parentInode); ++i ){
		
		int blockIndex = getDirectoryEntryBlockIndex(i);//get block's index
		int block = parentInode->diskBlocks[blockIndex];//get block number
		int offset = getDirectoryEntryBlockOffset(i);//get offset

		currentDirectoryEntry = getDirectoryEntryOptimized( block, offset, fp );

		if( fileNameCmp( currentDirectoryEntry.filename, targetFileName ) == TRUE )//compare read filename with target filename
			return currentDirectoryEntry.inodeNumber;
	
	}

	return -1;
}

DirectoryEntry getDirectoryEntryFromPosition(const Inode* parentInode, int i, FILE* fp){
	DirectoryEntry currentDirectoryEntry;
	
	int blockIndex = getDirectoryEntryBlockIndex(i);//get block's index
	int block = parentInode->diskBlocks[blockIndex];//get block number
	int offset = getDirectoryEntryBlockOffset(i);//get offset

	currentDirectoryEntry = getDirectoryEntryOptimized( block, offset, fp );
	return currentDirectoryEntry;
}

DirectoryEntry getDirectoryEntryFromFilename(const Inode* parentInode, const char* filename, FILE* fp){
	
	DirectoryEntry currentDirectoryEntry;
	currentDirectoryEntry.inodeNumber = -1;

	if( strlen(filename) > FILENAME_LENGTH ) return currentDirectoryEntry;//maximum file length is defined
	if( parentInode -> type != DIRECTORY ) return currentDirectoryEntry;//has to be a directory in order to traverse it

	for( int i = 0; i<countDirectoryEntries(parentInode); ++i ){

		currentDirectoryEntry = getDirectoryEntryFromPosition( parentInode, i, fp );

		if( fileNameCmp( currentDirectoryEntry.filename, filename ) == TRUE )//compare read filename with target filename
			return currentDirectoryEntry;
	}

	return currentDirectoryEntry;
}

Inode getDirectoryEntryInode(int i, const Inode* inode, FILE* fp){

	if( inode -> type != DIRECTORY ) {
		printf("attempt to get directory entry from non directory inode");
		exit(1);
	}

	Inode dirEntryInode;

	int blockIndex = getDirectoryEntryBlockIndex(i);//get block's index
	int block = inode->diskBlocks[blockIndex];//get block number
	int offset = getDirectoryEntryBlockOffset(i);//get offset

	DirectoryEntry currentDirectoryEntry = getDirectoryEntryOptimized( block, offset, fp );
	dirEntryInode = getInodeOptimized(currentDirectoryEntry.inodeNumber, fp);

	return dirEntryInode;
}

int getDirectoryEntryBlockIndex(int directoryEntryIndex){
	int size = directoryEntryIndex * sizeof(DirectoryEntry);
	int block = size / superBlock.blockSize;//factor of 2, whole will be populated

	return block;
}

int getDirectoryEntryBlockOffset(int dirEntryIndex){
	int size = dirEntryIndex * sizeof(DirectoryEntry);
	int offset = size % superBlock.blockSize;

	return offset;
}

bool fileNameCmp(const char* fn1, const char* fn2){//no tracing \0

	int fn1_len = fileNameLength(fn1);
	int fn2_len = fileNameLength(fn2);

	if( fn1_len != fn2_len ) return FALSE;//check same length

	for( int i = 0; i<fn1_len; ++i )//check all same
		if( fn1[i] != fn2[i] )
			return FALSE;

	return TRUE;
}

int fileNameLength(const char* fn){//no tracking \0
	int length = 0;
	for( int i = 0; fn[i]  && i<FILENAME_LENGTH; ++i )
		length++;

	return length;
}

int getInodeByPath(char* path, FILE* fp){//returns inode number of directory, -1 otherwise

	char delim = '\\';
	
	const char* targetFileName = strtok(path, &delim);
	
	Inode parentInode = rootInode;

	uint16_t parentInodeNumber, targetInodeNumber;
	// uint16_t targetInodeNumber = -1;
	targetInodeNumber = parentInodeNumber = ROOT_INODE;//start from root inode which is 0
	
	while( parentInodeNumber != -1 && targetFileName != NULL ){//go through each filename

		parentInode = getInodeOptimized(parentInodeNumber, fp);
		int newParentInodeNumber = -1;

		if( parentInode.type == DIRECTORY )
			newParentInodeNumber = getDirectoryEntryInodeNumber(&parentInode, targetFileName, fp);
		else
			return -1;

		parentInodeNumber = newParentInodeNumber;
		targetInodeNumber = newParentInodeNumber;

		targetFileName = strtok(NULL, &delim);
	}

	return targetInodeNumber;
}

void printDirectoryContent(uint16_t inodeNumber, const char* fileSystemName){
	FILE* fp = getFile("r", fileSystemName);
		printDirectoryContentOptimized(inodeNumber, fp);
	closeFile(fp);
}

void printDirectoryContentOptimized(uint16_t inodeNumber, FILE* fp){

	Inode inode = getInodeOptimized(inodeNumber, fp);

	for( int i = 0; i<countDirectoryEntries(&inode); ++i ){

	
		int blockIndex = getDirectoryEntryBlockIndex(i);//get block's index
		int block = inode.diskBlocks[blockIndex];
		int offset = getDirectoryEntryBlockOffset(i);//get offset

		DirectoryEntry currentDirectoryEntry;
	
		readFromBlockOptimized( block, offset, &currentDirectoryEntry, sizeof(DirectoryEntry), fp);//read directory entry at offset
		printFileName(currentDirectoryEntry.filename);

		if( (int16_t)currentDirectoryEntry.inodeNumber != -1 ){
			printf("\n");
		}
	}

}

void printFileName(const char* filename){
	for( int i = 0; filename[i] && i<FILENAME_LENGTH; ++i ){
		printf("%c", filename[i]);
	}
}


bool containsFile(uint16_t inodeNumber, const char* filename, FILE* fp){
	Inode inode = getInodeOptimized(inodeNumber, fp);
	
	for( int i = 0; i<countDirectoryEntries(&inode); ++i ){

		int blockIndex = getDirectoryEntryBlockIndex(i);//get block's index
		int block = inode.diskBlocks[blockIndex];
		int offset = getDirectoryEntryBlockOffset(i);//get offset

		DirectoryEntry currentDirectoryEntry;
	
		readFromBlockOptimized( block, offset, &currentDirectoryEntry, sizeof(DirectoryEntry), fp);//read directory entry at offset

		if( fileNameCmp(currentDirectoryEntry.filename, filename) == TRUE )
			return TRUE;
	}

	return FALSE;
}


int addNewDirectoryEntryOptimized(uint16_t parentInodeNumber, const char* fileName, FileType fileType, FILE* fp){

	int newInodeNumber=-1;

	if(fileType==DIRECTORY)
		newInodeNumber = writeInitNewDirectory( parentInodeNumber, fp);
	else
		newInodeNumber = writeInitNewRegularFile( parentInodeNumber, fp );

	DirectoryEntry newDirectoryEntry = createDirectoryEntry(newInodeNumber, fileName);
	writeDirectoryEntryOptimized( &newDirectoryEntry, parentInodeNumber, fp);
	return newInodeNumber;
}

DirectoryEntry createDirectoryEntry(uint16_t inodeNumber, const char* filename){
	DirectoryEntry directoryEntry;
	directoryEntry.inodeNumber = inodeNumber;
	for( int i = 0; i < FILENAME_LENGTH && filename[i] ; ++i )
		directoryEntry.filename[i] = filename[i];

	fileNameCopy(directoryEntry.filename, filename);
	return directoryEntry;
}

int writeInitNewDirectory(uint16_t parentInode, FILE* fp){//returns new inode number

	int inodeNum = getAndClaimEmptyInode();//creating a new file
	if( inodeNum == -1 ) return -1;

	Inode newInode = createInode(DIRECTORY);
 	writeInodeOptimized(inodeNum, &newInode, fp);//update inode table

	DirectoryEntry directoryEntry1 = createDirectoryEntry(inodeNum, ".");
	DirectoryEntry directoryEntry2 = createDirectoryEntry(parentInode, "..");

	writeDirectoryEntryOptimized( &directoryEntry1, inodeNum, fp);
	writeDirectoryEntryOptimized( &directoryEntry2, inodeNum, fp);

	return inodeNum;
}

int writeInitNewRegularFile(uint16_t parentInode, FILE* fp){//returns new inode number

	int inodeNum = getAndClaimEmptyInode();//creating a new file
	if( inodeNum == -1 ) return -1;

	Inode newInode = createInode(REGULAR_FILE);
 	writeInodeOptimized(inodeNum, &newInode, fp);//update inode table

	return inodeNum;
}

void addNewDirectoryEntry(uint16_t inodeNumber, const char* fileName, FileType fileType, const char* fileSystemName){
	FILE* fp = getFile("r+", fileSystemName);
		addNewDirectoryEntryOptimized(inodeNumber, fileName, fileType, fp);
	closeFile(fp);
}

void fileNameCopy(char* dest, const char* src){
	if( fileNameLength( src ) > FILENAME_LENGTH ) printf("Source will shouldn't be longer than %d characters", FILENAME_LENGTH);
	
	int i = 0;
	for( i = 0; src[i] && i<FILENAME_LENGTH ; ++i )
		dest[i] = src[i];

	if( i < FILENAME_LENGTH ) 
		dest[i] = '\0';
}

void unclaimFile(int inodeNumber, FILE* fp){

	Inode inode = getInodeOptimized(inodeNumber, fp);
	for( int i = 0; (int16_t)inode.diskBlocks[i] != -1 && i < INODE_DB_N; ++i ){
		unclaimBlock(inode.diskBlocks[i]);
		inode.diskBlocks[i] = -1;
	}
	syncAndUnclaimInode(inodeNumber, &inode, fp);
}