// Exciting but dangerous generic array system for c

#ifndef DYNARR_H
#define DYNARR_H
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define INIT_SIZE 2

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

extern const size_t dyn_t_o;
extern const size_t dyn_l_o;
extern const size_t dyn_c_o;
extern const size_t dyn_d_o;

#define DYN_L(pArr) *((size_t*)((void*)pArr + dyn_l_o))
#define DYN_C(pArr) *((size_t*)((void*)pArr + dyn_c_o))

// Takes a type and a pointer to a byteArr and templates a calculation for the length
#define dynarr_length_m(TYPE, pArr) (DYN_L(pArr) / sizeof(TYPE))
// Takes a type and a pointer to a byteArr and allocates and sets the values for the tracker struct
#define dynarr_init_m(TYPE, pArr) (pArr = malloc(dyn_d_o + sizeof(TYPE))); (memcpy((void*)pArr + dyn_t_o, &(Tracker){ (size_t) 0, sizeof(TYPE) }, sizeof(Tracker)))
// Takes a pointer to a byteArr, a pointer to the data to be inserted, and the size of the data and templates a push
#define dynarr_push_m(pArr, pDat, size) (pArr = (((DYN_L(pArr) + (size)) > DYN_C(pArr)) ? resize_arr(pArr, (size)) : pArr)); (memcpy((void*)pArr + dyn_d_o + DYN_L(pArr), pDat, (size))); (DYN_L(pArr) += (size))
// Takes a type, a byteArr pointer and gives array access, eg "dynarr_get_m(uint32_t, array)[index]"
#define dynarr_get_m(TYPE, pArr) ((TYPE *)pArr->dat)

void * init_arr(size_t datasize);
void * dynarr_init(size_t datasize);
void * resize_arr(void * arr, size_t size);
size_t arr_length(byteArr * arr, size_t size);
void * dynarr_push(void * arr, void * dat, size_t size);

#endif