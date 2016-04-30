
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <queue>

using namespace std;

typedef unsigned long planeid_t;

typedef struct plane{
  planeid_t id;
  struct timeval rt;
  //pthread_cond_t pcond;
  //pthread_mutex_t plock;
} plane;

pthread_cond_t pcond_l;
pthread_cond_t pcond_d;
pthread_mutex_t plock;
struct timeval now, init;

int simulation_time;
int sleep_time;

queue <plane> lq;
queue <plane> dq;
queue <plane> eq;

void* landing(void* id);
void* departing(void* id);
void* act(void* dummy);
int pthread_sleep(int seconds);

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
  
  pthread_t acthread;
  pthread_t *threads = new pthread_t[simulation_time/sleep_time];
  //  plane *planes = new plane[simulation_time/sleep_time];
  
  pthread_create(&acthread, NULL, act,(void*) NULL);
  
  while (now.tv_sec <= init.tv_sec + simulation_time) {
    //    printf("now: %d\n", now.tv_sec);

    srand(time(NULL));
    int res = rand() % 100;
    //Emergency
    
    int tdelta = now.tv_sec - init.tv_sec;   
    printf("tdelta: %d\n", tdelta);
    if (tdelta / sleep_time == 40) {//emergency landing      
      plane p;
      p.rt = now;
      eq.push(p);          
    } 
    else if (res <= 50) { //landing      
      printf("%s%d\n\n", "Landing plane with id", plane_id);
      pthread_create(&threads[plane_id], NULL, landing, (void*)plane_id);
      plane_id++;
    } 
    else if (res <= 80) { //departure
      printf("%s%d\n\n", "Departing plane with id", plane_id);
      pthread_create(&threads[plane_id], NULL, departing, (void*)plane_id);
      plane_id++;
    }
    else printf("Nothing happened\n\n");
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


void* act(void *dummy)
{
  while (now.tv_sec <= init.tv_sec + simulation_time) {        
    printf("lq size:%d\n", lq.size());
    printf("dq size: %d\n", dq.size());
    printf("eq size: %d\n", eq.size());
    //emergency landing
    if (!eq.empty()) {
      printf("Emergency landing occured\n\n");
      eq.pop();     
    }    
    else if(!lq.empty() //condition 2.a
       && dq.size() < 3 //condition 2.b
       &&  !(!dq.empty() && ((now.tv_sec - dq.front().rt.tv_sec) >= 10*sleep_time))//condition 2.c
       || lq.size() > dq.size() //a 4th condition to disallow landing starvation by disallowing a longer landing queue than 
       ) {
      plane p = lq.front();
      printf("Signaling for landing %d\n\n", p.id );
      lq.pop();
      pthread_cond_signal(&pcond_l);          
    }
    
    else if(!dq.empty()) {
      
      plane p = dq.front();
      printf("Signaling for departing %d\n\n", p.id );
      dq.pop();
      pthread_cond_signal(&pcond_d);
    } else printf("no act\n\n");
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
  
  pthread_mutex_lock(&plock); 
  //printf("I locked for land: %d\n\n", p.id);
  //printf("Pushing to queue %d \n", p.id);
  lq.push(p);
  //  printf("Pushed to queue to land %d --  \n\n", p.id);//, lq.back().id);
  pthread_cond_wait(&pcond_l, &plock);
  // pthread_cond_init(&(p.pcond), NULL);
  //pthread_cond_wait(&(p.pcond), &plock);
  printf("Eheeey: %ld\n\n", p.id);
  //pthread_cond_destroy(&(p.pcond));
  pthread_mutex_unlock(&plock);
  pthread_exit(NULL);
}

void* departing(void* id)
{
  plane p; 
  p.id = (planeid_t)id;
  gettimeofday(&(p.rt), NULL);  
  pthread_mutex_lock(&plock);
  //printf("I locked depart: %d\n\n", p.id);
  dq.push(p);
  //printf("Pushed to queue depart  %d --  \n\n", p.id);//lq.back().id);
  pthread_cond_wait(&pcond_d, &plock);
  //  printf("Pushing to queue %d \n\n", p.id);
  //pthread_cond_init(&(p.pcond), NULL);
  //pthread_cond_wait(&(p.pcond), &plock);
  printf("Alooha: %ld\n\n", p.id);
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

