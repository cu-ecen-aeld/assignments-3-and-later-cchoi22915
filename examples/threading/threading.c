#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

#define MILLI_TO_MICRO 1000

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_func_args = (struct thread_data *)thread_param;
    
    //sleep after converting milliseconds to microseconds
    usleep(thread_func_args->wait_to_obtain_ms * MILLI_TO_MICRO);
    
    //lock mutex and check return value
    if (0 != (pthread_mutex_lock(thread_func_args->mutex))) 
    {
        perror("ERROR: pthread_mutex_lock failed");
        exit(-1);
    }
    
    //sleep after converting milliseconds to microseconds
    usleep(thread_func_args->wait_to_release_ms * MILLI_TO_MICRO);
    
    //unlock mutex and check return value
    if (0 != (pthread_mutex_unlock(thread_func_args->mutex))) 
    {
        perror("ERROR: pthread_mutex_unlock failed");
        exit(-1);
    }

    //success if code reaches here...
    thread_func_args->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     * 
     * See implementation details in threading.h file comment block
     */
     
    struct thread_data* thread_param = (struct thread_data *)malloc(sizeof(struct thread_data));
    
    //check malloc call
    if (thread_param == NULL) 
    {
        perror("ERROR: Failed to allocate memory...");
        exit(-1);
    }
    
    //set thread parameters
    thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_param->wait_to_release_ms = wait_to_release_ms;
    thread_param->mutex = mutex;
    thread_param->thread_complete_success = false;
    
    //create thread and check return value
    if (0 != pthread_create(thread, NULL, threadfunc, thread_param)) 
    {
        perror("ERROR: pthread_create failed!");
        return false;
    }
    else
    {
        return true;
    }
    
}

