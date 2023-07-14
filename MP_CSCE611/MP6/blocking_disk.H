/*
     File        : blocking_disk.H

     Author      : 

     Date        : 
     Description : 

*/

#ifndef _BLOCKING_DISK_H_
#define _BLOCKING_DISK_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

//#define ENABLE_THREAD_SYNC
/* To implement thread safe disk system UNCOMMENT this macro*/

//#define INTERRUPT_ENABLE
/*To implemet interrupt handler UNCOMMENT this macro*/
/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "simple_disk.H"
#include "thread.H"
#include "interrupts.H"
#include "scheduler.H"
/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* B l o c k i n g D i s k  */
/*--------------------------------------------------------------------------*/

class BlockingDisk : public SimpleDisk {

#ifdef INTERRUPT_ENABLE
   static Queue *blocking_Q;
   void handle_interrupt(REGS *_r);
#endif
public:
   BlockingDisk(DISK_ID _disk_id, unsigned int _size); 
   /* Creates a BlockingDisk device with the given size connected to the 
      MASTER or SLAVE slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness. 
      In a real system, we would infer this information from the 
      disk controller. */

   /* DISK OPERATIONS */

   virtual void read(unsigned long _block_no, unsigned char * _buf);
   /* Reads 512 Bytes from the given block of the disk and copies them 
      to the given buffer. No error check! */

   virtual void write(unsigned long _block_no, unsigned char * _buf);
   /* Writes 512 Bytes from the buffer to the given block on the disk. */
    
   bool is_ready_blocking();
protected:

   virtual void wait_until_ready();

     /* Is called after each read/write operation to check whether the disk is
        ready to start transfering the data from/to the disk. */

};

/*--------------------------------------------------------------------------*/
/* M i r r o r i n g D i s k  */
/*--------------------------------------------------------------------------*/
class MirroringDisk : public BlockingDisk{

private:
    BlockingDisk * MASTER_DISK;
    BlockingDisk * DEPENDENT_DISK;

    void issue_operation(DISK_OPERATION _op, unsigned long _block_no, DISK_ID Disk_ID);
     /* Send a sequence of commands to the controller to initialize the READ/WRITE 
        operation. This operation is called by read() and write(). */

public:
   MirroringDisk(DISK_ID _disk_id, unsigned int _size);
   /* Creates a MirroringDisk device with the given size connected to the
      MASTER or SLAVE slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness.
      In a real system, we would infer this information from the
      disk controller. */

   /* DISK OPERATIONS */

   virtual void read(unsigned long _block_no, unsigned char * _buf);
   /* Reads 512 Bytes from the given block of the disk and copies them
      to the given buffer. No error check! */

   virtual void write(unsigned long _block_no, unsigned char * _buf);
   /* Writes 512 Bytes from the buffer to the given block on the disk. */

protected:
    void wait_until_ready();

};

#endif
