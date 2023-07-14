#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"


#define PAGE_PRESENT        1		
#define PAGE_WRITE          2
#define PAGE_LEVEL          4

#define PD_SHIFT            22
#define PT_SHIFT            12

#define PDE_MASK            0xFFFFF000
#define PTE_MASK             0x3FF


PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   //assert(false);
   //Initializing the variables
   PageTable::kernel_mem_pool = _kernel_mem_pool;
   PageTable::process_mem_pool = _process_mem_pool;
   PageTable::shared_size = _shared_size;

   Console::puts("Initialized Paging System\n");
}

/*
 * bit 0 - valid bit i.e.page present(set as 1)/page not present(set as 0)
 * bit 1 - read only(set as 0)/ read and write(set as 1)
 * bit 2 - kernel/ user mode
 * */
PageTable::PageTable()
{
   //assert(false);
   // one frame for page directory
   page_directory = (unsigned long *)(kernel_mem_pool->get_frames(1)*PageTable::PAGE_SIZE);

   //one frame for page table
   unsigned long page_table_address = PageTable::kernel_mem_pool->get_frames(1)* PAGE_SIZE;
   unsigned long * map_page_table = (unsigned long *) page_table_address;

   //number of frames shared
   unsigned long shared_frames_num = ( PageTable::shared_size / PAGE_SIZE);

   /*
    *Map first 4MB shared memory
    *mark them: kernel mode, read and write, present
    *set as 011
    * */
   unsigned long address = 0;
   for(int i = 0; i < shared_frames_num; i++) {
        map_page_table[i] = address | PAGE_WRITE | PAGE_PRESENT;
        address += PAGE_SIZE;
    }

   //set the first PDE(page directory entry
   page_directory[0] = page_table_address | PAGE_WRITE | PAGE_PRESENT ;

   /*
    *update the page table directory
    *set to kernel mode, read and write, not present
   */
   address = 0;
   for (unsigned int i = 1; i < shared_frames_num; i++){
	  page_directory[i] = address | PAGE_WRITE ; 
   
   }

   //update current_page_table pointer
   current_page_table = this;


   Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{

   //set the pade directory address in CR3 register
   write_cr3((unsigned long)page_directory);

   Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
   //update paging bit to 1 in CR0 register
   write_cr0(read_cr0() | 0x80000000);

   paging_enabled = 1;

   Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
  // get the current page directory
  unsigned long * cur_page_directory = (unsigned long *) read_cr3();

  // read Page Fault Linear Address from CR2
  unsigned long fault_page_addr = read_cr2();
  unsigned long PD_addr = fault_page_addr >> PD_SHIFT;	// get bits 22-31 
  unsigned long PT_addr = fault_page_addr >> PT_SHIFT;	//// get bits 12-21

  unsigned long * page_table = NULL;
  unsigned long address = 0;
  
  if ((cur_page_directory[PD_addr] & PAGE_PRESENT ) == 1) {  //fault in Page table
            page_table = (unsigned long *)(cur_page_directory[PD_addr] & PDE_MASK);
            page_table[PT_addr & PTE_MASK] = (PageTable::process_mem_pool->get_frames
                                                (1)*PAGE_SIZE) | PAGE_WRITE | PAGE_PRESENT ;

        } else {
            cur_page_directory[PD_addr] = (unsigned long) ((kernel_mem_pool->get_frames
                                                (1)*PAGE_SIZE) | PAGE_WRITE | PAGE_PRESENT);

            page_table = (unsigned long *)(cur_page_directory[PD_addr] & PDE_MASK);

            for (int i = 0; i<1024; i++) {
                page_table[i] = address | PAGE_LEVEL ; // set the pages as user page
            }

            page_table[PT_addr & PTE_MASK] = (PageTable::process_mem_pool->get_frames
                                                (1)*PAGE_SIZE) | PAGE_WRITE | PAGE_PRESENT ;

        }

  Console::puts("handled page fault\n");
}

