#ifndef IMGEXPLORER_COMPARE_H
#define IMGEXPLORER_COMPARE_H

#include "image.h"

#include <stddef.h>
#include <stdint.h>

typedef struct DiffStats {
    size_t total_pixels;
    size_t differing_pixels;
    size_t total_values;
    size_t differing_values;
    size_t differing_by_channel[4];
    size_t finite_delta_values;
    size_t nonfinite_differences;
    double sum_absolute_difference;
    double max_absolute_difference;
    size_t max_difference_pixel;
    unsigned int max_difference_channel;
    unsigned int compared_channels;
} DiffStats;

int images_can_compare(const Image *a, const Image *b, char *error,
                       size_t error_size);
float image_comparison_value(const Image *image, size_t pixel,
                             unsigned int channel);
uint32_t float_raw_bits(float value);
void diff_calculate(const Image *a, const Image *b, DiffStats *stats);
void diff_print_report(const Image *a, const Image *b, size_t example_limit);

#endif
