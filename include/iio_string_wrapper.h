#ifndef IIO_STRING_WRAPPER_H
#define IIO_STRING_WRAPPER_H

#include "stc/zsview.h"

typedef struct IIOStringWrapper_S {
  char * str;
  zsview view;
} IIOStringWrapper;

typedef const char * IIOStringWrapper_raw;

IIOStringWrapper IIOStringWrapper_make(const char * str);

void IIOStringWrapper_drop(IIOStringWrapper * string);

IIOStringWrapper IIOStringWrapper_clone(IIOStringWrapper string);

size_t IIOStringWrapper_hash(const IIOStringWrapper * string);

bool IIOStringWrapper_eq(const IIOStringWrapper * a, const IIOStringWrapper * b);

IIOStringWrapper_raw IIOStringWrapper_toraw(const IIOStringWrapper * string);

IIOStringWrapper IIOStringWrapper_from(const char * str);

bool IIOStringWrapper_raw_eq(const IIOStringWrapper_raw * a, const IIOStringWrapper_raw * b);

size_t IIOStringWrapper_raw_hash(const IIOStringWrapper_raw * s);

#endif