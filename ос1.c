#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <pthread.h>

pthread_cond_t cond_provider = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_consumer = PTHREAD_COND_INITIALIZER; 
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int ready = 0;
int event_counter = 0;

void* provider(void* arg)
{
    for (int i = 1; i <= 6; ++i)
    {
        sleep(1); 

        pthread_mutex_lock(&lock);

        while (ready == 1) 
        {
            pthread_cond_wait(&cond_provider, &lock);
        }

        event_counter = i;
        ready = 1;
        printf("provided: event %d sent\n", i);
        pthread_cond_signal(&cond_consumer);  
        pthread_mutex_unlock(&lock);
    }
    return 0;
}

void* consumer(void* arg) 
{
    for (int i = 0; i < 6; ++i) 
    {
        pthread_mutex_lock(&lock);

        while (ready == 0)
        {
            pthread_cond_wait(&cond_consumer, &lock);
        }

        int received = event_counter;
        ready = 0;
        printf("consumed: event %d received\n", received);
        pthread_cond_signal(&cond_provider);
        pthread_mutex_unlock(&lock);
    }
    return 0;
}

int main() 
{
    pthread_t t_provider, t_consumer;

    pthread_create(&t_provider, NULL, provider, NULL);
    pthread_create(&t_consumer, NULL, consumer, NULL);

    pthread_join(t_provider, NULL);
    pthread_join(t_consumer, NULL);

    pthread_cond_destroy(&cond_consumer);
    pthread_cond_destroy(&cond_provider);
    pthread_mutex_destroy(&lock);

    return 0;
}