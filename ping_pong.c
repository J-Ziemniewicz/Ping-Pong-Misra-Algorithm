
// Author: Wes Kendall
// Copyright 2011 www.mpitutorial.com
// This code is provided freely with the tutorials on mpitutorial.com. Feel
// free to modify it for your own use. Any distribution of the code must
// either provide a link to www.mpitutorial.com or keep this header intact.
//
// Example using MPI_Send and MPI_Recv to pass a message around in a ring.
//
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define PING_MODE 1
#define PONG_MODE 2
#define DEBUG true
#define MSG_SIZE 1
#define STARTING_HOST 0

int debug = 0;

int ping = 1;
int pong = -1;
int m = 0;

int my_id = 1;
int nproc;
int nextHost;

bool critical_section = false;

pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wait_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wait_conditional = PTHREAD_COND_INITIALIZER;





void regenerate(int val) {
	ping = abs(val);
	pong = -ping;
}

void incarnate(int val) {
	ping = abs(val) + 1;
	pong = -ping;
}

void sendToken(int val, int type){

  m = val;
  
  if(type == PING_MODE){
    if(debug){
        printf("Sending PING: %d",val);
    }
    MPI_Send(&val, MSG_SIZE, MPI_INT, nextHost, PING_MODE, MPI_COMM_WORLD);
  }
  else if(type == PONG_MODE){
    if(debug){
        printf("Sending PONG: %d",val);
    }
    MPI_Send(&val, MSG_SIZE, MPI_INT, nextHost, PONG_MODE, MPI_COMM_WORLD);
  }
}

// TODO: Write handling incoming message (with mutex)
void *receive_thread()
{
    printf("%d: Zaczynam wątek odbierający\n", my_id);
    
   
    while (1)
    {
      int msg[MSG_SIZE];
      MPI_Status status;
      int MSG_SIZE;      
      MPI_Recv(&msg, MSG_SIZE, MPI_INT,MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,&status);
      MPI_Get_count( &status, MPI_INT, &size);
      
      if(status.MPI_TAG == PING_MODE){
        if(!(msg < abs(m))){
          if(debug){
            printf("[Thread %d] PING received: %d\n",my_id,msg);
          }
          pthread_mutex_lock(&main_mutex);
          if(m == msg){
            regenerate(msg);
            if(debug){
              printf("[Thread %d] PONG REGENERATE\n[Thread %d] New PONG value %d",my_id,my_id,pong);
            }
            sendToken(pong,PONG_MODE);
          }
          else {
            if (m < msg){
              regenerate(msg);
            }
          }
          pthread_mutex_unlock(&main_mutex);
        }
        else {
          if(debug){
            printf("[Thread %d] Old PING\n",my_id);
          }
        }  
      }
      else if(status.MPI_TAG == PONG_MODE){
        if(!(msg < abs(m))){
          if(debug){
            printf("[Thread %d] PONG received: %d\n",my_id,msg);
          }
          pthread_mutex_lock(&main_mutex);
          // PING and PONG meets
          if(critical_section){
            incarnate(msg);
          }
          else if(m == msg){
            regenerate(msg);
            if(debug){
              printf("[Thread %d] PING REGENERATE\n[Thread %d] New PING value %d",my_id,my_id,pong);
            }
            sendToken(ping,PING_MODE);
          }
          else {
            if (m < msg){
              regenerate(msg);
            }
          }
          pthread_mutex_unlock(&main_mutex);
        }
        else {
          if(debug){
            printf("[Thread %d] Old PONG\n",my_id);
          }
        }
      }

      pthread_cond_signal(&wait_conditional);

    }

    return 0;
}




int main(int argc, char** argv) {

  int run_mode;
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
            fprintf(stderr, "Usage: %s [-t mode] \n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

  int temp;
  MPI_Init_thread(&argc, &argv, 3, &temp);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc );
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id );
  printf("Sync level: %d\n", temp);
  MPI_Barrier(MPI_COMM_WORLD);

  nextHost = (my_id+1)%nproc;

  pthread_t thread;
  rc = pthread_create(&thread, NULL, receive_thread, NULL);
  if(rc){
    fprintf(stderr,"Receiving thread failed to start...\nExiting...\n");
    MPI_Finalize();
  }

  sleep(1);
  if(debug){
    printf("%d: Starting, PID: %d\n",my_id, getpid());
  };

  if(my_id == STARTING_HOST){
    // sending PING
    sendToken(ping,PING_MODE);
    // Sending PONG
    sendToken(pong,PONG_MODE);
  }

  while(1){
    pthread_mutex_lock(&wait_mutex);
    while(!critical_section){
      pthread_cond_wait(&wait_conditional,&wait_mutex);
    }

    if(debug){
      printf("Thread %d entered critical section\n",my_id);
    }
    sleep(1);
    if(debug){
      printf("Thread %d left critical section\n",my_id);
    }

    pthread_mutex_lock(&main_mutex);
    critical_section = false;
    pthread_mutex_unlock(&main_mutex);
    pthread_mutex_unlock(&wait_mutex);
  }

  pthread_join(thread, NULL);

  MPI_Finalize();

  return 0;
}