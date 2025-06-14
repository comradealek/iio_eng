#ifndef IIO_MODEL_H
#define IIO_MODEL_H

#include <vulkan/vulkan.h>
#include "cglm/cglm.h"
#include "iio_eng_typedef.h"

#define IIOVERTEX_ATTRIBUTE_COUNT 8

extern const char * testModelPath;

void iio_load_model(const char * path, IIOModel * model);

void iio_destroy_model(IIOModel * model);

void iio_destroy_material(IIOMaterial * material);

void iio_load_test_model();

#endif