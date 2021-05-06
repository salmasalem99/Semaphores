#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#define Bsize 5
int Bfirst=0;
int Blast=0;
int Bcurrentsize=0;
int buffer[Bsize]; //Our buffer
int message;
sem_t mutex; // semaphore to ensure mutual exclusion for message
sem_t mutexforbuffer; //semaphore to ensure mutual exculsion for buffer and all its related variables
sem_t writetobuff; //semaphore to ensure no more than x threads have or had access to write to the buffer
sem_t readfrombuff; //semaphore to ensure that the buffer is not empty upon trying to read from it
void* count(void* arg)
{
    int TN = *(int *)arg; //taking thread number as an argument
    while(1)
    {
        printf("Counter Thread %d: received a message\n", TN);
        printf("Counter Thread %d: waiting to write\n", TN);
        sem_wait(&mutex); //mutex locking message variable
        message++;
        printf("Counter Thread %d: now adding to counter, counter value=%d\n", TN, message);
        sem_post(&mutex); //unlocking message variable
        sleep(rand()%13); //sleep for a random time
    }
}
void* monitor(void* arg)
{
    while(1)
    {
        int tempint=0;
        //waiting on message
        printf("Monitor thread: waiting to read counter\n");
        sem_wait(&mutex); //mutex locking message variable

        //reading message
        tempint=message;
        printf("Monitor thread: reading a counter value of: %d\n", tempint);
        //resetting message to 0
        message=0;
        sem_post(&mutex); //unlocking message variable

        //wait for buffer to have empty space;
        sem_wait(&writetobuff);

        //wait for buffer to be mutually exclusive
        sem_wait(&mutexforbuffer);

        //inserting the read value of my integer into the end of the buffer
        printf("Monitor thread: writing to buffer at position: %d\n", Blast);
        buffer[Blast]=tempint;
        Blast=(Blast+1)%Bsize;
        Bcurrentsize++;

        //check if the buffer is full
        if(Bcurrentsize==Bsize){
            printf("Monitor thread: Buffer full!!\n");
        }

        //exiting mutual exclusion of buffer
        sem_post(&mutexforbuffer);

        //activate semaphore to indicate that the buffer has elements and allow reading from it
        sem_post(&readfrombuff);

        //the monitor will check at random times
        sleep(rand()%13);
    }

}
void* collector(void* arg)
{
    while(1)
    {
        //wait for buffer to have values in it
        sem_wait(&readfrombuff);
        //wait for buffer to be mutually exclusive
        sem_wait(&mutexforbuffer);
        //reading from buffer
        int readvalue=buffer[Bfirst];
        printf("Collector thread: reading from the buffer at position %d value:%d\n",Bfirst,readvalue);
        Bfirst=(Bfirst+1)% Bsize; //incrementing Bfirst
        Bcurrentsize--; //decrementing size of the buffer
        if(Bcurrentsize==0){
            printf("Collector Thread: Buffer Empty!\n");
        }
        //exit mutual exclusion of buffer
        sem_post(&mutexforbuffer);
        //empty one space in the buffer
        sem_post(&writetobuff);
        //Collector reads at random times
        sleep(rand()%13);

    }
}
int main()
{
    srand(time(0)); // seed for future uses of rand()
    int N=10;
    pthread_t mMonitor;
    pthread_t mCollector;
    pthread_t mCounter[N];
    message=0;
    int pshared = 1;
    sem_init(&mutex,pshared,1);
    sem_init(&mutexforbuffer,pshared,1);
    sem_init(&writetobuff,pshared,Bsize);
    sem_init(&readfrombuff,pshared,0);
    for(int i=0;i<N;i++){
        pthread_create(&mCounter[i],NULL,count,&i);
        usleep(300); //sleep for 300 microseconds
    }
    sleep(5);
    pthread_create(&mMonitor,NULL,monitor,NULL);
    sleep(15);
    pthread_create(&mCollector,NULL,collector,NULL);
    for(int i=0;i<N;i++)
        pthread_join(mCounter[i],NULL);
    pthread_join(mMonitor,NULL);
    sem_destroy(&mutex);
    sem_destroy(&mutexforbuffer);
    return 0;
}
