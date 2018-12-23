/*
     File        : mirrored_disk.H

     Author      : 

     Date        : 
     Description : 

*/

#ifndef _MIRRORED_DISK_H_
#define _MIRRORED_DISK_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/
#include "thread.H"
#include "scheduler.H"
#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* B l o c k i n g D i s k  */
/*--------------------------------------------------------------------------*/
class Scheduler;
class Queue;

class MirroredDisk: public SimpleDisk {
public:
   MirroredDisk(DISK_ID _disk_id, unsigned int _size, Scheduler *scheduler); 
   /* Creates a BlockingDisk device with the given size connected to the 
      MASTER or SLAVE slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness. 
      In a real system, we would infer this information from the 
      disk controller. */

   /* DISK OPERATIONS */

   virtual void read(unsigned long _block_no, unsigned char * _buf);
   /* Reads 512 Bytes from the given block of the disk and copies them 
      to the given buffer. No error check! */

   virtual void write(unsigned long _block_no, unsigned char * _buf);
   /* Writes 512 Bytes from the buffer to the given block on the disk. */

   int size;

   void issue_op1(DISK_OPERATION _op, unsigned long _block_no);
   void issue_op2(DISK_OPERATION _op, unsigned long _block_no);
   void enqueue(Thread * thread);
   void resume_blockedQ();
   bool is_ready();
   void wait_until_ready();
   Scheduler * m_sched;
   Queue * m_blockedQ_head;
   SimpleDisk * master;
   SimpleDisk * slave;
};

#endif
