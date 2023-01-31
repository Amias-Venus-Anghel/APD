/* Anghel Ionela-Carmen 332CB */

#include <pthread.h>
#include <bits/stdc++.h>

struct arg_struct {
    int id;
    int N_mapper;
    int N_reducer;
    pthread_barrier_t* barrier;
    pthread_mutex_t* mutex;
    void* qfiles;
    void* partialSols;
}args;