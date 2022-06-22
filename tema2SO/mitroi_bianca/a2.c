#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include "a2_helper.h"

#define nrTP7 5
#define nrTP5 42
#define nrTP6 5

//...63...71...62...

typedef struct
{
    int idT;
    int idP;
} TH_P;
sem_t* sem54;
sem_t* sem45;
sem_t* s6371;
sem_t* s7162;

void* task76(void* arg) // sincronizarea thread urilor
{
    TH_P* s = (TH_P*)arg;
// thread-urile din procese diferite
    if(s->idP == 6 && s->idT == 2){
        sem_wait(s7162);
    }
    if(s->idP == 7 && s->idT == 1){
        sem_wait(s6371);
    }
//-------------------------------------------------------------------------------------------------------------------
//thread-urile din acelasi proces
    if(s->idT == 4 && s->idP == 7){
        sem_wait(sem54);
    }
// inceperea thread-ului
    info(BEGIN, s->idP, s->idT);//---------------------------------------------------------------------------------
    
// thread-urile din acelasi proces
    if(s->idT == 5 && s->idP == 7){
        sem_post(sem54);
        sem_wait(sem45);
    }
//----------------------------------------------------------------------------------------------------------------
// incheierea thread-ului
    info(END, s->idP, s->idT);//---------------------------------------------------------------------------------
// thread-urile din procese diferite
    if(s->idP == 7 && s->idT == 1){
        sem_post(s7162);
    }
    if(s->idP == 6 && s->idT == 3){
        sem_post(s6371);
    }
// thread-urile din acelasi proces
//-----------------------------------------------------------------------------------------------------------------
    if(s->idT == 4 && s->idP == 7){
        sem_post(sem45);
    }
    return NULL;
}
int resolvedTP5 = 0;
int waitingTP5 = nrTP5;
int p5t11_ended = 0;
pthread_mutex_t mThreads = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mt = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m5 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c5 = PTHREAD_COND_INITIALIZER;
sem_t sem_threads;
void* task5(void *arg) // bariera de threaduri
{
    TH_P* s = (TH_P*)arg;

    sem_wait(&sem_threads);
    info(BEGIN, s->idP, s->idT);

    pthread_mutex_lock(&mThreads);
    resolvedTP5++;
    waitingTP5--;
    pthread_mutex_unlock(&mThreads);

    if(resolvedTP5 == 4)
        pthread_cond_signal(&c5);

    if(s->idT == 11){
        while(resolvedTP5 != 4){
            pthread_cond_wait(&c5,&m5);
        }
        p5t11_ended = 1;
    }

    if(waitingTP5 <=4 && !p5t11_ended)
        pthread_mutex_lock(&mt);

    pthread_mutex_lock(&mThreads);
        resolvedTP5--;
    pthread_mutex_unlock(&mThreads);

    info(END,s->idP,s->idT);

    if(p5t11_ended){
        pthread_mutex_unlock(&mt);
    }

    sem_post(&sem_threads);

    return NULL;
}

