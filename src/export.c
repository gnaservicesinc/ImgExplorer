#include "export.h"

#include "compare.h"

#include <float.h>
#include <stdio.h>

static int finish_file(FILE *file, const char *path, char *error,
                       size_t error_size, int failed) {
    if (fclose(file) != 0) {
        failed = 1;
    }
    if (failed) {
        (void)snprintf(error, error_size, "Failed while writing '%s'", path);
        return 0;
    }
    return 1;
}

static int write_float(FILE *file, float value) {
    return fprintf(file, "%.*g", FLT_DECIMAL_DIG, (double)value) >= 0;
}

int export_image_values(const Image *image, const char *path, char *error,
                        size_t error_size) {
    FILE *file;
    size_t y;
    int failed = 0;

    if (image == NULL || image->pixels == NULL) {
        (void)snprintf(error, error_size, "The selected image is not loaded");
        return 0;
    }
    file = fopen(path, "w");
    if (file == NULL) {
        (void)snprintf(error, error_size, "Cannot open '%s' for writing", path);
        return 0;
    }

    for (y = 0u; y < image->height && !failed; ++y) {
        size_t x;
        for (x = 0u; x < image->width && !failed; ++x) {
            size_t pixel = y * image->width + x;
            unsigned int channel;
            for (channel = 0u; channel < image->channels; ++channel) {
                if (channel > 0u && fputc(',', file) == EOF) {
                    failed = 1;
                    break;
                }
                if (!write_float(file,
                                 image->pixels[pixel * image->channels + channel])) {
                    failed = 1;
                    break;
                }
            }
            if (!failed && x + 1u < image->width && fputc('\t', file) == EOF) {
                failed = 1;
            }
        }
        if (!failed && fputc('\n', file) == EOF) {
            failed = 1;
        }
    }
    return finish_file(file, path, error, error_size, failed);
}

int export_difference_values(const Image *a, const Image *b, const char *path,
                             char *error, size_t error_size) {
    FILE *file;
    unsigned int channels;
    size_t y;
    int failed = 0;

    if (!images_can_compare(a, b, error, error_size)) {
        return 0;
    }
    file = fopen(path, "w");
    if (file == NULL) {
        (void)snprintf(error, error_size, "Cannot open '%s' for writing", path);
        return 0;
    }
    channels = a->channels > b->channels ? a->channels : b->channels;

    for (y = 0u; y < a->height && !failed; ++y) {
        size_t x;
        for (x = 0u; x < a->width && !failed; ++x) {
            size_t pixel = y * a->width + x;
            unsigned int channel;
            for (channel = 0u; channel < channels; ++channel) {
                float av = image_comparison_value(a, pixel, channel);
                float bv = image_comparison_value(b, pixel, channel);
                if (channel > 0u && fputc(',', file) == EOF) {
                    failed = 1;
                    break;
                }
                if (!write_float(file, bv - av)) {
                    failed = 1;
                    break;
                }
            }
            if (!failed && x + 1u < a->width && fputc('\t', file) == EOF) {
                failed = 1;
            }
        }
        if (!failed && fputc('\n', file) == EOF) {
            failed = 1;
        }
    }
    return finish_file(file, path, error, error_size, failed);
}
