/*
##########################################################################
IN THIS PROJECT I USE MONITORS & MUTEX TO AVOID FROM DEADLOCK!
##########################################################################
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

enum distribution {exponential, uniform};
enum state {thinking, hungry, dining};

int num_phsp;
int min_think;;
int max_think;
int min_dine;
int max_dine;
int count;
enum distribution dst;

/*
##########################################################################
MONITOR STRUCTERS - START
##########################################################################
*/

struct monitor_t{

    pthread_mutex_t *mutex;
    pthread_cond_t *status;
    enum state *philosopher_state;

} forks;

// INITIALIZE MONITOR!
void init_monitor(int num){

    for(int i = 0; i < num; i++){
        forks.philosopher_state[i] = thinking;
    }

}

/*
##########################################################################
MONITOR STRUCTERS - FINISH
##########################################################################
*/


/*
##########################################################################
DISTRIBUTION FUNCTIONS - START
##########################################################################
*/

int generate_random_time_for_dining_and_thinking(int max, int min){
    
    double think_and_dining_time = 0;

    if(max == min){
        return max;
    }
  
    if(dst == uniform){
        while(think_and_dining_time > max || think_and_dining_time < min){
            double random_number = ((double)rand() / RAND_MAX);
            think_and_dining_time = max * random_number;
        }
    } else if(dst == exponential) {
        while(think_and_dining_time > max || think_and_dining_time < min){
            double random_number = ((double)rand() / RAND_MAX);
            think_and_dining_time = (-((double)(min + max)) / 2) * log(1 - random_number);
        }
    } else {
        exit(0);
    }

    return (int)think_and_dining_time;

}

/*
##########################################################################
DISTRIBUTION FUNCTIONS - FINISH
##########################################################################
*/

/*
##########################################################################
PHILOSOPHER DINING - START
##########################################################################
*/

void philosopher_dining(int i){
    if(forks.philosopher_state[(i + num_phsp - 1) % num_phsp] != dining){
        if(forks.philosopher_state[( i + 1 ) % num_phsp] != dining){
            if(forks.philosopher_state[i] == hungry){
                forks.philosopher_state[i] = dining;
                printf(" - Philosopher %d is dining. \n", (i + 1));
                pthread_cond_signal(&forks.status[i]);
            }
        }
    }
}

/*
##########################################################################
PHILOSOPHER DINING - FINISH
##########################################################################
*/


/*
##########################################################################
TAKE FORK - START
##########################################################################
*/

int take_fork(int i){
    forks.philosopher_state[i] = hungry;
    
    printf(" - Philoshoper %d is hungry. \n", (i + 1));
    
    clock_t start = clock();
    philosopher_dining(i);
    int auto_lock = -1;
    
    if(forks.philosopher_state[i] != dining){
        
        if(forks.philosopher_state[(i + 1) % num_phsp] == dining){
        
            auto_lock = (i + 1) % num_phsp;
            pthread_cond_wait(&forks.status[i], &forks.mutex[(i + 1) % num_phsp]);
        
        } else{   
        
            auto_lock = i;
            pthread_cond_wait(&forks.status[i], &forks.mutex[i]);
        
        }

    } else{ 
        
        pthread_mutex_lock(&forks.mutex[i]);
        pthread_mutex_lock(&forks.mutex[(i + 1) % num_phsp]);
    
    }

    
    if(auto_lock > 0){
        
        if(auto_lock == i){
        
            pthread_mutex_lock(&forks.mutex[(i + 1) % num_phsp]);
        
        } else{
          
            pthread_mutex_lock(&forks.mutex[i]);
        
        }
    }

    return (clock() - start) * 1000 / CLOCKS_PER_SEC;
}

/*
##########################################################################
TAKE FORK - FINISH
##########################################################################
*/

/*
##########################################################################
PUT FORK - START
##########################################################################
*/

void put_fork(int philosopher_number){
    
    forks.philosopher_state[philosopher_number] = thinking;

    pthread_mutex_unlock(&forks.mutex[philosopher_number]);
    pthread_mutex_unlock(&forks.mutex[(philosopher_number + 1) % num_phsp]);

    printf(" - Philosopher %d is thinking. \n", (philosopher_number + 1));

    philosopher_dining((philosopher_number + num_phsp - 1) % num_phsp);
    philosopher_dining((philosopher_number + 1) % num_phsp);

}
/*
##########################################################################
PUT FORK - FINISH
##########################################################################
*/

/*
##########################################################################
PHILOSOPHER START (CREATION) - START
##########################################################################
*/

