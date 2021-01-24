#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define PING_MODE 1
#define PONG_MODE 2
#define DEBUG 1
#define MSG_SIZE 1

// configurable_values
#define STARTING_HOST 0
#define PING_DELAY 50000
#define PONG_DELAY 1000000

// Debug mode default disabled
int debug = 0;

// Initial ping, pong and m values
int ping = 1;
int pong = -1;
int m = 0;

// Host variables
int my_id = 1;
int nproc;
int nextHost;

int critical_section = 0;

pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wait_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wait_conditional = PTHREAD_COND_INITIALIZER;



// void regenerate(int val) {
// 	ping = abs(val);
// 	pong = -ping;
// }

void incarnate(int val) {
	// ping = (abs(val) + 1)%(nproc+1);
  ping = abs(val)+1;
	pong = -ping;
}

void sendToken(int val, int type){

  m = val;
  
  if(type == PING_MODE){
    if(debug){
        printf("[Thread %d] Sending PING: %d\n",my_id, val);
    }
    usleep(PING_DELAY);
    MPI_Send(&val, MSG_SIZE, MPI_INT, nextHost, PING_MODE, MPI_COMM_WORLD);
  }
  else if(type == PONG_MODE){
    if(debug){
        printf("[Thread %d] Sending PONG: %d\n",my_id, val);
    }
    usleep(PONG_DELAY);
    MPI_Send(&val, MSG_SIZE, MPI_INT, nextHost, PONG_MODE, MPI_COMM_WORLD);
  }
}


void *receive_thread()
{
    // printf("%d: Zaczynam wątek odbierający\n", my_id);
    
    while (1)
    {
      int msg;
      MPI_Status status;
      int size;      
      MPI_Recv(&msg, MSG_SIZE, MPI_INT,MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,&status);
      MPI_Get_count( &status, MPI_INT, &size);
      
      if(status.MPI_TAG == PING_MODE){
        if(!(abs(msg) < abs(m))){
          if(debug){
            printf("[Thread %d] PING received: %d\n",my_id,msg);
          }
          pthread_mutex_lock(&main_mutex);
          if(m == msg){
            // instead of regenerate and than increment do incarnate (to simplyfy code)
            incarnate(msg);
            if(debug){
              printf("[Thread %d] PONG REGENERATE\n[Thread %d] New PONG value %d\n",my_id,my_id,pong);
            }
            sendToken(pong,PONG_MODE);
          }
          else {
            ping = msg;
            if(debug){
              printf("[Thread %d] Received token for Critical section %d\n",my_id,ping);
            }
          }
          critical_section = 1;
          pthread_mutex_unlock(&main_mutex);
          pthread_cond_signal(&wait_conditional);
        }
        else {
          if(debug){
            printf("[Thread %d] Old PING %d\n",my_id,msg);
          }
        }  
      }
      else if(status.MPI_TAG == PONG_MODE){
        if(!(abs(msg) < abs(m))){
          if(debug){
            printf("[Thread %d] PONG received: %d\n",my_id,msg);
          }
          pthread_mutex_lock(&main_mutex);
          // PING and PONG meets
          if(critical_section){
            incarnate(msg);
            if(debug){
              printf("[Thread %d] Incarnation: %d\n",my_id,ping);
            }
            sendToken(pong,PONG_MODE);
            pthread_mutex_unlock(&main_mutex);
          }
          else if(m == msg){
            // instead of regenerate and than increment do incarnate (to simplyfy code)
            incarnate(msg);
            if(debug){
              printf("[Thread %d] PING REGENERATE\n[Thread %d] New PING value %d\n",my_id,my_id,ping);
            }
            critical_section = 1;
            sendToken(pong,PONG_MODE);
            pthread_mutex_unlock(&main_mutex);
            pthread_cond_signal(&wait_conditional);
          }
          else{
            pong = msg;
            sendToken(pong,PONG_MODE);
            pthread_mutex_unlock(&main_mutex);
          }
          
        }
        else {
          if(debug){
            printf("[Thread %d] Old PONG %d\n",my_id,msg);
          }
        }
      }

      pthread_cond_signal(&wait_conditional);

    }

    return 0;
}




int main(int argc, char** argv) {

  int run_mode =0;
  int opt; 
      
  while ((opt = getopt(argc, argv, "dt:")) != -1) {
        switch (opt) {
        case 'd':
            debug = DEBUG;
            break;
        case 't':
            if(strncmp(optarg,"ping",4)==0){
              run_mode = PING_MODE; 
            }
            else if(strncmp(optarg,"pong",4)==0){
              run_mode = PONG_MODE;
            }
            else{
              fprintf(stderr,"Wrong parameters \n");
              exit(EXIT_FAILURE);
            }
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-t mode] -d \n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

  int temp;
  MPI_Init_thread(&argc, &argv, 3, &temp);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc );
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id );
  
  MPI_Barrier(MPI_COMM_WORLD);

  nextHost = (my_id+1)%nproc;

  pthread_t thread;
  int rc = pthread_create(&thread, NULL, receive_thread, NULL);
  if(rc){
    fprintf(stderr,"Receiving thread failed to start...\nExiting...\n");
    MPI_Finalize();
  }

  printf("Sync level: %d\n", temp);
  MPI_Barrier(MPI_COMM_WORLD);
  sleep(1);
  if(debug){
    printf("Starting thread : %d\n",my_id);
  };

  if(my_id == STARTING_HOST){
    // Sending PING
    if(!(run_mode == 1)){
      sendToken(ping,PING_MODE);
    }
    if(!(run_mode == 2)){
    // Sending PONG
      sendToken(pong,PONG_MODE);
    }
  }

  while(1){
    
      pthread_mutex_lock(&wait_mutex);
      while(!critical_section){
        pthread_cond_wait(&wait_conditional,&wait_mutex);
      }
      if(debug){
      printf("Thread %d entered critical section\n",my_id);
      }
      sleep(2);
      if(debug){
        printf("Thread %d left critical section\n",my_id);
      }

      pthread_mutex_lock(&main_mutex);
      critical_section = 0;
      sendToken(ping,PING_MODE);
      pthread_mutex_unlock(&main_mutex);
      pthread_mutex_unlock(&wait_mutex);
    
  }

  pthread_join(thread, NULL);

  MPI_Finalize();

  return 0;
}