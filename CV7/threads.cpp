#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

struct threads
{
    /* data */
};


void *thread(void *t_args){
    char *name = (char *)t_args;
    printf("Thread %s is running %d\n", name, gettid());

    return nullptr;
}

int main(){

    pthread_t l_p1, l_p2;

    pthread_create(&l_p1, NULL, thread, NULL);
    pthread_create(&l_p2, NULL, thread, NULL);

    pthread_join(l_p1, NULL);
    pthread_join(l_p2, NULL);

    // return 0;
}