// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct buf heads[NBUCKET];
  struct spinlock buclocks[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for(int i = 0; i < NBUCKET; i++){
    initlock(&bcache.buclocks[i], "bcache.bucket");
    // Create sentinel of buffers
    bcache.heads[i].prev = &bcache.heads[i];
    bcache.heads[i].next = &bcache.heads[i];
  }

  /* allocate all free blocks to bucket[0] when initialize */
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.heads[0].next;
    b->prev = &bcache.heads[0];
    initsleeplock(&b->lock, "buffer");
    bcache.heads[0].next->prev = b;
    bcache.heads[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int index;

  index = blockno % 13;

  acquire(&bcache.buclocks[index]);

  // Is the block already cached?
  for(b = bcache.heads[index].next; b != &bcache.heads[index]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->timestamp = ticks;
      release(&bcache.buclocks[index]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.buclocks[index]);

  acquire(&bcache.lock);
  acquire(&bcache.buclocks[index]);

  /* re-check if the block is cached */
  for(b = bcache.heads[index].next; b != &bcache.heads[index]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->timestamp = ticks;
      release(&bcache.buclocks[index]);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  /* first, search in current bucket for victim */
  for(b = bcache.heads[index].next; b != &bcache.heads[index]; b = b->next){
    /* start from first fit, instead of LRU */
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      b->timestamp = ticks;             /* update timestamp */
      release(&bcache.buclocks[index]);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  /* steal from others */
  for(int i = 0; i < NBUCKET; i++){
    if(i == index)
      continue;
    acquire(&bcache.buclocks[i]);
    for(b = bcache.heads[i].next; b != &bcache.heads[i]; b = b->next){
      /* start from first fit, instead of LRU */
      if(b->refcnt == 0) {
        /* delete from old bucket[i] */
        b->prev->next = b->next;
        b->next->prev = b->prev; 
        release(&bcache.buclocks[i]);

        /* update block info */
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->timestamp = ticks;             /* update timestamp */

        /* add to new bucket[index] */
        b->next = bcache.heads[index].next;
        b->prev = &bcache.heads[index];
        (bcache.heads[index].next)->prev = b;
        bcache.heads[index].next = b;

        release(&bcache.buclocks[index]);
        release(&bcache.lock);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.buclocks[i]);
  }

  release(&bcache.buclocks[index]);
  release(&bcache.lock);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  int index;

  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  index = b->blockno % 13;
  acquire(&bcache.buclocks[index]);
  b->refcnt--;
  release(&bcache.buclocks[index]);
}

void
bpin(struct buf *b) {
  int index = b->blockno % 13;
  acquire(&bcache.buclocks[index]);
  b->refcnt++;
  release(&bcache.buclocks[index]);
}

void
bunpin(struct buf *b) {
  int index = b->blockno % 13;
  acquire(&bcache.buclocks[index]);
  b->refcnt--;
  release(&bcache.buclocks[index]);
}


