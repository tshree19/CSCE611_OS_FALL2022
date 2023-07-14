/*
     File        : blocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "simple_disk.H"
#include "machine.H"

/*--------------------------------------------------------------------------*/
/* EXTERNS */
/*--------------------------------------------------------------------------*/
extern Scheduler *SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* MUTEX Definition */
/*--------------------------------------------------------------------------*/

#ifdef ENABLE_THREAD_SYNC
int lock;
int TestAndSet(int *lock){
    int temp = *lock;
    *lock = 1;
    return temp;
}

void lock_init(int *lock){
    *lock = 0;
}

void lock_aquire(){
    while(TestAndSet(&lock));
}

void lock_release(){
    lock = 0;
}
#endif
/*--------------------------------------------------------------------------*/
/* BLOCKING DISK */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

#ifdef INTERRUPT_ENABLE
Queue *BlockingDisk::blocking_Q = new Queue();
#endif

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
#ifdef ENABLE_THREAD_SYNC
  lock_init(&lock);
#endif
}

/*--------------------------------------------------------------------------*/
/* BLOCKING_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

//read operation issued
void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
#ifdef ENABLE_THREAD_SYNC
    lock_aquire();
#endif
    SimpleDisk::read(_block_no, _buf);
#ifdef ENABLE_THREAD_SYNC
    lock_release();
#endif
}

//returns the status of the disk
bool BlockingDisk::is_ready_blocking() {
   return ((Machine::inportb(0x1F7) & 0x08) != 0);
}

//Write operation issued
void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
#ifdef ENABLE_THREAD_SYNC
    lock_aquire();
#endif
    SimpleDisk::write(_block_no, _buf);
#ifdef ENABLE_THREAD_SYNC
    lock_release();
#endif
}


//Disk status is checked and if not ready
//pushed into blocking queue and yield if interrupt is enabled
// otherwise resume and yield
void BlockingDisk::wait_until_ready() {

    while (!is_ready_blocking()) {
#ifdef INTERRUPT_ENABLE
        blocking_Q->enQueue(Thread::CurrentThread());
#else
        SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
#endif
        SYSTEM_SCHEDULER->yield();


    }
}
//Interrupt is handeled and the thread from blocking queue is popped out
//and is resumed.
#ifdef INTERRUPT_ENABLE
void BlockingDisk::handle_interrupt(REGS *_r){
    struct Qnode *node = blocking_Q->deQueue();
    Thread * block_thread = node->thread;
    SYSTEM_SCHEDULER->resume(block_thread);
}
#endif
/*--------------------------------------------------------------------------*/
/* MIRRORING_DISK */
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/
MirroringDisk::MirroringDisk(DISK_ID _disk_id, unsigned int _size): BlockingDisk(_disk_id, _size)
{
    MASTER_DISK = new BlockingDisk(DISK_ID::MASTER, _size);
    DEPENDENT_DISK = new BlockingDisk(DISK_ID::DEPENDENT, _size);

}
/*--------------------------------------------------------------------------*/
/* MIRROR_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

//checks the status of the master and dependent disk
//if either of them not ready, resume and yield
void MirroringDisk::wait_until_ready()
{
    while (!MASTER_DISK->is_ready_blocking() || !DEPENDENT_DISK->is_ready_blocking())
    {
        SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
        SYSTEM_SCHEDULER->yield();
    }
}


//This function is taken from Simple disk
void MirroringDisk::issue_operation(DISK_OPERATION _op, unsigned long _block_no, DISK_ID Disk_ID) {

  Machine::outportb(0x1F1, 0x00); /* send NULL to port 0x1F1         */
  Machine::outportb(0x1F2, 0x01); /* send sector count to port 0X1F2 */
  Machine::outportb(0x1F3, (unsigned char)_block_no);
                         /* send low 8 bits of block number */
  Machine::outportb(0x1F4, (unsigned char)(_block_no >> 8));
                         /* send next 8 bits of block number */
  Machine::outportb(0x1F5, (unsigned char)(_block_no >> 16));
                         /* send next 8 bits of block number */
  unsigned int disk_no = Disk_ID == DISK_ID::MASTER ? 0 : 1;
  Machine::outportb(0x1F6, ((unsigned char)(_block_no >> 24)&0x0F) | 0xE0 | (disk_no << 4));
                         /* send drive indicator, some bits,
                            highest 4 bits of block no */

  Machine::outportb(0x1F7, (_op == DISK_OPERATION::READ) ? 0x20 : 0x30);

}


//Read operation issued to master and dependent disks
//the one which returns early reads data from port.
void MirroringDisk::read(unsigned long _block_no, unsigned char * _buf)
{
    issue_operation(DISK_OPERATION::READ, _block_no, DISK_ID::MASTER);
    issue_operation(DISK_OPERATION::READ, _block_no, DISK_ID::DEPENDENT);
    wait_until_ready();

    /* read data from port */
    int i;
    unsigned short tmpw;
    for (i = 0; i < 256; i++) {
        tmpw = Machine::inportw(0x1F0);
        _buf[i*2]   = (unsigned char)tmpw;
        _buf[i*2+1] = (unsigned char)(tmpw >> 8);
    }

}

//write operation issued to both master an depnedent disks
// data is written to both master and dependent disk
void MirroringDisk::write(unsigned long _block_no, unsigned char * _buf)
{
    MASTER_DISK->write (_block_no, _buf);
    DEPENDENT_DISK->write (_block_no, _buf);
}

