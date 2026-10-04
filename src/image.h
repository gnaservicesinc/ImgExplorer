#ifndef IMGEXPLORER_IMAGE_H
#define IMGEXPLORER_IMAGE_H

#include <stddef.h>

typedef enum ImageFormat {
    IMAGE_FORMAT_UNKNOWN = 0,
    IMAGE_FORMAT_PNG,
    IMAGE_FORMAT_EXR
} ImageFormat;

typedef struct Image {
    size_t width;
    size_t height;
    unsigned int channels;
    unsigned int source_bit_depth;
    ImageFormat format;
    float *pixels;
    char *path;
} Image;

void image_init(Image *image);
void image_free(Image *image);

/*
 * Load an RGB or RGBA PNG/EXR into a temporary image, then replace *image only
 * on success. PNG values are normalized to [0, 1]. EXR values are unchanged.
 */
int image_load(Image *image, const char *path, char *error, size_t error_size);

const char *image_format_name(ImageFormat format);
const char *image_channel_name(unsigned int channel);

#endif
