/*
 File: Page_table.C
 
 Author:Tanu Shree
 Date  : 10/20/2022
 
 */


#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"


#define PAGE_PRESENT        1		
#define PAGE_WRITE          2


PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
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
   // one frame for page directory
   page_directory = (unsigned long *)(process_mem_pool->get_frames(1)*PAGE_SIZE);

   //one frame for page table
   unsigned long * map_page_table = (unsigned long *) (process_mem_pool->get_frames(1)* PAGE_SIZE);

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
   page_directory[0] = (unsigned long) map_page_table | PAGE_WRITE | PAGE_PRESENT;

   /*
    *update the page table directory
    *set to kernel mode, read and write, not present
   */
   address = 0;
   for (unsigned int i = 1; i < shared_frames_num; i++){
	  page_directory[i] = address | PAGE_WRITE ; 
   
   }

   //set the last directory to itself( for recursive lookup)
   page_directory[shared_frames_num - 1] = (unsigned long) page_directory | PAGE_WRITE | PAGE_PRESENT;

   // Initialize the Virtual Memory Pool
    for(int i = 0 ; i < VM_POOL_SIZE; i++) {
        reg_vm_pool[i] = NULL;
    }
    vm_pool_no = 0;

   
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

static unsigned long *PTE_addr(unsigned long _page_no)
{
	return(unsigned long *)((0xFFC00000 | (_page_no >> 10)) & 0xFFFFFFFC);
}

static unsigned long *PDE_addr(unsigned long _page_no)
{
        return(unsigned long *)((0xFFFFF000 | (_page_no >> 20)) & 0xFFFFFFFC);
}


void PageTable::handle_fault(REGS * _r)
{

  // read Page Fault Linear Address from CR2
  unsigned long fault_page_addr = read_cr2();
  unsigned long *PD_addr = (unsigned long *) PDE_addr(fault_page_addr);	// get bits 22-31 
  unsigned long *PT_addr = (unsigned long *) PTE_addr(fault_page_addr); 	//// get bits 12-21

  
  //Check whether the page fault address is legitimate  
  VMPool ** vm_pool = current_page_table->reg_vm_pool;
  bool legit = false;
  for (int i = 0; i < current_page_table->vm_pool_no; i++) {
     if (vm_pool[i] != NULL) {
        if (vm_pool[i]->is_legitimate(fault_page_addr)) {
            legit = true;
            break;
        }
     }
  }
  if (current_page_table->vm_pool_no > 0){
    assert(legit);
  } 

  if ((*PD_addr & PAGE_PRESENT ) == 1) {  //directory is correct but fault in Page table
            *PT_addr = (PageTable::process_mem_pool->get_frames
                                                (1)*PAGE_SIZE) | PAGE_WRITE | PAGE_PRESENT ;

        } else {
            *PD_addr = (unsigned long) ((process_mem_pool->get_frames
                                                (1)*PAGE_SIZE) | PAGE_WRITE | PAGE_PRESENT);

            *PT_addr = (unsigned long) (PageTable::process_mem_pool->get_frames
                                                (1)*PAGE_SIZE) | PAGE_WRITE | PAGE_PRESENT ;

        }

  Console::puts("handled page fault\n");
}
void PageTable::register_pool(VMPool * _vm_pool)
{
    //assert(false);
    if (vm_pool_no < VM_POOL_SIZE) {
        reg_vm_pool[vm_pool_no++] = _vm_pool;
        Console::puts("registered VM pool\n");
    } else {
        Console::puts("VM POOL is already full, cannot register");
    }
}

void PageTable::free_page(unsigned long _page_no) {

    unsigned long *PT_addr = (unsigned long *) PTE_addr(_page_no); 

    if(*PT_addr & 0x1){
        unsigned long frame_no  = *PT_addr / (Machine::PAGE_SIZE);
        process_mem_pool->release_frames(frame_no);
        *PT_addr = 0 | PAGE_WRITE ;
    }

    // Flush TLB
    write_cr3((unsigned long) page_directory);

}
                       
