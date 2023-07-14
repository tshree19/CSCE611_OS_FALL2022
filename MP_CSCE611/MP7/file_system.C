/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/
    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store
   inodes from and to disk. */
/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");
}

FileSystem::~FileSystem() {
    Console::puts("Unmounting file system\n");
    /* Make sure that the inode list and the free list are saved. */
}


/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/


bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("Unmounting file system from disk\n");
    disk = _disk;

    /* Here you read the inode list and the free list into memory */
    return true;

}


bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) { // static!
    Console::puts("Formatting disk\n");
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */
    
    //
    unsigned char blk_buffer[SimpleDisk::BLOCK_SIZE];
    int total_blocks = _size/BLOCK_SIZE;
    Inode * inode = (Inode *) blk_buffer;

    
    //clearing the disk first
    memset(blk_buffer, 0, BLOCK_SIZE);
    for (int i = 0; i < total_blocks; i++) {
        _disk->write(i, blk_buffer);
    }

    //In the disk , 1st block is assigned to inode
    //Initializing the inode list. 
    memset(blk_buffer, 0, BLOCK_SIZE);
    _disk->read(0, blk_buffer);         //block 0 is assigned to the inodelist
    
    for (int i = 0; i < MAX_INODES; i++) {
        inode[i].id = -1;                   //setting it invalid
        inode[i].file_size = 0;
        inode[i].block_begin = -1;
    }
    _disk->write(0, blk_buffer);
    _disk->read(0, blk_buffer);

    
    //In the disk , 2nd block(block 1) is assigned to freeblock list
    //Initializing the freeblock  list. 
    memset(blk_buffer, 0, BLOCK_SIZE);
    _disk->read(1, blk_buffer);
    for (int i = 0; i < BLOCK_SIZE; i++) {
        blk_buffer[i] = 0;
    }

    //setting blocks (inodelist)0 and (freeblocklist)1 to 'USED'. 
    //Here USED = 1 and FREE = 0
    blk_buffer[0] = 1;
    blk_buffer[1] = 1;
    _disk->write(1, blk_buffer);

    Console::puts("Formatting disk complete\n");
    return true;

}
//get free block from freeblock list
int FileSystem::GetFreeBlock() {
    unsigned char blk_buffer[SimpleDisk::BLOCK_SIZE];
    
    memset(blk_buffer, 0, BLOCK_SIZE);
    disk->read(1, blk_buffer);          //read freeblock list
    
    for (int i = 0; i < BLOCK_SIZE; i++) {  //if the block is FREE(0), set it to USED(1) 
        if (blk_buffer[i] == 0) {
            blk_buffer[i] = 1;
            disk->write(1, blk_buffer);
            return i;
        }
    }
    disk->write(1, blk_buffer);
    return -1;                          //return that the block is used, so invalid
}
#ifdef _FILE_IS_LARGE_ 
int FileSystem::GetMoreFreeBlocks(int no_of_blocks) {
    unsigned char blk_buffer[SimpleDisk::BLOCK_SIZE];
    memset(blk_buffer, 0, BLOCK_SIZE);
    _disk->read(1, blk_buffer);
    int blk_count = 0;
    int i;
    for ( i = 0; i < BLOCK_SIZE; i++) {
        if (blk_buffer[i] == 0) {
            blk_count++;
            if (blk_count == no_of_blocks){
                break;
            }

        } 
        else {
            blk_count = 0;
        }
    }

    if (blk_count) { 
        for (int j = 0; j<blk_count; j++)
            blk_buffer[j] = 0;
            i--;
    }
    disk->write(1, blk_buffer);
    return i;
}

#endif

Inode * FileSystem::LookupFile(int _file_id) {
    
    /* Here you go through the inode list to find the file. */
    Console::puts("Looking up file with id:"); Console::puti(_file_id); Console::puts("\n");
    unsigned char blk_buffer[SimpleDisk::BLOCK_SIZE];
    inodes = (Inode *) blk_buffer;

    memset(blk_buffer, 0, BLOCK_SIZE);
    disk->read(0, blk_buffer);       //reading inodelist

    for (int i = 0; i < MAX_INODES; i++) {
        if (_file_id == inodes[i].id) {
            Console::puts("File found "); Console::puti(_file_id); Console::puts("\n");
            return &(inodes[i]);
        }

    }
    return nullptr;
}


//create a new file
bool FileSystem::CreateFile(int _file_id) {
    Console::puts("Creating file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */

    unsigned char blk_buffer[SimpleDisk::BLOCK_SIZE];
    inodes = (Inode *) blk_buffer;
    
    memset(blk_buffer, 0, BLOCK_SIZE);
    disk->read(0, blk_buffer);
    
    for (int i = 0; i < MAX_INODES; i++) {      //assign block of the file into inode
        if (inodes[i].id == _file_id) {
            Console::puts("File already exists\n");
            return false;
        }
        else if (inodes[i].id == -1) {          //if no file in that inode
            inodes[i].id = _file_id;            //set the file name to that inode
            inodes[i].file_size = BLOCK_SIZE;
            inodes[i].block_begin = GetFreeBlock();     //block number would be the first free block from freeblock list
            break;
        }
    }
    disk->write(0, blk_buffer);
    disk->read(0, blk_buffer);
    Console::puts("Creating file complete"); Console::puts("\n");

    return true;
}


//For a given block number , free that block
bool FileSystem::BlockRelease(int block_num) {
    unsigned char blk_buffer[SimpleDisk::BLOCK_SIZE];
   
    memset(blk_buffer, 0, BLOCK_SIZE);
    disk->read(1, blk_buffer);              //read freeblock list
    if (blk_buffer[block_num] == 0) {
        Console::puts("Block is free already!"); Console::puti(block_num); Console::puts("\n");
        return false;
    }
    blk_buffer[block_num] = 0;
    disk->write(1, blk_buffer);
    return true;
}


bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("Deleting file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* First, check if the file exists. If not, throw an error.
       Then free all blocks that belong to the file and delete/invalidate
       (depending on your implementation of the inode list) the inode. */

    unsigned char blk_buffer[SimpleDisk::BLOCK_SIZE];
    inodes = (Inode *) blk_buffer;
    
    memset(blk_buffer, 0, BLOCK_SIZE);
    disk->read(0, blk_buffer);
    
    for (int i = 0; i < MAX_INODES; i++) {
        if (inodes[i].id == _file_id) {
            Console::puts("File ");Console::puti(_file_id);Console::puts(" found.. deleting it...\n");
            inodes[i].id = -1;          //delete the file setting it to invalid
            BlockRelease(inodes[i].block_begin);   //call function BlockRelease() passing the block number to be deleted
            inodes[i].file_size = 0;
            inodes[i].block_begin = -1;
            break;
        }
    }
    disk->write(0, blk_buffer);
    Console::puts("Deleting file complete"); Console::puts("\n");
    return true;
}
