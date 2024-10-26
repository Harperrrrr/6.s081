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

#define BUCKETSZ 13

extern uint ticks;

struct bucket{
  struct spinlock lock;
  struct buf head;
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct bucket buckets[BUCKETSZ];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

int hash(uint dev, uint blockno){
  const int primes1 = 31;
  const int primes2 = 37;
  return ((dev * primes1 + blockno) * primes2) % BUCKETSZ;
}

void
binit(void)
{
  // struct buf *b;
  initlock(&bcache.lock, "bcache");

  static char name[BUCKETSZ][20];
  for(int i = 0; i < BUCKETSZ; ++i){
    snprintf(name[i], sizeof(name), "bcache%d", i);
    initlock(&bcache.buckets[i].lock, name[i]);
  }

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  // for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   initsleeplock(&b->lock, "buffer");
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // acquire(&bcache.lock);

  // Is the block already cached?
  // for(b = bcache.head.next; b != &bcache.head; b = b->next){
  //   if(b->dev == dev && b->blockno == blockno){
  //     b->refcnt++;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  int curhs = hash(dev, blockno);
  struct bucket *curbkt = &bcache.buckets[curhs];
  acquire(&curbkt->lock);
  for(b = curbkt->head.next; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&curbkt->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&curbkt->lock);


  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  acquire(&bcache.lock);
  uint least = __INT32_MAX__;
  struct buf *evtbuf = 0;
  for(int i = 0; i < NBUF; ++i){
    int tmphs = hash(bcache.buf[i].dev, bcache.buf[i].blockno);
    acquire(&bcache.buckets[tmphs].lock);
    if(bcache.buf[i].refcnt == 0 && bcache.buf[i].timestamp < least){
      least = bcache.buf[i].timestamp;
      evtbuf = &bcache.buf[i];
    }
    release(&bcache.buckets[tmphs].lock);
  }
  if(!evtbuf)
    panic("bget: no buffers");
  
  int orihs = hash(evtbuf->dev, evtbuf->blockno);
  struct bucket *oribkt = &bcache.buckets[orihs];
  if(orihs < curhs){
    acquire(&oribkt->lock);
    acquire(&curbkt->lock);
  } else if(orihs > curhs){
    acquire(&curbkt->lock);
    acquire(&oribkt->lock);
  } else {
    acquire(&oribkt->lock);
  }

  struct buf *prev = &curbkt->head;
  for(b = curbkt->head.next; b; b = b->next, prev = prev->next){
    if(b->dev == dev && b->blockno == blockno){
      // 如果已经挂上去了
      b->refcnt++;
      release(&curbkt->lock);
      if(orihs != curhs){
        release(&oribkt->lock);
      }
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  prev->next = evtbuf;
  evtbuf->prev = prev;
  evtbuf->next = 0;
  evtbuf->dev = dev;
  evtbuf->blockno = blockno;
  evtbuf->valid = 0;
  evtbuf->refcnt = 1;
  if(evtbuf->prev)
    evtbuf->prev->next = evtbuf->next;
  if(evtbuf->next)
    evtbuf->next->prev = evtbuf->prev;
  release(&curbkt->lock);
  if(orihs != curhs){
    release(&oribkt->lock);
  }
  release(&bcache.lock);
  acquiresleep(&evtbuf->lock);
  return evtbuf;
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
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int hs = hash(b->dev, b->blockno);
  acquire(&bcache.buckets[hs].lock);
  b->refcnt--;
  if(b->refcnt == 0)
    b->timestamp = ticks;
  release(&bcache.buckets[hs].lock);

  // acquire(&bcache.lock);
  // b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  
  // release(&bcache.lock);
}

void
bpin(struct buf *b) {
  int hs = hash(b->dev, b->blockno);
  acquire(&bcache.buckets[hs].lock);
  b->refcnt++;
  release(&bcache.buckets[hs].lock);
}

void
bunpin(struct buf *b) {
  int hs = hash(b->dev, b->blockno);
  acquire(&bcache.buckets[hs].lock);
  b->refcnt--;
  release(&bcache.buckets[hs].lock);
}


