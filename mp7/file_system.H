/* 
    File: file_system.H

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 10/04/05

    Description: Simple File System.
    

*/

#ifndef _FILE_SYSTEM_H_                   // include file only once
#define _FILE_SYSTEM_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define BLOCKSIZE 512
// 10MB/Blocksize
#define DISK_BLOCKS_CNT 10485760/BLOCKSIZE
#define FREE    0x0000
#define USED    0xFFFF
#define HEADER_SIZE 12

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "file.H"
#include "simple_disk.H"
#include "utils.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

struct DISKBLOCK
{
    // status - used or free 
    unsigned int status;
    unsigned int file_id;
    unsigned int datablocksCnt;
    unsigned int dataBlocks[125];
};

static unsigned char blockBuffer[BLOCKSIZE];
static DISKBLOCK * blockPtr = (DISKBLOCK *) blockBuffer;

/*--------------------------------------------------------------------------*/
/* FORWARD DECLARATIONS */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* F i l e S y s t e m  */
/*--------------------------------------------------------------------------*/

class File;

class FileSystem 
{

friend class File; /* -- not sure if we need this; feel free to delete */

private:
     /* -- DEFINE YOUR FILE SYSTEM DATA STRUCTURES HERE. */
     
     SimpleDisk * disk;
     // unsigned int size;
     unsigned int filesCnt;
     File * files;

     // inodes are stored in first 250 blocks, and then data blocks are stored.
     // following variables point to next free inode block and next free data block in disk respectively
     unsigned int free_inodeBlock_num;
     unsigned int free_dataBlock_num;
     
public:
    FileSystem();
    /* Just initializes local data structures. Does not connect to disk yet. */
    
    bool Mount(SimpleDisk * _disk);
    /* Associates this file system with a disk. Limit to at most one file system per disk.
     Returns true if operation successful (i.e. there is indeed a file system on the disk.) */
    
    static bool Format(SimpleDisk * _disk, unsigned int _size);
    /* Wipes any file system from the disk and installs an empty file system of given size. */
    
    File * LookupFile(int _file_id);
    /* Find file with given id in file system. If found, return the initialized
     file object. Otherwise, return null. */
   
    bool FileExists(unsigned int _file_id, File * _file);
    void add_filetoFS(File * file);
    
    unsigned int AssignBlock(unsigned int pBlockNum, bool isdata);

    void FreeBlock(unsigned int);

    bool CreateFile(int _file_id);
    /* Create file with given id in the file system. If file exists already,
     abort and return false. Otherwise, return true. */
    
    bool DeleteFile(int _file_id);
    /* Delete file with given id in the file system; free any disk block occupied by the file. */
};
#endif
