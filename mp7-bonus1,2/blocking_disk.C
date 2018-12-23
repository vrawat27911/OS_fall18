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


Scheduler * SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size, Scheduler * _scheduler) 
  : SimpleDisk(_disk_id, _size) 
{
    this->m_sched = _scheduler;
    size = 0;
    this->m_blockedQ_head = new Queue();
}


void BlockingDisk::wait_until_ready()
{
    
    this->enqueue(Thread::CurrentThread());
    // Console::puti(size); Console::puts("Queue Size\n"); 
    // Console::puti(is_ready()); Console::puts("Queue Ready blocking\n");

    //while(!is_ready())
    //{
        // SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
        // this->enqueue(Thread::CurrentThread());
    SYSTEM_SCHEDULER->yield();
    //}
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
        Thread *thrd = this->m_blockedQ_head->dequeue();
        size--;
        this->m_sched->resume(thrd);
    }
}

bool BlockingDisk::is_ready()
{
    return SimpleDisk::is_ready();
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
  SimpleDisk::read(_block_no, _buf);

}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
  SimpleDisk::write(_block_no, _buf);
}
