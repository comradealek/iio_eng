#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stc/zsview.h"
#include "iio_string_wrapper.h"

IIOStringWrapper IIOStringWrapper_make(const char * str) {
  IIOStringWrapper string = {0};
  size_t length = strlen(str);
  string.str = malloc(length * sizeof(char));
  if (!string.str) exit(1);
  strncpy(string.str, str, length);
  string.view = zsview_from(string.str);
  return string;
}

void IIOStringWrapper_drop(IIOStringWrapper * string) {
  if (string->str) free(string->str);
}

IIOStringWrapper IIOStringWrapper_clone(IIOStringWrapper string) {
  char * str = malloc(string.view.size * sizeof(char));
  if (!str) exit(1);
  strncpy(str, string.str, string.view.size);
  string.str = str;
  string.view = zsview_from(str);
  return string;
}

size_t IIOStringWrapper_hash(const IIOStringWrapper * string) {
  return zsview_hash(&string->view);
}

bool IIOStringWrapper_eq(const IIOStringWrapper * a, const IIOStringWrapper * b) {
  return zsview_eq(&a->view, &b->view);
}

IIOStringWrapper_raw IIOStringWrapper_toraw(const IIOStringWrapper * string) {
  return string->str;
}

IIOStringWrapper IIOStringWrapper_from(const char * str) {
  return IIOStringWrapper_make(str);
}

bool IIOStringWrapper_raw_eq(const IIOStringWrapper_raw * a, const IIOStringWrapper_raw * b) {
  return strcmp(*a, *b) == 0;
}

size_t IIOStringWrapper_raw_hash(const IIOStringWrapper_raw * s) {
  return c_hash_str(*s);
}
