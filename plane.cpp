
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <queue>
#include <vector>
#include <iostream>

using namespace std;

typedef unsigned long planeid_t;

typedef struct plane{
  planeid_t id;
  struct timeval rt;
  char status;
  struct timeval rnt;
  //pthread_cond_t pcond;
  //pthread_mutex_t plock;
} plane;


pthread_cond_t pcond_l;
pthread_cond_t pcond_d;
pthread_mutex_t plock;
struct timeval now, init;

int simulation_time;
int sleep_time;
int cnt;
int prob;
int nth;

queue <plane> lq;
queue <plane> dq;
queue <plane> eq;

queue <planeid_t> lq_strs;
queue <planeid_t> dq_strs;
//vector <log> logs;

void* landing(void* id);
void* departing(void* id);
void* act(void* dummy);

int pthread_sleep(int seconds);
void print_queues();
//inline int cmp_times(struct timeval t1, struct timeval t2){ return t1.tv_sec - t2.tv_sec; }

int main(int argc, char *argv[])
{
  simulation_time = 100;
  sleep_time = 2;
  pthread_mutex_init(&plock, NULL);
  pthread_cond_init(&pcond_d, NULL);
  pthread_cond_init(&pcond_l, NULL);

  id_t plane_id = 0;
  gettimeofday(&init, NULL);
  gettimeofday(&now, NULL);
  prob = 50;
  nth = 0;

  for(int ct=1; ct<argc; ct++){
    if(strcmp(argv[ct],"-s")==0){
      simulation_time = atoi(argv[ct]);
    }else if(strcmp(argv[ct],"-p")==0){
      prob = atoi(argv[ct])*100;
    }else if(strcmp(argv[ct],"-n")==0){
      nth = atoi(argv[ct]);
    }
  }

  pthread_t acthread;
  pthread_t *threads = new pthread_t[simulation_time/sleep_time];
  //  plane *planes = new plane[simulation_time/sleep_time];

  pthread_create(&acthread, NULL, act,(void*) NULL);

  while (now.tv_sec <= init.tv_sec + simulation_time) {
    //    printf("now: %d\n", now.tv_sec);

    //pthread_mutex_lock(&plock);
    //for (int i = 0; i < lq.size(); i++) {

      //}
    //pthread_mutex_unlock(&plock);


    srand(time(NULL));
    int res = rand() % 100;
    //Emergency

    int tdelta = now.tv_sec - init.tv_sec;
    if (tdelta%(2*sleep_time) == 0) cnt++;


    //printf("tdelta: %d\n", tdelta);
    if (tdelta / sleep_time == 40) {//emergency landing
      pthread_mutex_lock(&plock);
      plane p;
      p.rt = now;
      eq.push(p);
      plane_id++;

      if((now.tv_sec - init.tv_sec) >= nth)
        print_queues();

      pthread_mutex_unlock(&plock);
    }
    else if (res <= prob) { //landing
      //printf("%s%d\n\n", "Landing plane with id", plane_id);
      pthread_create(&threads[plane_id], NULL, landing, (void*)plane_id);
      plane_id++;
    }
    else if (res <= 100) { //departure
      //printf("%s%d\n\n", "Departing plane with id", plane_id);
      pthread_create(&threads[plane_id], NULL, departing, (void*)plane_id);
      plane_id++;
    } else {
      pthread_mutex_lock(&plock);
      if((now.tv_sec - init.tv_sec) >= nth)
        print_queues();
        
      pthread_mutex_unlock(&plock);
    }


    //else printf("Nothing happened\n\n");

    pthread_sleep(sleep_time);
    gettimeofday(&now, NULL);
  }

  //pthread_join(acthread, NULL);
  //for (int i = 0; i < plane_id; i++) {
  //  pthread_join(threads[i], NULL);
  //}

  pthread_mutex_destroy(&plock);
  pthread_cond_destroy(&pcond_l);
  pthread_cond_destroy(&pcond_d);
  delete [] threads;
  return 0;
}

inline void log_print(plane p) { printf("%20d %20c %20d %20d %20d\n", p.id, p.status, p.rt.tv_sec - init.tv_sec,
					p.rnt.tv_sec - init.tv_sec, p.rnt.tv_sec - p.rt.tv_sec); }

inline void log_title() { printf("%20s %20s %20s %20s %20s\n%s\n", "PlaneID", "Status",
					"Request Time", "Runway Time", "Turnaround Time",
					"---------------------------------------------------------------------------------------------------------------------------");
}

