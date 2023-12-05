#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
/* takes three argument:
 * starting virtual address of the first user page to check(argaddr())
 * number of pages to check(argint()) 
 * a user address to a buffer to store the results into a bitmask(copyout())
 */

int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 a, uva, usrbuf, bitmask;
  int npages, i;
  pte_t *ptep;
  pagetable_t pagetable = myproc()->pagetable;

  if(argaddr(0 , &uva) < 0)
    return -1;
  if(argint(1, &npages) < 0)
    return -1;
  if(argaddr(2, &usrbuf) < 0)
    return -1;

  memset(&bitmask, 0, sizeof(bitmask)); // initialize the bitmask

  for(i = 0, a = uva; i < npages; i++, a+=PGSIZE){
    if((ptep = walk(pagetable, a, 0)) == 0)
      panic("sys_pgaccess: walk");
    /*
    if((*ptep & PTE_V) == 0)
      panic("sys_pgaccess: not mapped");
    if(PTE_FLAGS(*ptep) == PTE_V)
      panic("sys_pgaccess: not a leaf");
    */

    if(*ptep & PTE_A){  /* pages have been accessed */
      // set corresponding bit in bitmask
      bitmask = bitmask | (1 << i);
    }
    /* Clear PTE_A after checking if it is set */
    *ptep = *ptep & (~PTE_A);
  }
  copyout(pagetable, usrbuf, (char *)&bitmask, sizeof(bitmask));
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;
  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
