#include "ReadyQueue.c"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <sys/wait.h>

//This function generates a random variable on exponential distribution.
double exponential_dist(double lamb){
    double u;
    u = rand() / (RAND_MAX + 1.0);
    return -log(1- u) / lamb;
}

//Ready queue that will be populated with CPU Bursts
ReadyQueue *rq;

int N_thread;   // Thread count.
int Bcount; // CPU bursts that a single thread will produce.
double minB;    // Minimum value that a CPU burst length can get.
double avgB;    // Average value of CPU bursts lengths.
double minA;    // Minimum value that an interarrival time can get.
double avgA;    // Average value of interarrival time for a thread.
char *ALG;      // Scheduling Algorithm

double *total_waitings; // Total waiting times of threads.

double *VRUNTIME; // Virtual runtimes of all threads.

// If the burst info is read from file, this matrix will be used.
double ***burst_info;

// ... and this flag will be set to 1.
int read_file = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;    // Mutex variable
pthread_cond_t cv;                                    // Condition variable

//Thread function. Each thread will populate the ready queue with CPU bursts.
void *W_thread(void *ptr) {

  // Thread indexes are passed as parameters.
  int thread_index = *((int *) ptr);

  /*Each thread will count the bursts that they produced. While
  burst_index < Bcound, they will produce a CPU burst.*/
  int burst_index = 1;

  // Generates (Bcount) CPU bursts for each thread.
  while (burst_index <= Bcount) {

    double interarrival_time;
    if (!read_file) {
      /* Generates inter arrival time randomly for each thread with
      exponential distribution.*/
      do {
        interarrival_time = exponential_dist(1 / avgA);
      } while (interarrival_time < minA);
    }
    else {
      // Reads inter arrival time from file.
      interarrival_time = burst_info[thread_index - 1][burst_index - 1][0];
    }

    double length;
    if (!read_file) {
      // Generates CPU burst length randomly with exponential distribution.
      do {
        length = exponential_dist(1 / avgB);
      } while (length < minB);
    }
    else {
      // Reads CPU burst lengths from file.
      length = burst_info[thread_index - 1][burst_index - 1][1];
    }

    // Sleep a thread before it generates its burst.
    usleep(interarrival_time * 1000);

    pthread_mutex_lock(&mutex);

    /* CRITICAL SECTION */
    /* Each thread will try to access the ready queue, therefore, synchronization
    should be done for the shared data ReadyQueue *rq */

    // Timestamp of the time CPU burst has been generated.
    struct timeval timestamp;
    gettimeofday(&timestamp, NULL);

    // Insert the burst to the ready queue.
    insert(rq, thread_index, burst_index, length, VRUNTIME[thread_index - 1], timestamp);

    /* Signal the S thread that there is an insertion and the queue
    is not empty.*/
    pthread_cond_signal(&cv);

    // Printing information about the burst.
    printf("Inserted to RQ:\n");
    printf("Thread index: %d\n", thread_index);
    printf("Burst index: %d\n", burst_index);
    printf("Burst length: %f\n", length);
    printf("VRUNTIME: %f\n", VRUNTIME[thread_index - 1]);
    printf("Arrival: %ld seconds, %f milliseconds\n",
            timestamp.tv_sec, (double) timestamp.tv_usec / 1000);
    printf("\n");

    /* END OF CRITICAL SECTION */

    pthread_mutex_unlock(&mutex);

    // Burst generated, increment the burst_index (bursts generated) by 1
    burst_index++;
  }

  // Deallocate the thread_index which is passed through heap.
  free(ptr);

  // Terminate the thread.
  pthread_exit(0);
}

int executed = 0;   //Executed CPU burst cound.

/* This is the CPU thread which will simulate execution of the bursts. */
void *S_thread(void *ptr) {

  while (1) {

    /* If there are no bursts to shedule, wait on the conditional variable
    until a signal is sent after a thread produces a burst. */
    if (isEmpty(rq)) {
      pthread_cond_wait(&cv, &mutex);
    }

    struct CPUBurst *next;

    /* Index of the next CPU burst that will be executed will be
    determined according to algorithms. */
    int next_index;

    if (strcmp(ALG, "FCFS") == 0) {
      next_index = 1;
    }
    else if (strcmp(ALG, "SJF") == 0) {
      next_index = selectLowest(rq, 0, N_thread);
    }
    else if (strcmp(ALG, "PRIO") == 0) {
      next_index = selectLowest(rq, 1, N_thread);
    }
    else if (strcmp(ALG, "VRUNTIME") == 0) {
      next_index = selectLowest(rq, 2, N_thread);
    }

    // Retrieving the next CPU burst.
    next = retrieve(rq, next_index);

    // Updating the VRUNTIME of the thread that generated the CPU burst.
    VRUNTIME[next->thread_index - 1] += next->length * (0.7 + 0.3 * next->thread_index);

    // Getting the exact time when a process starts to be executed.
    struct timeval start_exec;
    gettimeofday(&start_exec, NULL);

    /* Calculation of the elapsed time between a process arrives and starts
    to be executed by the CPU. */
    double elapsed = 0;
    elapsed += (start_exec.tv_sec - next->wall_clock_time.tv_sec) * 1000;
    if (start_exec.tv_usec < next->wall_clock_time.tv_usec) {
      elapsed += (start_exec.tv_usec + (1000000 - next->wall_clock_time.tv_usec)) / 1000;
    }
    else {
      elapsed += (start_exec.tv_usec - next->wall_clock_time.tv_usec) / 1000;
    }

    /* Increment the total waiting time of the thread by elapsed time.
    Will be divided with total burst count to find the average waiting time. */
    total_waitings[next->thread_index - 1] += elapsed;

    // Simulating the CPU burst by sleeping for the length of the burst.
    usleep(next->length * 1000);

    // Getting the finish time of the burst.
    struct timeval finito;
    gettimeofday(&finito, NULL);

    // Printing the info about burst.
    printf("Executed.\n");
    printf("Thread index: %d\n", next->thread_index);
    printf("Burst index: %d\n", next->burst_index);
    printf("Burst length: %f\n", next->length);
    printf("VRUNTIME: %f\n", next->thread_vruntime);
    printf("Arrival: %ld seconds, %f milliseconds\n",
            next->wall_clock_time.tv_sec, (double) next->wall_clock_time.tv_usec / 1000);
    printf("Start: %ld seconds, %f milliseconds\n",
            start_exec.tv_sec, (double) start_exec.tv_usec / 1000);
    printf("Finito: %ld seconds, %f milliseconds\n",
            finito.tv_sec, (double) finito.tv_usec / 1000);
    printf("Waiting time for burst %d of thread %d: %f\n", next->burst_index, next->thread_index, elapsed);
    printf("\n");

    // Removing the executed CPU burst from the ready queue.
    delete(rq, next_index);

    // Incrementing the executed CPU burst count by 1.
    executed++;

    // If there are no more CPU bursts to execute:
    if (executed == N_thread * Bcount) {
      break;
    }
  }
  pthread_exit(0);
}

