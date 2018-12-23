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
    page_directory = (unsigned long *) (process_mem_pool->get_frames(1) * PAGE_SIZE);
    
    // frames assigned in first fit so page_dir and page table in adjacent frames
    // initialize with page table for kernel mem till 4 MB.
    unsigned long * page_table = (unsigned long *) (process_mem_pool->get_frames(1) * PAGE_SIZE);
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

    page_directory[1023] = (unsigned long) page_directory | 3;
    reg_vmpools_cnt = 0;
    
    for(int i = 0; i < MAX_VM; i++)
    {
        reg_vmpools[i] = NULL;    
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
    // Console::puti(_r->int_no); Console::puts("\n");
 
    // page not present fault
    if((excpt_no & 1) == 0) // || _r->int_no == 14)
    {
        
        VMPool ** head = current_page_table->reg_vmpools;
        unsigned long fault_addr = read_cr2(); 
        int index = -1;

        // checkup if fault address corresponds to any virtual memory pool
        for(int i = 0; i < current_page_table->reg_vmpools_cnt; i++)
        {
            if(head[i] != NULL && head[i]->is_legitimate(fault_addr))
            {
                index = i;
                break;
            }
        }

        unsigned long * page_table;
        // unsigned long * page_dir = (unsigned long *)read_cr3();
        // unsigned long fault_addr = read_cr2(); 
        unsigned long * dir_entry = (unsigned long *) (((fault_addr >> 22) << 2) | ((0xFFFFF) << 12));
        unsigned long * table_entry = (unsigned long *) (((fault_addr >> 12) << 2) | ((0x03FF) << 22)); 
 
        // unsigned long dir_entry = fault_addr >> 22;
        // unsigned long table_entry = (fault_addr >> 12) & 0x03FF;

        if(head[index] != NULL)
        {
            // page directory does not have valid dir_entry - create a page table for it 
            if((*dir_entry & 1) != 1)
            {
                *dir_entry = (unsigned long) PageTable::process_mem_pool->get_frames(1);
                *dir_entry = ((*dir_entry) << 12) | 3;

                fault_addr = (fault_addr >> 22) << 22;
                
                for(int i = 0; i < 1024; i++)
                {
                    unsigned long * entry = (unsigned long *) (((fault_addr >> 10)) | ((0x03FF) << 22));
                    *entry = 4;
                    fault_addr = ((fault_addr >> 12) + 1) << 12;
                }

                // create a new pde in page directory - allocating a page table for pde not pointing to any page table 
                // page_dir[dir_entry] = (unsigned long) (PageTable::kernel_mem_pool->get_frames(1) * PAGE_SIZE);
                // page_dir[dir_entry] |= 3;            

                // point to newly created page table - first 20 bits
                //page_table = (unsigned long *) ((page_dir[dir_entry >> 12]) << 12);
           
                // user can access these frames 
                // for(int i = 0; i < 1024; i++)
                // {
                   //  page_table[i] = 4;
                // }
            }
            
            *table_entry = PageTable::process_mem_pool->get_frames(1);
            *table_entry = ((*table_entry) << 12) | 3;
    
            // page_table = (unsigned long *) ((page_dir[dir_entry] >> 12) << 12);
       
            // if((page_table[table_entry] & 1) == 0)
            // {
            // unsigned long frame = PageTable::process_mem_pool->get_frames(1);
            // page_table[table_entry] = frame * PAGE_SIZE;
            // page_table[table_entry] |= 3;
            // }

            // Console::puti(frame); Console::puts("\n");
            // Console::puti(dir_entry); Console::puts("\n");
            // Console::puti(table_entry); Console::puts("\n"); 
        }
    }
    
    Console::puts("handled page fault\n");
}


void PageTable::register_pool(VMPool *pool)
{
  // Console::puti(this->reg_vmpools_cnt); Console::puts("\n");  

  if((this->reg_vmpools_cnt < MAX_VM) && (this->reg_vmpools[this->reg_vmpools_cnt] == NULL))
  {
    this->reg_vmpools[this->reg_vmpools_cnt] = pool;
    this->reg_vmpools_cnt++;
    Console::puts("Registered VM pool. \n");
  }
  else
     Console::puts("Max pools already registered. \n");

  // Console::puts("Registered VM pool. \n");
}

void PageTable::free_page(unsigned long _page_no) 
{
    // fetch frame no by looking up frame no for corresponding page no.
    unsigned long * table_entry =  (unsigned long *) (((_page_no >> 12) << 2 ) | ((0x03FF)<<22));
    unsigned long frameNo = ((*table_entry) >> 12);

    // mark table entry as invalid
    *table_entry = 0;
    process_mem_pool->release_frames(frameNo);
    Console::puts("Page is freed. \n");
}
