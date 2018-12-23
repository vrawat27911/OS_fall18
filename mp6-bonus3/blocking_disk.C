/*
     File        : blocking_disk.c

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
#include "blocking_disk.H"
#include "simple_disk.H"
#include "scheduler.H"
#include "thread.H"

extern Scheduler * SYSTEM_SCHEDULER;
/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size, Scheduler * _scheduler) 
  : SimpleDisk(_disk_id, _size) 
{
    this->disk_obj = new Lock();
    this->m_sched = _scheduler;
    size = 0;
    this->m_blockedQ_head = new Queue();
}


void BlockingDisk::wait_until_ready()
{
    //if(!is_ready())
    //{
        // Thread *th = Thread::CurrentThread();
        // this->enqueue(th);
        // this->m_sched->yield();
    //}    
    this->enqueue(Thread::CurrentThread());
    SYSTEM_SCHEDULER->yield();
}

void BlockingDisk::enqueue(Thread * _thread)
{
    this->m_blockedQ_head->enqueue(_thread);
    ++size;
}

void BlockingDisk::resume_blockedQ()
{
    if(size > 0)
    {
        // this->disk_obj->lock();
        Thread *thrd = this->m_blockedQ_head->dequeue();
        size--;
        this->m_sched->resume(thrd);
            // this->disk_obj->unlock();
        }
}

bool BlockingDisk::is_ready()
{
    return SimpleDisk::is_ready();
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) 
{
  // -- REPLACE THIS!!!
  // SimpleDisk::read(_block_no, _buf);

  this->disk_obj->lock();

  issue_operation(READ, _block_no);
  this->enqueue(Thread::CurrentThread());
 
  this->disk_obj->unlock();

  SYSTEM_SCHEDULER->yield();

  //wait_until_ready();

  /* read data from port */
  Console::puts("Reading Operation \n");

  int i;
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
    tmpw = Machine::inportw(0x1F0);
    _buf[i*2]   = (unsigned char)tmpw;
    _buf[i*2+1] = (unsigned char)(tmpw >> 8);
  }
}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) 
{
  // -- REPLACE THIS!!!
  // SimpleDisk::write(_block_no, _buf);

  this->disk_obj->lock();

  issue_operation(WRITE, _block_no);
  this->enqueue(Thread::CurrentThread());

  this->disk_obj->unlock();

  SYSTEM_SCHEDULER->yield();
  // wait_until_ready();

  Console::puts("Writing Operation \n");

  /* write data to port */
  int i;
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
    tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
    Machine::outportw(0x1F0, tmpw);
  }
}
