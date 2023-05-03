#include "mr.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;

#include "hash.h"
#include "kvlist.h"

typedef struct {
  kvlist_t* arg1;
  reducer_t arg2;
  kvlist_t* output;
} reducer_arg;

typedef struct {
  kvlist_t* arg1;
  mapper_t arg2;
  size_t num_reducer;
  kvlist_t** shuffle_array;
} thread_arg;

void* reducer_routine(void* arg) {
  reducer_arg* args = (reducer_arg*)arg;
  kvlist_t* List = args->arg1;
  kvlist_t* Output = args->output;
  kvlist_sort(List);
  char* prev;
  kvlist_iterator_t* iterate = kvlist_iterator_new(List);
  kvlist_t* reducer_list = kvlist_new();
  int counter = 0;
  while (true) {
    kvpair_t* pair = kvlist_iterator_next(iterate);

    if (pair == NULL) {
      if (kvlist_empty(reducer_list) == true) {
        kvlist_free(&reducer_list);  // Risky
        break;
      }
      kvlist_t* Reduce_Output = kvlist_new();

      args->arg2(prev, reducer_list, Reduce_Output);  // REDUCE
      pthread_mutex_lock(&my_mutex);
      kvlist_extend(Output, Reduce_Output);
      pthread_mutex_unlock(&my_mutex);
      kvlist_free(&reducer_list);
      kvlist_free(&Reduce_Output);
      reducer_list = NULL;
      Reduce_Output = NULL;

      break;
    }
    if (counter == 0) {
      prev = pair->key;
    }
    int result = strcmp(pair->key, prev);
    if (result == 0) {  // SAME AS PREV
      kvlist_append(reducer_list, kvpair_clone(pair));
    } else {
      // Send it to the output
      kvlist_t* Reduce_Output = kvlist_new();

      args->arg2(prev, reducer_list, Reduce_Output);  // REDUCE
      pthread_mutex_lock(&my_mutex);
      kvlist_extend(Output, Reduce_Output);
      pthread_mutex_unlock(&my_mutex);
      kvlist_free(&reducer_list);
      kvlist_free(&Reduce_Output);
      reducer_list = NULL;
      Reduce_Output = NULL;
      reducer_list = kvlist_new();

      kvlist_append(reducer_list, kvpair_clone(pair));

      prev = pair->key;
    }
    counter += 1;
  }
  kvlist_iterator_free(&iterate);
  kvlist_free(&List);

  // bleh
  return NULL;
}

void* thread(void* arg) {
  thread_arg* args = (thread_arg*)arg;

  kvlist_t* arg1 = args->arg1;

  mapper_t map = args->arg2;

  kvlist_t* output = kvlist_new();
  kvlist_iterator_t* itor = kvlist_iterator_new(arg1);  // FREE OUR ITERATOR
  while (true) {
    kvpair_t* pair = kvlist_iterator_next(itor);
    if (pair == NULL) {
      break;
    }

    map(pair, output);
  }
  // kvlist_free(&arg1);
  kvlist_iterator_free(&itor);

  // SHUFFLE PHASE USING SEMAPHORES + MUTEXES

  // EACH THREAD HAS 1 OUTPUT. WANT TO COMBINE ALL THREADS USING HASH. EACH
  // THREAD WANTS TO TRY AND STORE ITS OUTPUT PAIR TO

  // IDEA: HASH THE KEY -> 123871293 -> % BY num_reducer -> 2: STORE THERE

  kvlist_iterator_t* iterate = kvlist_iterator_new(output);
  while (true) {
    kvpair_t* pair = kvlist_iterator_next(iterate);
    if (pair == NULL) {
      break;
    }

    int index = hash(pair->key) % args->num_reducer;

    pthread_mutex_lock(&my_mutex);
    kvlist_append(args->shuffle_array[index], kvpair_clone(pair));

    pthread_mutex_unlock(&my_mutex);

    // shuffle_array type is kvlist_t

    //
  }
  kvlist_iterator_free(&iterate);
  kvlist_free(&output);

  return NULL;
}

void map_reduce(mapper_t mapper, size_t num_mapper, reducer_t reducer,
                size_t num_reducer, kvlist_t* input, kvlist_t* output) {
  //-----MAPPING-----

  // INPUT PHASE - Take the input, put the key-value pairs into num_mapper
  // different lists

  (void)reducer;

  (void)output;

  // Define an array for our new kvlists
  kvlist_t* list_array[num_mapper];

  for (size_t i = 0; i < num_mapper; i++) {
    list_array[i] = kvlist_new();  // FREE ALL OF THIS
    // kvlist_print(1, list_array[i]);
  }

  // SPLIT PHASE

  // WE
  int counter = 0;
  kvlist_iterator_t* itor = kvlist_iterator_new(input);
  while (true) {
    kvpair_t* pair = kvlist_iterator_next(itor);
    if (pair == NULL) {
      break;
    }
    int index = counter % num_mapper;

    kvlist_append(list_array[index], kvpair_clone(pair));

    // kvlist_print(1, list_array[index]);
    counter += 1;
  }
  // kvlist_free(&input);
  //  FREE THE ITERATOR
  kvlist_iterator_free(&itor);

  // CREATE THE SHUFFLE ARRAY LIST
  kvlist_t** shuffle_array =
      (kvlist_t**)malloc(num_reducer * sizeof(kvlist_t*));

  for (size_t i = 0; i < num_reducer; i++) {
    shuffle_array[i] = kvlist_new();
  }

  // CREATE THE THREADS
  pthread_t array[num_mapper];  // dont need to store pointers because p_threads
                                // are opaque

  thread_arg* args_array = (thread_arg*)malloc(num_mapper * sizeof(thread_arg));
  for (size_t i = 0; i < num_mapper; i++) {
    args_array[i].arg1 = list_array[i];
    args_array[i].arg2 = mapper;
    args_array[i].num_reducer = num_reducer;
    args_array[i].shuffle_array = shuffle_array;

    pthread_create(&array[i], NULL, thread, (void*)&args_array[i]);
  }

  // WAIT FOR THREADS TO FINISH
  for (size_t i = 0; i < num_mapper; i++) {
    pthread_join(array[i], NULL);
  }
  // for (size_t i = 0; i < num_mapper; i++) {
  //   kvlist_free(&(list_array[i]));
  // }

  free(args_array);

  // FREE List_array

  for (size_t i = 0; i < num_mapper; i++) {
    kvlist_free(&list_array[i]);
  }

  //-------REDUCING--------

  pthread_t reducer_thread_array[num_reducer];

  kvlist_t* reducer_output = kvlist_new();
  // dont need to store pointers
  // because p_threads are opaque

  reducer_arg* argument_array =
      (reducer_arg*)malloc(num_reducer * sizeof(reducer_arg));
  for (size_t i = 0; i < num_reducer; i++) {
    argument_array[i].arg1 = shuffle_array[i];
    argument_array[i].arg2 = reducer;
    argument_array[i].output = reducer_output;

    pthread_create(&reducer_thread_array[i], NULL, reducer_routine,
                   (void*)&argument_array[i]);
  }

  for (size_t i = 0; i < num_reducer; i++) {
    pthread_join(reducer_thread_array[i], NULL);
  }

  free(argument_array);

  kvlist_extend(output, reducer_output);

  kvlist_free(&reducer_output);

  free(shuffle_array);

  pthread_mutex_destroy(&my_mutex);
}
