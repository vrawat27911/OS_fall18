/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

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
#include "file_system.H"
#include "assert.H"
#include "console.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File()
{
    /* We will need some arguments for the constructor, maybe pointer to disk
     block with file management and allocation data. */

    // Console::puts("In file constructor.\n");
    file_blockNumsList = NULL;
    cur_blockIdx = 0;
    cur_blockPos = 0;
    file_blockCnt = 0;
    endpos = 0;
}

File::File(unsigned int id)
{
    // Console::puts("In file constructor.\n");
    file_blockNumsList = NULL;
    cur_blockIdx = 0;
    file_id = id;
    cur_blockPos = 0;
    file_blockCnt = 0;
    endpos = 0;
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/


int File::Read(unsigned int _n, char * _buf) 
{
    Console::puts("File: Reading from file\n");
    unsigned int charCnt;

    for(charCnt = _n; charCnt > 0; )
    {
        // Console::puti(block_nums[cur_block]); Console::puts("\n");
        // Console::puti(cur_blockPos);
        // Console::puti(*blockBuffer);

        FILE_SYSTEM->disk->read(file_blockNumsList[cur_blockIdx], (unsigned char *) blockBuffer);

        for(cur_blockPos; cur_blockPos < (BLOCKSIZE - HEADER_SIZE); ++cur_blockPos, ++_buf, charCnt--)
        {
            // Console::puti(charCnt);
            if(EoF())
            {
                // Console::puti(_n - charCnt); Console::puts("File: Read EOF reached \n");
                return (_n - charCnt);
            }
            else if(charCnt == 0)
                break;
        
            memcpy(_buf, blockBuffer + HEADER_SIZE + cur_blockPos, 1);
        }

        if(cur_blockPos == (BLOCKSIZE - HEADER_SIZE))
        {
            ++cur_blockIdx;
            cur_blockPos = 0;
        }
    }

    return (_n - charCnt);
}


void File::Write(unsigned int _n, const char * _buf) 
{
    Console::puts("File: Writing to file\n");
    //assert(false);
    
    unsigned int charCnt = _n;

    while(true)
    {
        if(NeedBlock())
            RequestBlock();

        int charToCopy;

        if(charCnt < BLOCKSIZE - HEADER_SIZE - cur_blockPos)
        {
            charToCopy = charCnt;
        }        
        else
            charToCopy = BLOCKSIZE - HEADER_SIZE - cur_blockPos;

        blockPtr->status = USED;        
        memcpy((void *) (blockBuffer + HEADER_SIZE + cur_blockPos), _buf, charToCopy);

        FILE_SYSTEM->disk->write(file_blockNumsList[cur_blockIdx], (unsigned char *) blockBuffer);

        charCnt -= charToCopy;
        cur_blockPos += charToCopy;
        
        if(cur_blockIdx == file_blockCnt - 1)
            endpos = cur_blockPos;

        if(cur_blockPos >= BLOCKSIZE - HEADER_SIZE)
        {
            cur_blockIdx++;
            cur_blockPos = 0;
        }
    
        if(charCnt <= 0)
            break;
    }

    return;
}

void File::Reset() 
{
    Console::puts("File: Reset current position in file\n");
    //assert(false);

    cur_blockPos = 0;
    cur_blockIdx = 0;
}

void File::Rewrite()
{
    Console::puts("File: Rewrite/erase content of file\n");
    // assert(false);

    for(cur_blockIdx = 0; cur_blockIdx < file_blockCnt; cur_blockIdx++)
    {
        FILE_SYSTEM->FreeBlock(file_blockNumsList[cur_blockIdx]);
    }

    cur_blockIdx = 0;
    cur_blockPos = 0;
    endpos = 0;

    // updating inode
    FILE_SYSTEM->disk->read(inode_blockNum, blockBuffer);
    blockPtr->datablocksCnt = 0;
    FILE_SYSTEM->disk->write(inode_blockNum, blockBuffer);

    // Console::puts("erase content of file...exit\n");
    file_blockNumsList = NULL;
    file_blockCnt = 0;
}


bool File::RequestBlock()
{
    unsigned int * updatedList = (unsigned int *) new unsigned int[file_blockCnt + 1];
    unsigned int block_alloc = FILE_SYSTEM->AssignBlock(0, true);
    // Console::puti(block_alloc); Console::puts("New block\n");

    // Update inode
    FILE_SYSTEM->disk->read(inode_blockNum, blockBuffer);
    blockPtr->dataBlocks[file_blockCnt] = block_alloc;
    blockPtr->datablocksCnt = file_blockCnt + 1;
    FILE_SYSTEM->disk->write(inode_blockNum, blockBuffer);

    unsigned int i = 0;
 
    while(i < file_blockCnt)
    {
        updatedList[i] = file_blockNumsList[i];
        i++;
    } 

    updatedList[file_blockCnt++] = block_alloc;

    delete file_blockNumsList;
    file_blockNumsList = updatedList;

    return true;
}   


bool File::NeedBlock() 
{
    if(file_blockNumsList == NULL)
    {
        return true;
    }

    if(cur_blockIdx == file_blockCnt) 
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool File::EoF() 
{
    // Console::puts("File: Testing end-of-file condition\n");

    if(file_blockNumsList == NULL)
    {
        return true;
    }
    if(cur_blockIdx == file_blockCnt - 1  && cur_blockPos > endpos || cur_blockIdx >= file_blockCnt)
    {
        return true;
    }
    else
    {
        return false;
    }
}
