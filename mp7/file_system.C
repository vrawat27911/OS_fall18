/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

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

#include "file_system.H"
#include "assert.H"
#include "console.H"


/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() 
{
    // Console::puts("In file system constructor.\n");
    
    files = NULL;
    filesCnt = 0;
    memset(blockBuffer, 0, BLOCKSIZE);
    free_inodeBlock_num = 0;
    free_dataBlock_num = 250;
}


/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/


bool FileSystem::Mount(SimpleDisk * _disk) 
{
    Console::puts("FileSystem: Mounting file system for disk\n");

    disk = _disk;
    disk->read(0, blockBuffer);
    filesCnt = blockPtr->datablocksCnt;
    
    // read existing files in disk from block 0
    for(unsigned int i = 0; i < filesCnt; i++)
    {
        // disk->read(0, blockBuffer);
        File * exfile = new File(); 
        disk->read(blockPtr->dataBlocks[i], blockBuffer);
        exfile->file_id = blockPtr->file_id;
        exfile->file_blockCnt = blockPtr->datablocksCnt;
        add_filetoFS(exfile);
    }

    return true;
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size)
{
    Console::puts("FileSystem: Formatting disk\n");
    // assert(false);
    
    unsigned int blockIdx = 0;
    memset(blockBuffer, 0, BLOCKSIZE);

    // set entire disk to 0
    while(blockIdx < DISK_BLOCKS_CNT)
    {
        _disk->write(blockIdx, blockBuffer);
        blockIdx++;
    }

    blockPtr->status = USED;
    blockPtr->datablocksCnt = 0;
    _disk->write(0, blockBuffer);

    return true;
}

File * FileSystem::LookupFile(int _file_id) 
{
    Console::puts("FileSystem: Looking up file\n");
    // assert(false);
    unsigned int i = 0;

    while(i < filesCnt)
    {
        // Console::puti(files[i].file_id);
        // Console::puts("::"); Console::puti(_file_id); Console::puts("\n");

        if(files[i].file_id == _file_id) 
        {
            Console::puts("file found \n");

            disk->read(files[i].inode_blockNum, blockBuffer);
            files[i].file_blockCnt = blockPtr->datablocksCnt;
 
            // Console::puti(blockPtr->datablocksCnt); Console::puts(" Disk file data block\n");
            unsigned int * list = (unsigned int*) new unsigned int[blockPtr->datablocksCnt];
 
            for(int j = 0; j < blockPtr->datablocksCnt; j++)
            {
                list[j] = blockPtr->dataBlocks[j];
            }

            files[i].file_blockNumsList = list;
            return &files[i];
        }

        i++;
    }
    
    return NULL;
}

bool FileSystem::FileExists(unsigned int _file_id, File * lfile)
{
    unsigned int i = 0;
    unsigned int found = 0;
   
    while(i < filesCnt)
    {
        if(files[i].file_id == _file_id)
        {
            *lfile = files[i];
            found = 1;
            break;
        }
        
        i++;
    }

    if(found == 1)
        return true;

    return false;
}

bool FileSystem::CreateFile(int _file_id) 
{
    Console::puts("FileSystem: Creating file\n");
    // assert(false);

    File * nwfile = (File *) new File();
    
    if(FileExists(_file_id, nwfile))
    {
        return false;
    }

    nwfile->inode_blockNum = AssignBlock(0, false);
    disk->read(nwfile->inode_blockNum, blockBuffer);

    blockPtr->file_id = _file_id;
    blockPtr->status = USED;
    blockPtr->datablocksCnt = 0;

    disk->write(nwfile->inode_blockNum, blockBuffer);
    // Console::puts("FileSystem: Write Done \n");

    nwfile->file_id = _file_id;
    nwfile->file_blockCnt = 0;
    nwfile->file_blockNumsList = NULL;
    // Console::puti(newFile->file_id); Console::puts("\n");

    add_filetoFS(nwfile);

    return true;
}

