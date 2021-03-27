#include "ReadyQueue.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

ReadyQueue *createQueue() {
  ReadyQueue *rq = (ReadyQueue *) malloc(sizeof(ReadyQueue));
  rq->head = 0;
  rq->size = 0;
  return rq;
}

void destruct(ReadyQueue *rq){
   while (!isEmpty(rq))
      delete(rq, 1);
}

int isEmpty(ReadyQueue *rq) {
   return rq->size == 0;
}

int getLength(ReadyQueue *rq) {
   return rq->size;
}

struct CPUBurst *find(ReadyQueue *rq, int index) {
   if ( (index < 1) || (index > getLength(rq)) )
      return NULL;

   else {
      struct CPUBurst *cur = rq->head;
      for (int skip = 1; skip < index; ++skip)
         cur = cur->next;
      return cur;
   }
}

struct CPUBurst *retrieve(ReadyQueue *rq, int index) {
   if ((index < 1) || (index > getLength(rq)))
      return NULL;

   // get pointer to node, then data in node
   struct CPUBurst *burst = find(rq, index);
   return burst;
}

struct CPUBurst *removeFromQueue(ReadyQueue *rq, int index) {
  struct CPUBurst *cur;

   if ((index < 1) || (index > getLength(rq)))
      return NULL;

   --rq->size;
   if (index == 1){
      cur = rq->head;
      rq->head = rq->head->next;
   }
   else {
      struct CPUBurst *prev = find(rq, index-1);
      cur = prev->next;
      prev->next = cur->next;
   }
   cur->next = NULL;
   return cur;
}

void delete(ReadyQueue *rq, int index) {
  struct CPUBurst *deleted = removeFromQueue(rq, index);
  free(deleted);
  deleted = NULL;
}

void insert(ReadyQueue *rq, int thread_index, int burst_index, double length, double thread_vruntime, struct timeval wall_clock_time) {
  struct CPUBurst *newPtr = (struct CPUBurst *) malloc(sizeof(struct CPUBurst));

  newPtr->thread_index = thread_index;
  newPtr->burst_index = burst_index;
  newPtr->length = length;
  newPtr->thread_vruntime = thread_vruntime;
  newPtr->wall_clock_time = wall_clock_time;
  newPtr->next = NULL;

  if (isEmpty(rq)) {
    rq->head = newPtr;
  }
  else {
    struct CPUBurst *prev = find(rq, rq->size);
    newPtr->next = prev->next;
    prev->next = newPtr;
  }

  rq->size++;
}

/* This function selects the next CPU burst index according to scheduling
algorithms. One algorithm is SRJ (Shortest Job First), other algorithm is
PRIO.
    mode = 0 denotes SRJ algorithm
    mode = 1 denotes PRIO algorithm
This function returns the index of the next CPU burst that will be chosen. */
int selectLowest(ReadyQueue *rq, int mode, int N_thread) {
  CPUBurst *cur = rq->head;
  if (cur == NULL) {
    return -1;
  }
  else {
    int flags[N_thread];
    for (int i = 0; i < N_thread; i++) {
      flags[i] = 0;
    }
    int priority = 1;   //index of the burst which has the priority
    double lowest;      //the lowest value that will be compared among bursts

    /* If mode == 0, SRJ algorithm will be running, therefore, length of
    the bursts will be compared. */
    if (mode == 0) {
      lowest = cur->length;
    }

    /* If mode == 1, PRIO algorithm will be running, therefore, indexes of
    threads will be compared. */
    else if (mode == 1) {
      lowest = cur->thread_index;
    }

    /* If mode == 2, VRUNTIME algorithm will be running, therefore,
    VRUNTIMES of the threads will be compared. */
    else if (mode == 2) {
      lowest = cur->thread_vruntime;
    }

    flags[cur->thread_index - 1] = 1;

    cur = cur->next;
    double checked;     //current checked value

    for (int i = 2; cur != NULL; i++) {
      if (flags[cur->thread_index - 1] == 0) {
        flags[cur->thread_index - 1] = 1;
        
        /* If mode == 0, SRJ algorithm will be running, therefore, length of
        the bursts will be compared. */
        if (mode == 0) {
          checked = cur->length;
        }

        /* If mode == 1, PRIO algorithm will be running, therefore, indexes of
        threads will be compared. */
        else if (mode == 1) {
          checked = cur->thread_index;
        }

        /* If mode == 2, VRUNTIME algorithm will be running, therefore,
        VRUNTIMES of the threads will be compared. */
        else if (mode == 2) {
          checked = cur->thread_vruntime;
        }

        // Comparing the current value with the lowest value and doing updates.
        if (checked < lowest) {
          lowest = checked;
          priority = i;
        }
      }
      cur = cur->next;
    }
    return priority;
  }
}
