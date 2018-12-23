/*
 File: vm_pool.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"

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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) 
{
    base_address = _base_address;
    frame_pool = _frame_pool;
    page_table = _page_table;
    size = _size;

    // limiting no of regions by number allocatable in one page.
    regions_limit = Machine::PAGE_SIZE/sizeof(region_entry);

    // allocate frame for regions table
    regions_table = (region_entry*)(Machine::PAGE_SIZE * (frame_pool->get_frames(1)));
    regions_cnt = 0;

    // register pool with page table object.
    page_table->register_pool(this);
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) 
{
    if(_size == 0)
        return 0;

    unsigned long reg_size = _size;
    unsigned long reg_start_addr = 0;
    unsigned int current_index = regions_cnt;
    unsigned int prev_index = current_index - 1;   
 
    if (current_index == 0)
        reg_start_addr = this->base_address;
    else if (current_index >= regions_limit)
        // if number of regions exceed limit
        Console::puts("Max number of regions allocated");
    else
        reg_start_addr = regions_table[prev_index].start_address + regions_table[prev_index].size;

    regions_table[current_index].start_address = reg_start_addr;
    regions_table[current_index].size = reg_size;

    // increase count for regions
    ++regions_cnt;

    return reg_start_addr;

    Console::puts("Allocated region of memory.\n");
}

void VMPool::release(unsigned long _start_address) 
{
    unsigned int index = 0;
    int i = 0;
 
    // ptr to current regions table
    region_entry * prev_table = regions_table;
    unsigned long frame = (unsigned long) prev_table/Machine::PAGE_SIZE;
    unsigned long temp_addr = _start_address;

    // loop to find the region corresponding to start address 
    while(i < regions_cnt)
    {
        if(prev_table[i].start_address != _start_address)
        {
            i++;
        }
        else
        {
            index = i;
            break;
        }
    }


    i = 0;
    while(i < prev_table[index].size/Machine::PAGE_SIZE)
    {
        // release all the pages corresponding to current region
        page_table->free_page(temp_addr);
        temp_addr = temp_addr + PageTable::PAGE_SIZE;
        i++;
    }
    
    i = 0;
    // allocate a page for storing new table.
    regions_table = (region_entry*)((frame_pool->get_frames(1)) * Machine::PAGE_SIZE);
    int cnt = 0;

    // update the table    
    while(i < regions_cnt)
    {
        if(i != index)
        {
            regions_table[cnt] = prev_table[i];
            ++cnt;
            i++;
        }
        else
            i++;
    }

    regions_cnt--;

    // release frame for prev regions table
    frame_pool->release_frames(frame);

    // reload the CR3 register - flushes TLB
    page_table->load();

    Console::puts("Released region of memory.\n");
}


bool VMPool::is_legitimate(unsigned long _address)
{
    int i = 0;

    while(i < this->regions_cnt)
    {
        unsigned long start_addr = this->regions_table[i].start_address;
        unsigned long boundary = start_addr + this->regions_table[i].size;

        // if_address is mapped to any virtual pool
        if(_address >= start_addr && _address <= boundary)
        {   
            return 1;
        }

        i++;
    }

    return 0;

    Console::puts("Checked whether address is part of an allocated region.\n");
}

