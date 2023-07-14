/*
 File: ContFramePool.C
 
 Author:Tanu Shree
 Date  : 09/17/2022
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/
#define MASK_FREE 0x0
#define MASK_USED 0x1
#define MASK_HOS 0x2
#define MASK_INACC 0x3

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

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

ContFramePool* ContFramePool::frame_pool_head;
ContFramePool* ContFramePool::frame_pool_tail;

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

// This function sets the status of the frames
// Implemented using bitwise operation

void ContFramePool::set_state(unsigned long _frame_no, FrameState _state) {
    unsigned int bitmap_index = _frame_no / 4;
    unsigned int shift = (_frame_no % 4)*2;

    switch(_state) {
      case FrameState::Free:
      bitmap[bitmap_index] = (bitmap[bitmap_index]& ~((0x3) << shift)) | (MASK_FREE << shift);
      break;
      case FrameState::Used:
      bitmap[bitmap_index] = (bitmap[bitmap_index]& ~((0x3) << shift)) | (MASK_USED << shift);
      break;
      case FrameState::HoS:
      bitmap[bitmap_index] = (bitmap[bitmap_index]& ~((0x3) << shift)) | (MASK_HOS << shift);
      break;
      case FrameState::Inaccessible:
      bitmap[bitmap_index] = (bitmap[bitmap_index]& ~((0x3) << shift)) | (MASK_INACC << shift);
      break;

    }

}

// This function gets the status of the frames.
// Implemented using bitwise operation


ContFramePool::FrameState ContFramePool::get_state(unsigned long _frame_no) {
    unsigned int bitmap_index = _frame_no / 4;
    unsigned int shift = (_frame_no % 4)*2;
    unsigned char mask = 0x3 << shift;
    unsigned char _state = (bitmap[bitmap_index] & mask)>>shift;

    switch(_state) {
        case MASK_FREE:
        return FrameState::Free;
        break;
        case MASK_USED:
	return FrameState::Used;
        break;
	case MASK_HOS:
        return FrameState::HoS;
	break;
	case MASK_INACC:
        return FrameState::Inaccessible;
        break;
    }

}


//This is a Contructor. Single Linked list is used keep the list of instantiated pools.
//
ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED!
   // Console::puts("ContframePool::Constructor not implemented!\n");
    //assert(false);

    assert(_n_frames <= FRAME_SIZE * 8);

    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    info_frame_no = _info_frame_no;
    nFreeFrames = _n_frames;
    //Console::puts("Print nFreeFrames initial: ");
    //Console::puti(nFreeFrames);

    // If _info_frame_no is zero then we keep management info in the first
    //frame, else we use the provided frame to keep management info

    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }

    // Everything ok. Proceed to mark all frame as free.
    for(int fno = 0; fno < nframes; fno++) {
        set_state(fno, FrameState::Free);
    }

    // Mark the first frame as being used if it is being used
    if(info_frame_no == 0) {
        set_state(0, FrameState::Used);
        nFreeFrames--;
    }
    //Console::puts("\nprint all frames nitial  ");
    //print_frames();



    if (ContFramePool::frame_pool_head == NULL) {
        ContFramePool::frame_pool_head = this;
        ContFramePool::frame_pool_tail = this;
    } else {
        ContFramePool::frame_pool_tail->frame_pool_next = this;
        ContFramePool::frame_pool_tail = this;
    }
    frame_pool_next = NULL;



    Console::puts("Frame Pool initialized\n");

}

//This function gets the frames to be allocated.
//First frame marked HoS and rest are marked Used.

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{

    //Any frames left to allocate?
    assert(nFreeFrames > 0);
    unsigned long start_frame_no = 0;
    unsigned int count_frames = 0;
    for(int fno = 0; fno < nframes; fno++){
	    if (get_state(fno) == FrameState::Free){
		count_frames++;
		if(count_frames == _n_frames){
			start_frame_no = fno-(_n_frames-1);
			break;
		}
	     }
	     else
	     	count_frames = 0;
    }

    if(count_frames != _n_frames){
	Console::puts("Not enough continuous frames available!\n");
	return 0;
    }
    else{
	set_state(start_frame_no, FrameState::HoS);
	nFreeFrames--;
	for(int fno = start_frame_no+1; fno< (start_frame_no + _n_frames); fno++){
		set_state(fno, FrameState::Used);
		nFreeFrames--;
	
	}
	//Console::puts("print all frames after setting   ");
        //print_frames();

	return  base_frame_no + start_frame_no;

    }
	

    
}

//This function marks the frames inaccessible that cannot be accessed in memory.
//
void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
   	
	//first set the start frames of inaccessable frames to HOS
	unsigned long inacc_start_frame = _base_frame_no - base_frame_no;
	set_state(inacc_start_frame, FrameState::HoS);
	nFreeFrames--;

	//set the next specified frames to inaccessable state
	unsigned int frame_no;
	for(frame_no= inacc_start_frame + 1; frame_no < inacc_start_frame + _n_frames ; frame_no++){
		set_state(frame_no, FrameState::Inaccessible);
                nFreeFrames--;

	}
	
	//Console::puts("print all frames after setting inaccessible   ");
	//print_frames();

}
//This function is to print status of frames
//used for debugging
/*void ContFramePool::print_frames(){
        for(unsigned long i =0; i<nframes; i++){
                Console::puti((unsigned long) get_state(i));

        }
}*/

//This function releases the frames which can be further used by other process

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED!
    
	// Find the pool to which this frame belongs
    	ContFramePool* current_pool = ContFramePool::frame_pool_head;
    	while ( (current_pool->base_frame_no > _first_frame_no || current_pool->base_frame_no + current_pool->nframes <= _first_frame_no) ) {
        	if (current_pool->frame_pool_next == NULL) {
            		Console::puts("Frame not found in any pool, cannot release. \n");
            		return;
        	}
	       	else {
            		current_pool = current_pool->frame_pool_next;
        	}
   	 }
	//pool found now set them free
	unsigned long next_frame_no = _first_frame_no - current_pool->base_frame_no;
        if(current_pool->get_state(next_frame_no) == FrameState::HoS){
                current_pool->set_state(next_frame_no , FrameState::Free);
                current_pool->nFreeFrames++;

                next_frame_no++;
                while(current_pool->get_state(next_frame_no) == FrameState::Used){
                        current_pool->set_state(next_frame_no, FrameState::Free);
                        current_pool->nFreeFrames++;
                        next_frame_no++;
                }
        }

	else
		Console::puts("Frame not HoS, cannot release");

}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    return (_n_frames*2)/(32 KB) + ((_n_frames*2) % (32 KB) > 0 ? 1 : 0);
}
