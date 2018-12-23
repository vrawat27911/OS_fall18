#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
    PageTable::shared_size = _shared_size;
    PageTable::kernel_mem_pool = _kernel_mem_pool;
    PageTable::process_mem_pool = _process_mem_pool;
    Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
    // get frame for page directory
    page_directory = (unsigned long *) (kernel_mem_pool->get_frames(1) * PAGE_SIZE);
    
    // frames assigned in first fit so page_dir and page table in adjacent frames
    // initialize with page table for kernel mem till 4 MB.
    unsigned long * page_table = (unsigned long *) (kernel_mem_pool->get_frames(1) * PAGE_SIZE);
    unsigned long address = 0;

    // map the first 4MB of memory
    // iterate through saving first address of frames in page table entry and setting control bits 
    for(int i = 0; i < 1024; i++)
    {
        // attribute set to: supervisor level, read/write, present(011 in binary)
        page_table[i] = address | 3;
        address = address + PAGE_SIZE; // 4096
    }

    page_directory[0] = (unsigned long) page_table;
    page_directory[0] = page_directory[0] | 3;
    
    for(int i = 1; i < 1024; i++)
    {
        // attribute set to: supervisor level, read/write, not present(010 in binary)
        page_directory[i] = 0 | 2;    
    }

    Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
   current_page_table = this;
   write_cr3((unsigned long)page_directory);
   Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
    // write_cr3, read_cr3, write_cr0, and read_cr0 all come from the assembly functions
    // put that page directory address into CR3
    // write_cr0(read_cr0() | 0x80000000); // set the paging bit in CR0 to 1
    write_cr3((unsigned long)current_page_table->page_directory); 
    write_cr0(read_cr0() | 0x80000000); // set the paging bit in CR0 to 1
    paging_enabled = 1;    

    Console::puts("Enabled paging\n");
}


void PageTable::handle_fault(REGS * _r)
{
    // CR2 stores address which caused fault
    unsigned long excpt_no = _r->err_code;
    
 
    // page not present fault
    if(((excpt_no & 1) == 0) || (excpt_no == 14))
    {
        unsigned long * page_table;
        unsigned long * page_dir = (unsigned long *)read_cr3();
       
        unsigned long fault_addr = read_cr2();  
        unsigned long dir_entry = fault_addr >> 22;
        unsigned long table_entry = (fault_addr >> 12) & 0x03FF;

        // page directory does not have valid dir_entry - create a page table for it 
        if((page_dir[dir_entry] & 1) != 1)
        {
            // create a new pde in page directory - allocating a page table for pde not pointing to any page table 
            page_dir[dir_entry] = (unsigned long) (PageTable::kernel_mem_pool->get_frames(1) * PAGE_SIZE);
            page_dir[dir_entry] |= 3;            

            // point to newly created page table - first 20 bits
            page_table = (unsigned long *) ((page_dir[dir_entry >> 12]) << 12);
           
            // user can access these frames 
            for(int i = 0; i < 1024; i++)
            {
                page_table[i] = 4;
            }
        }
    
        page_table = (unsigned long *) ((page_dir[dir_entry] >> 12) << 12);
        page_table[table_entry] = PageTable::process_mem_pool->get_frames(1) * PAGE_SIZE;
        page_table[table_entry] |= 3;
    }
    
    Console::puts("handled page fault\n");
}
