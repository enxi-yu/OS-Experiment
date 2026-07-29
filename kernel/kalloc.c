// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
#ifdef LAB_PGTBL
  struct run *superfreelist;
#endif
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

#ifdef LAB_PGTBL
void *
superalloc(void)
{
  struct run *prev, *start, *cur, *after;
  int i;

  acquire(&kmem.lock);
  prev = 0;
  for(start = kmem.freelist; start; prev = start, start = start->next){
    cur = start;
    for(i = 1; i < SUPERPGSIZE / PGSIZE; i++){
      if(cur->next == 0 || (uint64)cur->next != (uint64)cur - PGSIZE)
        break;
      cur = cur->next;
    }
    if(i == SUPERPGSIZE / PGSIZE){
      uint64 base = (uint64)cur;
      if(base % SUPERPGSIZE != 0)
        continue;
      after = cur->next;
      if(prev)
        prev->next = after;
      else
        kmem.freelist = after;
      release(&kmem.lock);
      memset((void*)base, 5, SUPERPGSIZE);
      return (void*)base;
    }
  }
  release(&kmem.lock);
  return 0;
}

void
superfree(void *pa)
{
  char *p = (char*)pa;

  if(((uint64)pa % SUPERPGSIZE) != 0 || p < end || (uint64)pa + SUPERPGSIZE > PHYSTOP)
    panic("superfree");
  for(; p < (char*)pa + SUPERPGSIZE; p += PGSIZE)
    kfree(p);
}
#endif