bool FileSystem::DeleteFile(int _file_id)
{
    Console::puts("FileSystem: Deleting file\n");

    bool fnd = false;
    File * updated_list = (File *) new File[filesCnt];

    unsigned int i = 0;

    while(i < filesCnt)
    {
        if(files[i].file_id == _file_id)
        {
            fnd = true;
            // erase file data contents
            files[i].Rewrite();

            // free inode block
            FreeBlock(files[i].inode_blockNum);

            --filesCnt;
        }

        if(fnd == true)
        {
            updated_list[i] = files[i+1];
        }
        else
        {
            updated_list[i] = files[i];
        }

        i++;
    }

    File * del_list = files;

    if(filesCnt != 0)
        files = updated_list;
    else
        files = NULL;

    delete del_list;

    return fnd;
}


unsigned int FileSystem::AssignBlock(unsigned int pBlockNum, bool isdata)
{
    Console::puts("FileSystem: Assign Block From Disk \n");    
    int check_free = 0;

    if (pBlockNum == 0 && isdata == false)
    {
        // disk->read(1, blockBuffer);
        // Console::puti(free_inodeBlock_num);
        disk->read(free_inodeBlock_num, blockBuffer);
        
        while(blockPtr->status == USED)
        {
            // Console::puti(free_inodeBlock_num);

            if(free_inodeBlock_num >= 250)
            {
                free_inodeBlock_num = 0;
                ++check_free;

                if(check_free > 1)
                {
                    Console::puts("Error: disk is full, no free inode blocks available");
                    return 0;
                }
            }
            
            free_inodeBlock_num++;
            disk->read(free_inodeBlock_num, blockBuffer);
        }

        disk->read(free_inodeBlock_num, blockBuffer);
        blockPtr->status = USED;
        disk->write(free_inodeBlock_num, blockBuffer);
        return free_inodeBlock_num;
    }
    else if(pBlockNum == 0 && isdata == true)
    {
        // disk->read(1, blockBuffer);
        // Console::puti(free_dataBlock_num);
        disk->read(free_dataBlock_num, blockBuffer);
        
        while(blockPtr->status == USED)
        {
            // Console::puti(free_dataBlock_num);

            if(free_dataBlock_num > (DISK_BLOCKS_CNT - 1))
            {
                free_dataBlock_num = 250;
                ++check_free;

                if(check_free > 1)
                {
                    Console::puts("Error: disk is full, no free data blocks available");
                    return 0;
                }
            }
            
            free_dataBlock_num++;
            disk->read(free_dataBlock_num, blockBuffer);
        }

        disk->read(free_dataBlock_num, blockBuffer);
        blockPtr->status = USED;
        disk->write(free_dataBlock_num, blockBuffer);
        return free_dataBlock_num;
    }
    else
    {
        disk->read(pBlockNum, blockBuffer);
        blockPtr->status = USED;
        disk->write(pBlockNum, blockBuffer);
        return pBlockNum;
    }
}


void FileSystem::FreeBlock(unsigned int _block_num)
{
    // set block status to free
    disk->read(_block_num, blockBuffer);
    blockPtr->status = FREE;
    disk->write(_block_num, blockBuffer);
}

void FileSystem::add_filetoFS(File * pFile)
{
    // Console::puts("FileSystem: PushBack \n");

    if(files != NULL)
    {
        File * updated_list = (File *) new File[filesCnt + 1];
        unsigned int i = 0;       
 
        while(i < filesCnt)
        {
            updated_list[i] = files[i];
            // Console::puti(files[i].file_id); Console::puts("\n");
            i++;
        }
        
        File * del_list = files;
        updated_list[filesCnt] = *pFile;

        files = updated_list;

        filesCnt++;
        delete del_list;
    
        return;
    }
    else
    {
        files = pFile;
        filesCnt = 1;
        return;
    }
 
    return;
}
