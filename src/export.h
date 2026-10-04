#ifndef IMGEXPLORER_EXPORT_H
#define IMGEXPLORER_EXPORT_H

#include "image.h"

#include <stddef.h>

int export_image_values(const Image *image, const char *path, char *error,
                        size_t error_size);
int export_difference_values(const Image *a, const Image *b, const char *path,
                             char *error, size_t error_size);

#endif
