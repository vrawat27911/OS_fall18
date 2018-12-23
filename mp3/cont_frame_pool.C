/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
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

/* -- (none) -- */

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

ContFramePool* ContFramePool::pools;

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

static unsigned char BITMASK[] = { 128, 64, 32, 16, 8, 4, 2, 1 };

// set nth bit
void setBit (unsigned char& pbyte, int n)
{
    pbyte  = pbyte | BITMASK[n];
}

//clear nth bit
void clearBit (unsigned char& pbyte, int n)
{
    pbyte = pbyte & (~BITMASK[n]);
}


// check whether nth bit is one
bool IsBitOne (unsigned char& pbyte, int n) 
{
    bool bOne = (pbyte & BITMASK[n]) == 0 ? false : true;
    return bOne;
}


ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    assert(_n_frames <= FRAME_SIZE * 8);

    freeFramesCnt = _n_frames;
    framesCnt = _n_frames;
    base_frame_no = _base_frame_no;    
    info_frame_no = _info_frame_no;
    infoFramesCnt = _n_info_frames;

    // initialize b keep bitmap in baseframe if _info_frame_no set to zero
    // else make use of _info_frame_no
    unsigned long initial_frame_no;

    if(_info_frame_no == 0)
    {
        initial_frame_no = _base_frame_no;
    }
    else
    {
        initial_frame_no = _info_frame_no;
    }

    bitmap = (unsigned char *) (initial_frame_no * FRAME_SIZE);
    bitmap2 = bitmap + 1024;

    // Number of frames must be "fill" the bitmap!
    assert ((framesCnt % 8 ) == 0);

    // Everything ok. Proceed to mark all bits in the bitmap
    for(int i=0; i*8 < _n_frames; i++) 
    {
        bitmap[i] = 0xFF;
        bitmap2[i] = 0xFF;
        // Console::puti(bitmap[i]); Console::puts("bitmap value \n");
    }

    // if baseframe is used for management info - mark first frame to be allocated
    if(_info_frame_no == 0)
    {
        bitmap[0] = 0x7F;
        info_frame_no = _base_frame_no;
        freeFramesCnt--;
    }

    if(ContFramePool::pools == NULL)
    {
        pools = this;
    }
    else
    {
        pools->prev = this;
        next = pools;
        pools = pools->prev;
    }
}


unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // if fails, return 0
    // assert(freeFramesCnt > 0);
    // assert(freeFramesCnt >= _n_frames);

    int cnt;
    int numBytes = framesCnt/8 + framesCnt%8;

    for(int i = 0; i < numBytes;)
    {
        // Console::puti(i); Console::puts(" i value \n"); Console::puti(bitmap2[i]); Console::puts("bitmap value \n");

        // all bytes are used up
        if ((bitmap[i] == 0) || (bitmap2[i] == 0))
        {
            // Console::puti("me");
            i++;
            continue;
        }

        int firstEmpty = -1;
        bool bfound = true;

        for(int jj = 0; jj < 8; jj++)
        {
            if(IsBitOne(bitmap[i], jj) && IsBitOne(bitmap2[i], jj))
            {
                firstEmpty = jj;
                break;
            }
        } 

        if(firstEmpty == -1)
        {
            // Console::puts("me1");
            i++;
            continue;
        }
              
        int index = i;
        int j = firstEmpty;
        cnt = 0;
    
        while(index < numBytes)
        {
            // Console::puti(index); Console::puts(" index value \n"); Console::puti(j); Console::puts(" J value \n");

            for(; j < 8; j++)
            {            
                // bit is free
                if(IsBitOne(bitmap[index], j) && IsBitOne(bitmap2[index], j))
                {
                    cnt++;
                    // Console::puti(firstEmpty); Console::puts("FirstEmpty");
                    // Console::puti(i); Console::puts(" I values");

                    if(cnt == _n_frames)
                    {
                        for(int pos = i*8 + firstEmpty; pos < i*8 + firstEmpty + _n_frames; pos++)
                        {
                            if(pos == i*8 + firstEmpty)
                            {
                                // set as head of seq
                                clearBit(bitmap[i], firstEmpty);
                                clearBit(bitmap2[i], firstEmpty);
                            }
                            else
                            {
                                int tempIndex = pos/8;
                                int bitpos = pos%8;
                                
                                // set as allocated
                                clearBit(bitmap[tempIndex], bitpos);
                            }
                        }
                        
                        freeFramesCnt -= _n_frames;
                        return (base_frame_no + 8 * i + firstEmpty);
                    }
                }
                else
                {
                    // Console::puti(index); Console::puts("Index");
                    // Console::puti(bitmap[index]); Console::puts(" bitmap");
                    bfound = false;
                    break;
                }
            }

            
            if(bfound == false)
            {
                if(i == index)
                    i++;
                else
                    i = index;

                break;
            }
            else
            {
                if(index == numBytes - 1)
                    i = numBytes;
                
                index++;
                j = 0;    
            } 
        }
    }

    Console::puts("Requested no of frames are not available. \n");
    return 0;
}