int main()
{
    init();
    if(sem_init(&sem_threads, 0, 4)){
        perror("Nu s-a putut initializa semaforul sem_threads\n");
        exit(1);
    }
    sem54 = sem_open("sem54", O_CREAT, 0644, 0);
    if(sem54 == NULL){
        perror("Nu s-a putut deschide semaforul sem54\n");
        exit(1);
    }
    sem45 = sem_open("sem45", O_CREAT, 0644, 0);
    if(sem45 == NULL){
        perror("Nu s-a putut deschide semaforul sem45\n");
        exit(1);
    }
    s6371 = sem_open("s6371", O_CREAT, 0644, 0);
    if(s6371 == NULL){
        perror("Nu s-a putut deschide semaforul s6371\n");
        exit(1);
    }
    s7162 = sem_open("s7162", O_CREAT, 0644, 0);
    if(s7162 == NULL){
        perror("Nu s-a putut deschide semaforul s7162\n");
        exit(1);
    }
    info(BEGIN, 1, 0);
    pid_t pid2 = fork();
    if(pid2 < 0)
    {
        perror("ERROR: Nu s-a putut crea procesul P2\n");
        exit(1);
    }
    else if(pid2 == 0) // P2
    {
        info(BEGIN, 2, 0);
        pid_t pid3 = fork();
        if(pid3 < 0)
        {
            perror("ERROR: Nu s-a putut crea procesul P3\n");
            exit(1);
        }
        else if(pid3 == 0) // P3
        {
            info(BEGIN, 3, 0);
            pid_t pid5 = fork();
            if(pid5 < 0)
            {
                perror("ERROR: Nu s-a putut crea procesul P5\n");
                exit(1);
            }
            else if(pid5 == 0) // P5
            {
                info(BEGIN, 5, 0);
                // TBD
                pthread_t tids5[nrTP5];
                TH_P params[nrTP5];
                
                    for(int i = 0; i < nrTP5; i++){
                        params[i].idT = i + 1;
                        params[i].idP = 5;
                        if(pthread_create(tids5 + i, NULL, task5, params + i)){
                            printf("ERROR: Nu s-a putut crea thread-ul %d\n", i + 1);
                            exit(1);
                        }
                    }
                    for(int i = 0; i < nrTP5; i++)
                        pthread_join(tids5[i], NULL);
                info(END, 5, 0);
            }
            else if(pid5 > 0) // still P3
            {
                wait(NULL); // p3 waits for p5
                info(END, 3, 0);
            }
        }
        else if(pid3 > 0) // still P2
        {
            wait(NULL); // p2 waits for p3
            pid_t pid4 = fork();
            if(pid4 < 0)
            {
                perror("ERROR: Nu s-a putut crea procesul P4\n");
                exit(1);
            }
            else if(pid4 == 0) // P4
            {
                info(BEGIN, 4, 0);
                pid_t pid6 = fork();
                if(pid6 < 0)
                {
                    perror("ERROR: Nu s-a putut crea procesul P6\n");
                    exit(1);
                }
                else if(pid6 == 0) // P6
                {
                    info(BEGIN, 6, 0);
                    // TBD
                    pthread_t tids6[nrTP6];
                    TH_P params[nrTP6];
                    for(int i = 0; i < nrTP6; i++){
                        params[i].idT = i + 1;
                        params[i].idP = 6;
                        if(pthread_create(tids6 + i, NULL, task76, params + i)){
                            printf("ERROR: Nu s-a putut crea thread-ul %d\n", i + 1);
                            exit(1);
                        }
                    }
                    for(int i = 0; i < nrTP6; i++)
                        pthread_join(tids6[i], NULL);

                    info(END, 6, 0);
                }
                else if(pid6 > 0) // still P4
                {
                    pid_t pid7 = fork();
                    if(pid7 < 0)
                    {
                        perror("ERROR: Nu s-a putut crea procesul P7\n");
                        exit(1);
                    }
                    else if(pid7 == 0) // P7
                    {   
                        info(BEGIN, 7, 0);
                        // TBD
                        pthread_t tids7[nrTP7];
                        TH_P params[nrTP7];
                        for(int i = 0; i < nrTP7; i++){
                            params[i].idT = i + 1;
                            params[i].idP = 7;
                            if(pthread_create(tids7 + i, NULL, task76, params + i)){
                                printf("ERROR: Nu s-a putut crea thread-ul %d\n", i + 1);
                                exit(1);
                            }
                        }
                        for(int i = 0; i < nrTP7; i++)
                            pthread_join(tids7[i], NULL);
                        info(END, 7, 0);
                    }
                    else if(pid7 > 0) // still p4
                    {
                        wait(NULL); // P4 waits for P7
                        wait(NULL); // P4 waits for P6
                        info(END, 4, 0);
                    }
                }
            }
            else if(pid4 > 0)
            {
                wait(NULL); // P2 waits for P4
                info(END, 2, 0);
            }
        }
    }
    else if(pid2 > 0) // still P1
    {
        wait(NULL); // P1 waits for P2
        sem_unlink("s6371");
        sem_unlink("s7162");
        sem_destroy(s6371);
        sem_destroy(s7162);
        sem_unlink("sem45");
        sem_unlink("sem54");
        sem_destroy(sem45);
        sem_destroy(sem54);
        info(END, 1, 0);
    }

    return 0;
}