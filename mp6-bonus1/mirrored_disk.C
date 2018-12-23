/*
     File        : mirrored_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "mirrored_disk.H"
#include "simple_disk.H"
#include "scheduler.H"
#include "thread.H"
#include "machine.H"

extern Scheduler * SYSTEM_SCHEDULER;
/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

MirroredDisk::MirroredDisk(DISK_ID _disk_id, unsigned int _size, Scheduler * _scheduler) 
  : SimpleDisk(_disk_id, _size) 
{
    this->m_sched = _scheduler;
    size = 0;
    this->m_blockedQ_head = new Queue();
}


void MirroredDisk::wait_until_ready()
{
    this->enqueue(Thread::CurrentThread());
    SYSTEM_SCHEDULER->yield();
}

void MirroredDisk::enqueue(Thread * _thread)
{
    this->m_blockedQ_head->enqueue(_thread);
    ++size;
}

void MirroredDisk::resume_blockedQ()
{
    if(size != 0)
    {
        Thread *thrd = this->m_blockedQ_head->dequeue();
        this->m_sched->resume(thrd);
    }
}

bool MirroredDisk::is_ready()
{
    return SimpleDisk::is_ready();
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/
void MirroredDisk::issue_op1(DISK_OPERATION _op, unsigned long _block_no)
{
  // MASTER
  Machine::outportb(0x1F1, 0x00); /* send NULL to port 0x1F1         */
  Machine::outportb(0x1F2, 0x01); /* send sector count to port 0X1F2 */
  Machine::outportb(0x1F3, (unsigned char)_block_no);
                         /* send low 8 bits of block number */
  Machine::outportb(0x1F4, (unsigned char)(_block_no >> 8));
                         /* send next 8 bits of block number */
  Machine::outportb(0x1F5, (unsigned char)(_block_no >> 16));
                         /* send next 8 bits of block number */
  Machine::outportb(0x1F6, ((unsigned char)(_block_no >> 24)&0x0F) | 0xE0 | (MASTER << 4));
                         /* send drive indicator, some bits, 
                            highest 4 bits of block no */

  Machine::outportb(0x1F7, (_op == READ) ? 0x20 : 0x30);
}

void MirroredDisk::issue_op2(DISK_OPERATION _op, unsigned long _block_no) 
{
  // SLAVE
  Machine::outportb(0x1F1, 0x00); /* send NULL to port 0x1F1         */
  Machine::outportb(0x1F2, 0x01); /* send sector count to port 0X1F2 */
  Machine::outportb(0x1F3, (unsigned char)_block_no);
                         /* send low 8 bits of block number */
  Machine::outportb(0x1F4, (unsigned char)(_block_no >> 8));
                         /* send next 8 bits of block number */
  Machine::outportb(0x1F5, (unsigned char)(_block_no >> 16));
                         /* send next 8 bits of block number */
  Machine::outportb(0x1F6, ((unsigned char)(_block_no >> 24) & 0x0F) | 0xE0 | (SLAVE << 4));
                         /* send drive indicator, some bits, 
                            highest 4 bits of block no */

  Machine::outportb(0x1F7, (_op == READ) ? 0x20 : 0x30);

}

void MirroredDisk::read(unsigned long _block_no, unsigned char * _buf) 
{
  // -- REPLACE THIS!!!
  // SimpleDisk::read(_block_no, _buf);

  issue_op1(READ, _block_no);
  issue_op2(READ, _block_no);

  wait_until_ready();

  /* read data from port */
  Console::puts("Reading Operation \n");

  int i;
  unsigned short tmpw;
  unsigned int diskNo;

  for (i = 0; i < 256; i++) 
  {
    tmpw = Machine::inportw(0x1F0);    
    _buf[i*2]   = (unsigned char)tmpw;
    _buf[i*2+1] = (unsigned char)(tmpw >> 8);
  }
}


void MirroredDisk::write(unsigned long _block_no, unsigned char * _buf) 
{
  // -- REPLACE THIS!!!
  // SimpleDisk::write(_block_no, _buf);

  issue_op1(WRITE, _block_no);

  wait_until_ready();

  Console::puts("Writing Operation Master\n");

  // write data to port
  int i;
  unsigned short tmpw;
  for (i = 0; i < 256; i++) 
  {
    tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
    Machine::outportw(0x1F0, tmpw);
  }

  issue_op2(WRITE, _block_no);

  wait_until_ready();

  Console::puts("Writing Operation Slave \n");


  /* write data to port */
  for (int i = 0; i < 256; i++) {
    unsigned short tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
    Machine::outportw(0x1F0, tmpw);
  } 
}
