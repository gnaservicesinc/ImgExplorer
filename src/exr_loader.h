#ifndef IMGEXPLORER_EXR_LOADER_H
#define IMGEXPLORER_EXR_LOADER_H

#include "image.h"

#include <stddef.h>

int exr_loader_load(const char *path, Image *out, char *error,
                    size_t error_size);

#endif
