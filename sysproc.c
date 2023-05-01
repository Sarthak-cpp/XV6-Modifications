#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

#define Q_SIZE 500
#define NO_OF_PIPES 25001 // 25000 pipes
enum traceState {TRACE_ON , TRACE_OFF};
enum traceState trace = TRACE_OFF;
enum waitState {WAIT_ON, WAIT_OFF};
enum waitState wait_states[NPROC] = {[0 ... (NPROC-1)] = WAIT_OFF};

int min(int a, int b){
  if(a <= b) return a;
  return b;
}

typedef struct{ // Pipe for inter process communication
  struct spinlock lock;
  int receiver[NO_OF_PIPES];
  char buffer[NO_OF_PIPES][8];
}longPipe;

longPipe pipes = {
  .receiver = { [0 ... (NO_OF_PIPES-1)] = -1 },
  .buffer = { [0 ... (NO_OF_PIPES-1)] = "$$$$$$$"}
};
// Allocating a total of 9999 pipes for communication

typedef struct{ // Queue implemented as circular array
  struct spinlock lock;
  int pipe_indices[Q_SIZE];
  int head;
  int tail;
}queue;

queue receiver_queue[NPROC] = {
  [0 ... (NPROC-1)] = {
    .pipe_indices = {0},
    .head = 0,
    .tail = 0
  }
};

int push(int recv_pid, int pipe_idx){
  queue* q = &receiver_queue[recv_pid];
  if(q->tail + 1 == q->head) return -1; // queue full
  q->pipe_indices[q->tail] = pipe_idx;
  q->tail++;
  q->tail %= Q_SIZE;
  return 0;
}

int pop_front(int recv_pid){
  queue *q = &receiver_queue[recv_pid];
  if(q->head == q->tail) return -1; // queue empty
  q->pipe_indices[q->head] = 0;
  q->head++;
  q->head %= Q_SIZE;
  return 0;
}

int front(int recv_pid){ // returns the index of pipes where first message for recv_pid is present
  queue *q = &receiver_queue[recv_pid];
  if(q->head == q->tail) return -1; // queue empty
  return q->pipe_indices[q->head];
}

int van_emde_boas_tree[4*NO_OF_PIPES] = {0}; // 1 indexed van emde boas tree
int treeBuilded = 0;
void buildTree(int si, int ss, int se){// van_emde_boas_tree[si] stores the first free index in range ss to se
    treeBuilded = 1;
    if(ss == se){
        van_emde_boas_tree[si] = ss;
        return;
    }
    int mid = (ss + se)/2;
    buildTree(2*si,ss,mid);
    buildTree(2*si+1,mid+1,se);
    van_emde_boas_tree[si] = min(van_emde_boas_tree[2*si],van_emde_boas_tree[2*si+1]);
}

void update(int si, int ss, int se, int ui, int recv_pid){ // updates the ui index upon encountering data with recv_pid
  if(!treeBuilded) buildTree(1,1,NO_OF_PIPES-1);
  if(ss == se){
    if(recv_pid < 0){
      van_emde_boas_tree[si] = ss;
    }else{
      van_emde_boas_tree[si] = (NO_OF_PIPES+1);
    }
    return;
  }
  int mid = (ss+se)/2;
  if(ui <= mid){
    update(2*si,ss,mid,ui,recv_pid);
  }else{
    update(2*si+1,mid+1,se,ui,recv_pid);
  }
  van_emde_boas_tree[si] = min(van_emde_boas_tree[2*si],van_emde_boas_tree[2*si+1]);
}

int get_first_empty_pipe_idx(){
  if(!treeBuilded) buildTree(1,1,NO_OF_PIPES-1);
  int res = van_emde_boas_tree[1];
  return res;
}

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
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

int
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

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// prints the list of system calls that have been invoked
// since last transition to TRACE_ON state with their counts
int sys_print_count(void)
{
  for(int i = 0; i < 22; ++i){
    if(traceSysCall(orderToSysCallNumber(i))){
      int syscall = orderToSysCallNumber(i);
      cprintf("%s %d\n",sysCallName(syscall),traceSysCall(syscall));
    }
  }
  return 0;
}

// toggles the state of Trace between ON and OFF
// returns 1 if current ON and 0 if OFF
int sys_toggle(void)
{
  if(trace == TRACE_OFF){
    trace = TRACE_ON;
    return traceClear();
  }
  trace = TRACE_OFF;
  return 0;
}

// takes in 2 integers and returns their sum
int sys_add(void)
{
  int a,b;
  if(argint(0,&a) < 0 || argint(1,&b) < 0) return -1;
  return (a+b);
}

//prints a list of all the current running processes
int sys_ps(void){
  printAllRunningProcesses();
  return 0;
}

int sys_send(void){
  char * mssg = "$$$$$$$";
  int sender_pid, receiver_pid;
  if(argint(0,&sender_pid) < 0 || argint(1,&receiver_pid) < 0 || argptr(2,&mssg,8) < 0) return -1;
  acquire(&pipes.lock);
  int idx = get_first_empty_pipe_idx();
  if(idx > NO_OF_PIPES){
    release(&pipes.lock);
    return -1;
  }
  update(1,1,NO_OF_PIPES,idx,receiver_pid);
  memmove(pipes.buffer[idx],mssg,8);
  pipes.receiver[idx] = receiver_pid;
  int temp = push(receiver_pid,idx);
  if(temp < 0){
    release(&pipes.lock);
    return -1;
  }
  if(wait_states[receiver_pid] == WAIT_ON){
    wait_states[receiver_pid] = WAIT_OFF;
    wakeup((void*)receiver_pid);
  }
  release(&pipes.lock);
  return 0;
};

int sys_recv(void){
  char * mssg = "$$$$$$$";
  if(argptr(0,&mssg,8)<0) return -1;
  int my_pid = myproc()->pid;
  while(1){
    acquire(&pipes.lock);
    acquire(&receiver_queue[my_pid].lock);
    int top = front(my_pid);
    int temp = pop_front(my_pid);
    if(temp>=0){
      memmove(mssg,pipes.buffer[top],8);
      update(1,1,NO_OF_PIPES,top,-1);
      release(&pipes.lock);
      release(&receiver_queue[my_pid].lock);
      return 0;
    }
    release(&receiver_queue[my_pid].lock);
    wait_states[my_pid] = WAIT_ON;
    sleep((void *)my_pid,&pipes.lock);
    release(&pipes.lock);
  }
  return 0;
};

int sys_send_multi(void){
  int sender_pid; int *recv_pid; char * mssg = "$$$$$$$";
  
  if(argint(0,&sender_pid) < 0 || argptr(1, (char**)&recv_pid, 8*sizeof(int)) < 0 || argptr(2,&mssg, 8) < 0){
    return -1;
  }
  for(int i = 0; i < 8; ++i){
    acquire(&pipes.lock);
    int receiver_pid = recv_pid[i];
    int idx = get_first_empty_pipe_idx();
    if(idx > NO_OF_PIPES){
      release(&pipes.lock);
      return -1;
    }
    update(1,1,NO_OF_PIPES,idx,receiver_pid);
    memmove(pipes.buffer[idx],mssg,8);
    pipes.receiver[idx] = receiver_pid;
    int temp = push(receiver_pid,idx);
    if(temp < 0){
      release(&pipes.lock);
      return -1;
    }
    if(wait_states[receiver_pid] == WAIT_ON){
      wait_states[receiver_pid] = WAIT_OFF;
      wakeup((void*)receiver_pid);
    }
    release(&pipes.lock);
  }
  return 0;
};
