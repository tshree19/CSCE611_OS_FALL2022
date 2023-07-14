/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
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
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id) {
    fs = _fs;
    fd = _id;

    //when open a file, we copy the content of block to cache
    // get the block number from lookup file Id
    int block_num = fs->LookupFile(_id)->block_begin;
    memset(block_cache, 0, SimpleDisk::BLOCK_SIZE);
    fs->disk->read(block_num, block_cache);

    Console::puts("Opening file ");Console::puti(fd);Console::puts("\n");
}

File::~File() {
    Console::puts("Closing file");Console::puti(fd);Console::puts("\n");
    
    int block_num = fs->LookupFile(fd)->block_begin;
    fs->disk->write(block_num, block_cache);
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */
    Console::puts("File Closed \n");
    
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf) {
    Console::puts("Reading from file\n");
    int char_count = 0;
#ifdef _FILE_IS_LARGE_
    if ((current_position + _n) > (BLOCK_SIZE - 1)) {
        file_size++;
        int block = fs->LookupFile(fd)->block_begin;
        int j = 0;
        int block_count = file_size;
        while (block_count) {
            memset(block_cache, 0, SimpleDisk::BLOCK_SIZE);
            fs->disk->read(block, block_cache);
            for (int i = 0; i < BLOCK_SIZE; i++) {
                if (EoF()){
                    break;
                 }
                _buf[j] = block_cache[i];
                j++;
                block_count++;
            }
            block_count--;
        }
        for (int i = 0; i < _n; i++) {
            if (EoF()){
                break;
            }
            _buf[j] = block_cache[current_position];
            j++
            current_position++;
            block_count++;
        }
#else

    for (int i = 0; i < _n; i++) {
        if (EoF()){
            break;
        }
        _buf[i] = block_cache[current_position];
        current_position++;
        char_count++;
    }
#endif
    Console::puts("Reading from file complete\n");
    return char_count;
}


//write to file
int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("Writing to file\n");
    int char_count = 0;
#ifdef _FILE_IS_LARGE_
    if ((current_position + _n) > (BLOCK_SIZE - 1)) {
        file_size++;
        int free_blk = fs->GetMoreFreeBlocks(file_size);
        int block = fs->LookupFile(fd)->block_begin;
        int block_count = file_size;
        while (block_count) {
            memset(block_cache, 0, SimpleDisk::BLOCK_SIZE);
            fs->disk->read(block, block_cache);
            fs->disk->write(free_blk, block_cache);
            block_count--;
        }
        current_position = 0;
        num_block = _n+current_position % BLOCK_SIZE;
        for (int i = 0; i < num_block; i++) {
            if (EoF()){
                break;
            }
            block_cache[current_position] = _buf[i];
            current_position++;
            char_count++;
        }
#else

    for (int i = 0; i < _n; i++) {
        if (EoF()){
            break;
        }
        block_cache[current_position] = _buf[i];
        current_position++;
        char_count++;
    }
#endif
    Console::puts("Writing to file complete\n");
    return char_count;
}

void File::Reset() {
    Console::puts("Resetting file\n");
    current_position = 0;
}

bool File::EoF() {
    if (current_position == SimpleDisk::BLOCK_SIZE -1){
        return true;
    }
    else
        return false;
}