void* philosopher_start(void* arg){

    int id = *((int *)arg);
    int local_count = 0;
    int h_time_sum = 0;

    printf(" - Philosopher %d created and start to thinking. \n", (id + 1));

    while(local_count < count){
        
        usleep((useconds_t)(generate_random_time_for_dining_and_thinking(max_think, min_think) * 1000));
        
        h_time_sum = h_time_sum + take_fork(id);

        int dining_time = generate_random_time_for_dining_and_thinking(max_dine, min_dine);
        
        clock_t start = clock();
        clock_t diff;
        
        int msec;
        
        while(msec < dining_time){

            diff = clock() - start;
            msec = diff * 1000 / CLOCKS_PER_SEC;

        }

        
        put_fork(id);

        local_count = local_count + 1;
    }

    
    printf(" - Philosopher %d duration of hungry state = %d\n", (id + 1), h_time_sum);

    pthread_exit(NULL);

}

/*
##########################################################################
PHILOSOPHER START (CREATION) - FINISH
##########################################################################
*/

int main(int argc, char *argv[]){
    
    /*
    ##########################################################################
    CHECK PARAMETERS AND SET VALUES!!! START
    ##########################################################################
    */

    num_phsp = atoi(argv[1]);

    if(num_phsp % 2 == 0 || num_phsp > 27){
        printf("There are some problems on number of philosopher parameter!!! Enter smaller than 27 philosophers and odd number. \n"); 
        exit(0);
    }

    min_think = atoi(argv[2]);
    max_think = atoi(argv[3]);
    min_dine = atoi(argv[4]);
    max_dine = atoi(argv[5]);

    if(max_think > 60000){
        printf("There are some problems on thinking parameter!!! Enter smaller than 60 seconds. \n");        
        exit(0);
    }

    if(max_dine > 60000){
        printf("There are some problems on dining parameter!!! Enter smaller than 60 seconds. \n");        
        exit(0);
    }

    if(min_think < 1){
        printf("There are some problems on thinking parameter!!! Enter bigger than 1 milisecond. \n");        
        exit(0);    
    }

    if(min_dine < 1){
        printf("There are some problems on dining parameter!!! Enter bigger than 1 milisecond. \n");        
        exit(0);
    }

    if(strcmp(argv[6], "exponential") == 0){
        dst = exponential;
    }
    else if(strcmp(argv[6], "uniform") == 0){
        dst = uniform;
    }
    else {
        printf("There are some problems on distribution parameter!!! \n");
        exit(0);
    }

    count = atoi(argv[7]);

    /*
    ##########################################################################
    CHECK PARAMETERS AND SET VALUES!!! FINISH
    ##########################################################################
    */

    /*
    ##########################################################################
    ALLOCATE SOURCES BY MALLOC & INITIALIZE MUTEX - START
    ##########################################################################
    */

    forks.mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * num_phsp);
    forks.status = (pthread_cond_t *)malloc(sizeof(pthread_cond_t) * num_phsp);
    forks.philosopher_state = (enum state *)malloc(sizeof(enum state) * num_phsp);

    init_monitor(num_phsp);

    for(int i = 0; i < num_phsp; i++){
        pthread_mutex_init(&forks.mutex[i], NULL);
        pthread_cond_init(&forks.status[i], NULL);
    }

    /*
    ##########################################################################
    ALLOCATE SOURCES BY MALLOC & INITIALIZE MuTEX - FINISH
    ##########################################################################
    */

    /*
    ##########################################################################
    CREATE THREADS (PHILOSOPHERS) - START
    ##########################################################################
    */

    pthread_t threads[num_phsp];
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    int ids[num_phsp];
    for(int i = 0; i < num_phsp; i++){
        ids[i] = i;
    }

    for(int i = 0; i < num_phsp; i++){
        pthread_create(&threads[i], &attr, philosopher_start, (void *)&ids[i]);
    }

    
    for(int i = 0; i < num_phsp; i++){
        pthread_join(threads[i], NULL);
    }

    
    pthread_attr_destroy(&attr);

    for(int i = 0; i < num_phsp; i++){
        pthread_mutex_destroy(&forks.mutex[i]);
        pthread_cond_destroy(&forks.status[i]);
    }

    /*
    ##########################################################################
    CREATE THREADS (PHILOSOPHERS) - START
    ##########################################################################
    */

    // RELEASE!
    free(forks.mutex);
    free(forks.status);
    free(forks.philosopher_state);
    exit(0);
}

/*
##########################################################################
FINISHED. :)
##########################################################################
*/