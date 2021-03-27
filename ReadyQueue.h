#include <sys/time.h>

typedef struct CPUBurst {
 int thread_index;
 int burst_index;
 double length;
 double thread_vruntime;
 struct timeval wall_clock_time;
 struct CPUBurst *next;
} CPUBurst;

typedef struct {
  struct CPUBurst *head;
  int size;
} ReadyQueue;

ReadyQueue *createQueue();
void destruct(ReadyQueue *rq);

int isEmpty(ReadyQueue *rq);
int getLength(ReadyQueue *rq);

struct CPUBurst *retrieve(ReadyQueue *rq, int index);

struct CPUBurst *removeFromQueue(ReadyQueue *rq, int index);
void delete(ReadyQueue *rq, int index);
void insert(ReadyQueue *rq, int thread_index, int burst_index, double length, double thread_vruntime, struct timeval wall_clock_time);

int selectLowest(ReadyQueue *rq, int mode, int N_thread);
struct CPUBurst *find(ReadyQueue *rq, int index);
