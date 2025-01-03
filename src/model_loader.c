#include <stdlib.h>
#include <stdint.h>
#include <model_loader.h>
#include <iio_eng_typedef.h>
#include <yyjson.h>

/**
 * Load a model into the dest location from the filepath at the source location
 * 
 * 
 */
void iio_load_model(IIOModel * dest, const char * src) {
  yyjson_read_err err;
  yyjson_doc * file = yyjson_read_file(src, 0, NULL, &err);
  if (!file) { // if the file fails to load, then we free it, print an error, and return the function
    yyjson_doc_free(file);
    fprintf(stderr, "Failed to load file at %p\n", src);
    fprintf(stderr, "Read error: %s, code: %u, byte position: %lu\n", err.msg, err.code, err.pos);
    size_t line, col, chr;
    return;
  }

  

  // We are done, so we free the file and return
  yyjson_doc_free(file);
  return;
}