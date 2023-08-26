#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/mutex_user.h"

struct balance {
  char name[32];
  int amount;
};

struct thread_mutex lock;

volatile int total_balance = 0;

volatile unsigned int delay(unsigned int d) {
  unsigned int i;
  for (i = 0; i < d; i++) {
    __asm volatile("nop" :::);
  }

  return i;
}

volatile int _tid = 0;
int *x[2];
volatile int locked = 0;
int n = 10000;

int *p, *q;

int *smem;

void do_work(void *arg) {
  int i;
  int old;
  int tid;
  tid = _tid++;

  thread_mutex_lock(&lock);
  printf("thread %d started\n", tid);
  thread_mutex_unlock(&lock);

  struct balance *b = (struct balance *)arg;
  // printf( "Starting do_work: s:%s\n", b->name);
  x[tid] = malloc(sizeof(int) * n);

  thread_mutex_lock(&lock);
  printf("thread %d malloced %p\n", tid, x[tid]);
  thread_mutex_unlock(&lock);

  for (int i = 0; i < n; i++) {
    x[tid][i] = i;
  }

  for (i = 0; i < b->amount; i++) {
    old = total_balance;
    delay(100000);
    total_balance = old + 1;
  }

  for (int i = 0; i < n; i++) {
    smem[i]++;
  }

  for(int k=0;k<100;k++) {
    delay(100000);
  }

  *p = 1234;

  thread_mutex_lock(&lock);
  printf("From main q was set: %d\n", *q);
  thread_mutex_unlock(&lock);

  fork();
  for(int k=0;k<100;k++) {
    delay(100000);
  }

  thread_exit();
  return;
}

void empty_task() {
  int tid;
  tid = _tid++;

  thread_mutex_lock(&lock);
  printf("thread %d started\n", tid);
  thread_mutex_unlock(&lock);

  for (int i = 0; i < n; i++) {
    smem[i]++;
  }

  int *x = (int*)malloc(sizeof(int)*n);
  int i = 0;
  while(1){
    x[i]++;
    i = (i + 1) % n;
  }

  // thread_exit();
  return;
}

int main(int argc, char *argv[]) {

  struct balance b1 = {"b1", 3200};
  struct balance b2 = {"b2", 2800};

  thread_mutex_init(&lock);

  void *s1, *s2, *s3;
  int thread1, thread2, r1, r2;

  s1 = malloc(4096); // 4096 is the PGSIZE defined in kernel/riscv.h
  s2 = malloc(4096);

  smem = malloc(sizeof(int) * n);

  p = (int *)malloc(sizeof(int*));
  q = (int *)malloc(sizeof(int*));

  *q = 9876;
  thread1 = thread_create(do_work, (void *)&b1, s1);
  thread2 = thread_create(do_work, (void *)&b2, s2);

  for(int i=0;i<20;i++) {
    s3 = malloc(4096);
    thread_create(empty_task, 0, s3);
  }


  r1 = thread_join(thread1);
  r2 = thread_join(thread2);

  printf("Threads finished: (%d):%d, (%d):%d, shared balance:%d\n", thread1, r1,
         thread2, r2, total_balance);

  printf("value of p was set from thread: %d\n", *p);

  for(int i=0;i<20000;i++) {
    delay(100000);
  }
  exit(0);
}