void print_queues()
{
  printf("On Air: ");
  for (int i = 0; i < lq.size(); i++) {
    planeid_t id = lq_strs.front();
    printf("%d,", id);
    lq_strs.pop();
    lq_strs.push(id);
  }printf("\n");
  printf("On Pist: ");
  for (int i = 0; i < dq.size(); i++) {
    planeid_t id = dq_strs.front();
    printf("%d,", id);
    dq_strs.pop();
    dq_strs.push(id);
  }printf("\n");
}


void* act(void *dummy)
{
  log_title();
  while (now.tv_sec <= init.tv_sec + simulation_time) {
    //printf("lq size:%d\n", lq.size());
    //printf("dq size: %d\n", dq.size());
    //printf("eq size: %d\n", eq.size());
    //emergency landing
    pthread_mutex_lock(&plock);

    if (!eq.empty()) {
      //printf("Emergency landing occured\n\n");
      eq.pop();
    }
    else if(!lq.empty() //condition 2.a
       && dq.size() < 3 //condition 2.b
       &&  !(!dq.empty() && ((now.tv_sec - dq.front().rt.tv_sec) >= 10*sleep_time))//condition 2.c
       || lq.size() > dq.size() //a 4th condition to prevent landing starvation by disallowing existence of a longer landing queue than departure queue
       ) {
      plane p = lq.front();
      //printf("Signaling for landing %d\n\n", p.id );
      lq.pop();
      lq_strs.pop();
      pthread_cond_signal(&pcond_l);
    }

    else if(!dq.empty()) {

      plane p = dq.front();
      //printf("Signaling for departing %d\n\n", p.id );
      dq.pop();
      dq_strs.pop();
      pthread_cond_signal(&pcond_d);
    } //else printf("no act\n\n");
    pthread_mutex_unlock(&plock);
    pthread_sleep(2*sleep_time);
    //gettimeofday(&now, NULL);
  }
  pthread_exit(NULL);
}


void* landing(void* id)
{
  plane p;
  p.id = (planeid_t)id;
  gettimeofday(&(p.rt), NULL);
  p.status = 'L';
  pthread_mutex_lock(&plock);
  //printf("I locked for land: %d\n\n", p.id);
  //printf("Pushing to queue %d \n", p.id);
  lq.push(p);
  lq_strs.push(p.id);
  //  printf("Pushed to queue to land %d --  \n\n", p.id);//, lq.back().id);
  if((now.tv_sec - init.tv_sec) >= nth)
    print_queues();

  pthread_cond_wait(&pcond_l, &plock);
  gettimeofday(&(p.rnt), NULL);
  //printf("here\n");
  log_print(p);

  pthread_mutex_unlock(&plock);
  // pthread_cond_init(&(p.pcond), NULL);
  //pthread_cond_wait(&(p.pcond), &plock);
  //printf("Eheeey: %ld\n\n", p.id);
  //pthread_cond_destroy(&(p.pcond));

  pthread_exit(NULL);
}

void* departing(void* id)
{
  plane p;
  p.id = (planeid_t)id;
  gettimeofday(&(p.rt), NULL);
  p.status = 'D';
  pthread_mutex_lock(&plock);
  if(tdelta>=nth)
    print_queues();
  //printf("I locked depart: %d\n\n", p.id);
  dq.push(p);
  dq_strs.push(p.id);
  //printf("Pushed to queue depart  %d --  \n\n", p.id);//lq.back().id);
  //cout << dq_strs << "\n";

  pthread_cond_wait(&pcond_d, &plock);
  gettimeofday(&(p.rnt), NULL);
  //  printf("there\n");
  log_print(p);
  //  printf("Pushing to queue %d \n\n", p.id);
  //pthread_cond_init(&(p.pcond), NULL);
  //pthread_cond_wait(&(p.pcond), &plock);
  //printf("Alooha: %ld\n\n", p.id);
  //pthread_cond_destroy(&(p.pcond));
  pthread_mutex_unlock(&plock);
  pthread_exit(NULL);
}

 /******************************************************************************
  pthread_sleep takes an integer number of seconds to pause the current thread
  original by Yingwu Zhu
  updated by Muhammed Nufail Farooqi
 *****************************************************************************/
int pthread_sleep (int seconds)
{
   pthread_mutex_t mutex;
   pthread_cond_t conditionvar;
   struct timespec timetoexpire;
   if(pthread_mutex_init(&mutex,NULL))
    {
      return -1;
    }
   if(pthread_cond_init(&conditionvar,NULL))
    {
      return -1;
    }
   struct timeval tp;
   //When to expire is an absolute time, so get the current time and add //it to our delay time
   gettimeofday(&tp, NULL);
   timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;

   pthread_mutex_lock (&mutex);
   int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
   pthread_mutex_unlock (&mutex);
   pthread_mutex_destroy(&mutex);
   pthread_cond_destroy(&conditionvar);

   //Upon successful completion, a value of zero shall be returned
   return res;
}
