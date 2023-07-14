/*
 File: scheduler.C
 
 Author: Tanu Shree
 Date  : Nov 5, 2022
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

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
/* METHODS FOR CLASS   Q u e u e  */
/*--------------------------------------------------------------------------*/
struct Qnode *Queue::front = NULL;
struct Qnode *Queue::rear = NULL;

Queue::Queue() {
//constructer for queue
}

//add an item into the queue
void Queue::enQueue(Thread * _thread){
    
    struct Qnode *new_node = new Qnode();
    new_node->thread = _thread;
    new_node->next = NULL;
    
    //If queue is empty
    if (Queue::front == NULL ){
        Queue::front = Queue::rear = new_node;
        return;
    }
    
    //add new node at the end
    Queue::rear->next = new_node;
    Queue::rear = new_node;

}

// remove an item(here thread) from the ready queue
struct Qnode *Queue::deQueue()
{
  if (Queue::front == NULL) {
    return NULL;
  }

  // get the front node
  struct Qnode *node = Queue::front;
  Queue::front = Queue::front->next;

  // on deleting, if front becomes to NULL, then make rear NULL as well
  if (Queue::front == NULL) {
    Queue::rear = NULL;
  }

  return node;
}


/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/
Queue *Scheduler::ready_Q = new Queue();

Scheduler::Scheduler() {
  
  Console::puts("Constructed Scheduler.\n");
}


void Scheduler::yield() {
 
 //get the first node from the ready queue and dispatch it to the CPU
  struct Qnode *node = ready_Q->deQueue();
  Thread * ready_thread = node->thread;

  if(ready_thread != NULL) {
    Thread::dispatch_to(ready_thread);
    delete node;
  }
  //Enable interrupts 
  if(!Machine::interrupts_enabled()) {
    Machine::enable_interrupts();
  }
}

void Scheduler::resume(Thread * _thread) {
  
  //disable interrupts
  if(Machine::interrupts_enabled()) {
    Machine::disable_interrupts();
  }
  // add the currently running thread to end of ready queue.
  //thread that was either waiting for event or preempted to give up the CPU
  if(_thread != NULL){
    ready_Q->enQueue(_thread);
  }
}

void Scheduler::add(Thread * _thread) {
  // add thread to ready queue using 'resume'
  this->resume(_thread);
}

void Scheduler::terminate(Thread * _thread) {
    
//   Console::puts("Terminated!!\n");
   
   unsigned int threadID = _thread->ThreadId();
   struct Qnode *curr_node = Queue::front;
   struct Qnode *prev_node = NULL;
   
   //if the terminating thread is in front
   if(curr_node != NULL && curr_node->thread->ThreadId() == threadID){
     Queue::front = curr_node->next;
     delete curr_node;

     return;
   }

   //check until we find the terminating thread in the list
   while(curr_node != NULL && curr_node->thread->ThreadId() != threadID){
     prev_node = curr_node;
     curr_node = curr_node->next;
   }
   
   // remove the node containing that thread from the list
   if(curr_node != NULL){
     prev_node->next = curr_node->next;
     delete curr_node;
   }
   Console::puts("Thread ");
   Console::putui(threadID);
   Console::puts(" Terminated!!\n");

}


/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS  RR S c h e d u l e r  */
/*--------------------------------------------------------------------------*/


RRScheduler::RRScheduler(int _time_quantum){
  /* This constructor initializes the member variables, sets the frequency
    and registers interrupt handler.
  */
  time_quantum = _time_quantum;
  ticks = 0;
  set_frequency(time_quantum);

  // register the interrupt handler
  InterruptHandler::register_handler(0, this);

  Console::puts("Constructed RR Scheduler.\n");
}


void RRScheduler::handle_interrupt(REGS *_r) {
  // increment ticks count
  ticks++;

  // update when time quantum is over
  if (ticks >= time_quantum)
  {
    ticks = 0;
    Console::puts("Quantum time has passed\n");

    //preempt the current thread and execute the next one 
    this->resume(Thread::CurrentThread());
    this->yield();
  }
}


void RRScheduler::set_frequency(int _hz) {
  /*
    This function is re-used from the SimpleTimer implementation.
    It sets the frequency at which interrupts will be fired.
  */

  int divisor = 1193180 / _hz;

  // set the PIT channel 0
  Machine::outportb(0x43, 0x34);
  Machine::outportb(0x40, divisor & 0xFF);
  Machine::outportb(0x40, divisor >> 8);
}


void RRScheduler::yield() {

  ticks   =  0;
  Machine::outportb(0x20, 0x20); //End of Interrupt message sent to PIC
 
 //get the first node from the ready queue and dispatch it to the CPU
  struct Qnode *node = ready_Q->deQueue();
  Thread * ready_thread = node->thread;

  if(ready_thread != NULL) {
    Thread::dispatch_to(ready_thread);
    delete node;
  }
  //Enable interrupts 
  if(!Machine::interrupts_enabled()) {
    Machine::enable_interrupts();
  }
}