void ContFramePool::mark_inaccessible(unsigned long _frame_no, unsigned long _n_frames)
{

    assert ((_frame_no >= base_frame_no) && (_frame_no + _n_frames <= base_frame_no + framesCnt));
   
    // set bits as not unavailable  
    for(int i = _frame_no; i < _frame_no + _n_frames; i++)
    {
        int diff = i - base_frame_no;
        int index = diff/8;
        int bitIndex = diff%8;
    
        clearBit(bitmap2[index], bitIndex);
    }

    freeFramesCnt -= _n_frames;
}


void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    ContFramePool * current = ContFramePool::pools;

    // find the pool in which first frame num lies
    while(!(current->base_frame_no <= _first_frame_no && (current->base_frame_no + current->framesCnt > _first_frame_no)))
    {
        if(current->next != NULL)
        {
            current = current->next;
        }
        else
        {
            Console::puts("Error: Frame to be released does not exist in frame pools. \n");
            // assert(false);
            return;
        }
    }

    int diff = _first_frame_no - current->base_frame_no;    

    // contains starting byte for bitmap of given first_frame
    unsigned char * bitmap_byte = &current->bitmap[diff/8];
    unsigned char * bitmap2_byte = &current->bitmap2[diff/8];

    // first bit should be head of sequence - 00
    if(!(!IsBitOne(current->bitmap[diff/8], diff%8) && !IsBitOne(current->bitmap2[diff/8], diff%8)))
    {
        Console::puts("Error : Frame to be released does not start with Head of Sequence \n");
        return;
    }

    // clear bits from bitmap and bitmap2 to mark them free     
    setBit(*bitmap_byte, diff % 8);
    setBit(*bitmap2_byte, diff %  8);
    int i = _first_frame_no + 1;
    current->freeFramesCnt++;

    // Console::puti(current->base_frame_no); Console::puts("\n");
    // Console::puti(_first_frame_no); Console::puts("\n");
        
    for(; i < current->base_frame_no + current->framesCnt; i++)
    {
        diff = i - current->base_frame_no;
        int index = diff/8;
        int bitIndex = diff%8;

        // next head of sequence reached.
        if(!IsBitOne(current->bitmap[index], bitIndex) && !IsBitOne(current->bitmap2[index], bitIndex))
        {
            break;
        }
        // free seq of frames started
        else if (IsBitOne(current->bitmap[index], bitIndex) && IsBitOne(current->bitmap2[index], bitIndex))
        {
            break;
        }
        else
        {
            // set as unallocated
            if (!IsBitOne(current->bitmap[index], bitIndex))
            {
                setBit(current->bitmap[index], bitIndex);
                current->freeFramesCnt++;
            }
            // once we enter inaccesible frames seq we must break since no allocated frames in inaccessible region
            else if (!IsBitOne(current->bitmap2[index], bitIndex))
            {
                break;
                //setBit(current->bitmap2[index], bitIndex);
            }
        }
    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    int frameForRemainder = _n_frames % 16384 > 0 ? 1 : 0;
    int framesNeeded = _n_frames/16384 + frameForRemainder;

    return framesNeeded;
}