int main(int argc, char *argv[]) {

  //This is to randomize the rand() function.
  srand (time(0));

  //Getting the parameters from the user.
  N_thread = atoi(argv[1]);
  if (atoi(argv[2]) == 0) { // Means that burst info will be read from file.
    ALG = argv[2];  // Scheduling Algorithm

    /* A 2D array is being created in the heap which will include the
    interarrival time and each CPU burst lengths. */
    burst_info = malloc(sizeof(double **) * N_thread);

    // Reading files for each W_thread.
    for (int i = 0; i < N_thread; i++) {
      Bcount = 0;
      char filename[20];
      strcpy(filename, argv[4]);  // Get file prefix.
      char postfix[8];
      sprintf(postfix, "-%d.txt", i + 1);
      strcat(filename, postfix);  // Append file prefix with -(thread_index).txt

      // Determine BCount by counting the lines of the files.
      FILE *f = fopen(filename, "r");

      while(!feof(f))
      {
        int ch = fgetc(f);
        if(ch == '\n')
        {
          Bcount++;
        }
      }

      fclose(f);

      // Reopen the file to read the burst info.
      f = fopen(filename, "r");

      burst_info[i] = malloc(sizeof(double*) * Bcount);

      for (int j = 0; j < Bcount; j++) {
        burst_info[i][j] = malloc(sizeof(double) * 2);

        // Read the values from the files.
        double interarrival_time;
        fscanf(f, "%lf", &interarrival_time);
        burst_info[i][j][0] = interarrival_time;

        double val;
        fscanf(f, "%lf", &val);
        burst_info[i][j][1] = val;
      }
    }
    read_file = 1;    // A flag to indicate whether a file is read or not.
  }
  else {
    Bcount = atoi(argv[2]); // CPU bursts that a single thread will produce.
    minB = atoi(argv[3]);   // Minimum value that a CPU burst length can get.
    avgB = atoi(argv[4]);   // Average value of CPU bursts lengths.
    minA = atoi(argv[5]);   // Minimum value that an interarrival time can get.
    avgA = atoi(argv[6]);   // Average value of interarrival time for a thread.
    ALG = argv[7];          // Scheduling Algorithm
  }

  total_waitings = malloc(sizeof(double) * N_thread);
  double average_waitings[N_thread];   // Average waiting time.
  double total_waiting = 0;                 // Total waiting time of

  //Creating thread_ids
  pthread_t W_tids[N_thread];
  pthread_t S_tid;

  //Creating the ReadyQueue
  rq = createQueue();

  //Initializing the VRUNTIMEs of threads to 0.
  VRUNTIME = malloc(sizeof(double) * N_thread);
  for (int i = 0; i < N_thread; i++) {
    VRUNTIME[i] = 0;
  }

  //Creating the S_thread, which will simulate CPU Scheduling.
  pthread_create(&S_tid, NULL, &S_thread, NULL);

  //Creating the W_threads, which will populate the ready queue.
  for (int i = 0; i < N_thread; i++) {
    int *arg = malloc(sizeof(int *));
    *arg = i + 1;
    pthread_create(&W_tids[i], NULL, &W_thread, (void *) arg);
  }

  //Joining the threads with the main thread, after threads terminate.
  for (int i = 0; i < N_thread; i++) {
    pthread_join(W_tids[i], NULL);
  }

  pthread_join(S_tid, NULL);

  if (isEmpty(rq)) {
    printf("Ready queue empty, scheduling complete!\n");
    for (int i = 0; i < N_thread; i++) {
      average_waitings[i] = total_waitings[i] / Bcount;
      printf("Average waiting time of thread %d: %f\n", i + 1, average_waitings[i]);
    }

    // Calculation of the average waiting time.
    for (int i = 0; i < N_thread; i++) {
      total_waiting += average_waitings[i];
    }
    printf("Average waiting time: %f\n", total_waiting / N_thread);
  }
  else {
    printf("Something went wrong.\n");
  }

  return 0;
}
