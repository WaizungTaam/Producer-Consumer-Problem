/*
 * Copyright 2016 Waizung Taam
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* - 2016-10-08 
 * - waizuntaam@gmail.com
 * - Reference: 
 *     https://en.wikipedia.org/wiki/Producer%E2%80%93consumer_problem
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NUM_PRODUCERS 8
#define NUM_CONSUMERS 8
#define BUFFER_SIZE 128
#define SLEEP_TIME 0

typedef int buffer_t;

buffer_t __buffer[BUFFER_SIZE];
unsigned long __num_items = 0;
pthread_mutex_t __buffer_mutex;
pthread_cond_t __insertable_cond;
pthread_cond_t __removable_cond;

buffer_t produce_item() {
  buffer_t new_item = rand() % BUFFER_SIZE;
  printf("Number of items: %lu.  Producing item %d...\n", 
         __num_items, new_item);
  return new_item;
}
void consume_item(buffer_t item) {
  printf("Number of items: %lu.  Consuming item %d...\n", 
         __num_items, item);
}
void insert_item(buffer_t item) {
  __buffer[__num_items] = item;
  ++__num_items;  
}
buffer_t remove_item() {
  buffer_t rm_item = __buffer[0];
  for (unsigned long i = 1; i < __num_items; ++i) {
    __buffer[i - 1] = __buffer[i];
  }
  --__num_items;
  return rm_item;
}

void* producer(void* thread_id) {
  unsigned long tid = (unsigned long)thread_id;
  while (1) {
    pthread_mutex_lock(&__buffer_mutex);
    buffer_t item = produce_item();
    while (__num_items == BUFFER_SIZE) {
      pthread_cond_wait(&__insertable_cond, &__buffer_mutex);
    }
    insert_item(item);
    if (__num_items == 1) {
      pthread_cond_signal(&__removable_cond);
    }
    pthread_mutex_unlock(&__buffer_mutex);
    sleep(SLEEP_TIME);
  }
  pthread_exit(NULL);
}

void* consumer(void* thread_id) {
  unsigned long tid = (unsigned long)thread_id;
  while (1) {
    pthread_mutex_lock(&__buffer_mutex);
    while (__num_items == 0) {
      pthread_cond_wait(&__removable_cond, &__buffer_mutex);
    }
    buffer_t item = remove_item();
    consume_item(item);
    if (__num_items == BUFFER_SIZE - 1) {
      pthread_cond_signal(&__insertable_cond);
    }
    pthread_mutex_unlock(&__buffer_mutex);
    sleep(SLEEP_TIME);
  }
  pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
  pthread_mutex_init(&__buffer_mutex, NULL);
  pthread_cond_init(&__insertable_cond, NULL);
  pthread_cond_init(&__removable_cond, NULL);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  pthread_t producers[NUM_PRODUCERS], consumers[NUM_CONSUMERS];

  srand(time(NULL));

  for (unsigned long i = 0; i < NUM_PRODUCERS; ++i) {
    pthread_create(&producers[i], NULL, producer, (void*)i);
  }
  for (unsigned long i = 0; i < NUM_CONSUMERS; ++i) {
    pthread_create(&consumers[i], NULL, consumer, (void*)i);
  }

  // The following part may never be reached. Keeping it to maintain the
  // completeness of code.
  for (unsigned long i = 0; i < NUM_PRODUCERS; ++i) {
    pthread_join(producers[i], NULL);
  }
  for (unsigned long i = 0; i < NUM_CONSUMERS; ++i) {
    pthread_join(consumers[i], NULL);
  }

  pthread_attr_destroy(&attr);
  pthread_mutex_destroy(&__buffer_mutex);
  pthread_cond_destroy(&__insertable_cond);
  pthread_cond_destroy(&__removable_cond);
  pthread_exit(NULL);

  return 0;
}