/*
 File: scheduler.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

#ifndef NULL
#define NULL 0L
#endif

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() 
{
  // assert(false);
  threadCnt = 0;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() 
{
    // assert(false);

    if(threadCnt <= 0)
    {
        return;
    }
    else
    {
        threadCnt--;
        Thread * deque_t  = readyQ.dequeue();

        Thread::dispatch_to(deque_t);
    }
}

void Scheduler::resume(Thread * _thread)
{
    // assert(false);
    readyQ.enqueue(_thread);
    threadCnt++;
}

void Scheduler::add(Thread * _thread)
{
    // assert(false);
    readyQ.enqueue(_thread);
    threadCnt++;
}

void Scheduler::terminate(Thread * _thread)
{
    // assert(false);
    
    int i = 0;    

    while(i < threadCnt)
    {
        Thread * deque_t = readyQ.dequeue();
        
        if(deque_t->ThreadId() != _thread->ThreadId())
        {
            readyQ.enqueue(deque_t);
        }
        else
        {
            return;
        }
 
        i++;
    }
}
