#ifndef DYNARR_H
#define DYNARR_H
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define INIT_SIZE 2

//TODO: Implement circular buffer style data mgmt (possible?)
//TODO: Implement pop, resize down, and queue style structure

typedef struct {
  size_t length;
  size_t cap;
} Tracker;

typedef struct {
  Tracker t;
  char dat[];
} charArr;

typedef struct {
  Tracker t;
  unsigned char dat[];
} byteArr;

const size_t dyn_t_o = offsetof(charArr, t);
const size_t dyn_l_o = dyn_t_o + offsetof(Tracker, length);
const size_t dyn_c_o = dyn_t_o + offsetof(Tracker, cap);
const size_t dyn_d_o = offsetof(charArr, dat);

#define DYN_L *((size_t*)(arr + dyn_l_o))
#define DYN_C *((size_t*)(arr + dyn_c_o))

//when declaring an array, give `sizeof(type)` as the argument. Note that 
//a struct in the format
//
//`typedef struct {
//  Tracker tracker;
//  type arr[];
//} typeArr;'
//
//is necessary for the operations to work.
void * init_arr(size_t datasize) {
  void * arr = malloc(dyn_d_o + datasize * INIT_SIZE);
  memcpy(arr + dyn_t_o, &(Tracker){(size_t) 0, (size_t) datasize * INIT_SIZE}, sizeof(Tracker));
  return arr;
}

void * dynarr_init(size_t datasize) {
  void * arr = malloc(dyn_d_o + datasize);
  memcpy( arr + dyn_t_o, &(Tracker){ (size_t) 0 , datasize } , sizeof(Tracker) );
  return arr;
}

void * resize_arr(void * arr, size_t size) {
  do {
    DYN_C *= 2;
  } while (DYN_C < DYN_L + size);
  arr = realloc(arr, dyn_d_o + DYN_C);
  return arr;
}

size_t arr_length(void * arr, size_t size) {
  size_t l = DYN_L / size;
  return l;
}

void * dynarr_push(void * arr, void * dat, size_t size) {
  if (DYN_L + size > DYN_C) arr = resize_arr(arr, size);
  memcpy(arr + dyn_d_o + DYN_L, dat, size);
  DYN_L += size;
  return arr;
}

#define dynarr_init_m(TYPE, arr) (arr = malloc(dyn_d_o + sizeof(TYPE))); memcpy(arr + dyn_t_o, &(Tracker){ (size_t) 0, sizeof(TYPE) }, sizeof(Tracker))
#endif