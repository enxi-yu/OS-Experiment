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

struct kmem_cpu {
  struct spinlock lock;
  struct run *freelist;
};

struct kmem_cpu kmem[NCPU];

static char kmem_names[NCPU][16];

static int
kmem_cpuid(void)
{
  int id;

  push_off();
  id = cpuid();
  pop_off();
  return id;
}

static struct run *
steal_pages(int id)
{
  struct run *r;
  struct run *tail;
  int n;

  for(int i = 0; i < NCPU; i++){
    if(i == id)
      continue;

    acquire(&kmem[i].lock);
    if(kmem[i].freelist == 0){
      release(&kmem[i].lock);
      continue;
    }

    // Steal only a small fixed-size batch to keep lock hold times short.
    r = kmem[i].freelist;
    tail = r;
    n = 1;
    while(n < 64 && tail->next){
      tail = tail->next;
      n++;
    }
    kmem[i].freelist = tail->next;
    tail->next = 0;
    release(&kmem[i].lock);
    return r;
  }

  return 0;
}

void
kinit()
{
  for(int i = 0; i < NCPU; i++){
    snprintf(kmem_names[i], sizeof(kmem_names[i]), "kmem%d", i);
    initlock(&kmem[i].lock, kmem_names[i]);
    kmem[i].freelist = 0;
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  int cpu = 0;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    struct run *r = (struct run*)p;
    memset(p, 1, PGSIZE);
    acquire(&kmem[cpu].lock);
    r->next = kmem[cpu].freelist;
    kmem[cpu].freelist = r;
    release(&kmem[cpu].lock);
    cpu = (cpu + 1) % NCPU;
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int id;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  id = kmem_cpuid();
  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int id;

  id = kmem_cpuid();

  acquire(&kmem[id].lock);
  r = kmem[id].freelist;
  if(r)
    kmem[id].freelist = r->next;
  release(&kmem[id].lock);

  if(r == 0){
    r = steal_pages(id);
    if(r){
      struct run *rest = r->next;
      r->next = 0;

      if(rest){
        struct run *tail = rest;
        while(tail->next)
          tail = tail->next;

        acquire(&kmem[id].lock);
        tail->next = kmem[id].freelist;
        kmem[id].freelist = rest;
        release(&kmem[id].lock);
      }
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
