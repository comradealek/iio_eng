#include <dynarr_3.h>

const size_t dyn_t_o = offsetof(charArr, t);
const size_t dyn_l_o = dyn_t_o + offsetof(Tracker, length);
const size_t dyn_c_o = dyn_t_o + offsetof(Tracker, cap);
const size_t dyn_d_o = offsetof(charArr, dat);

//when declaring an array, give `sizeof(type)` as the argument. Note that 
//a struct in the format
//
//typedef struct {
//  Tracker tracker;
//  type arr[];
//} typeArr;
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
    DYN_C(arr) *= 2;
  } while (DYN_C(arr) < DYN_L(arr) + size);
  arr = realloc(arr, dyn_d_o + DYN_C(arr));
  return arr;
}

size_t arr_length(byteArr * arr, size_t size) {
  size_t l = DYN_L(arr) / size;
  return l;
}

void * dynarr_push(void * arr, void * dat, size_t size) {
  if (DYN_L(arr) + size > DYN_C(arr)) arr = resize_arr(arr, size);
  memcpy(arr + dyn_d_o + DYN_L(arr), dat, size);
  DYN_L(arr) += size;
  return arr;
}