#include "compare.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

int images_can_compare(const Image *a, const Image *b, char *error,
                       size_t error_size) {
    if (a == NULL || b == NULL || a->pixels == NULL || b->pixels == NULL) {
        (void)snprintf(error, error_size, "Load both image A and image B first");
        return 0;
    }
    if (a->width != b->width || a->height != b->height) {
        (void)snprintf(error, error_size,
                       "Image dimensions differ: A is %zux%zu, B is %zux%zu",
                       a->width, a->height, b->width, b->height);
        return 0;
    }
    if ((a->channels != 3u && a->channels != 4u) ||
        (b->channels != 3u && b->channels != 4u)) {
        (void)snprintf(error, error_size,
                       "Only RGB and RGBA images can be compared");
        return 0;
    }
    return 1;
}

float image_comparison_value(const Image *image, size_t pixel,
                             unsigned int channel) {
    if (channel == 3u && image->channels == 3u) {
        return 1.0f;
    }
    return image->pixels[pixel * (size_t)image->channels + (size_t)channel];
}

uint32_t float_raw_bits(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void diff_calculate(const Image *a, const Image *b, DiffStats *stats) {
    size_t pixel_count = a->width * a->height;
    size_t pixel;
    unsigned int channels = a->channels > b->channels ? a->channels : b->channels;

    memset(stats, 0, sizeof(*stats));
    stats->total_pixels = pixel_count;
    stats->compared_channels = channels;
    stats->total_values = pixel_count * (size_t)channels;

    for (pixel = 0u; pixel < pixel_count; ++pixel) {
        int pixel_differs = 0;
        unsigned int channel;
        for (channel = 0u; channel < channels; ++channel) {
            float av = image_comparison_value(a, pixel, channel);
            float bv = image_comparison_value(b, pixel, channel);
            if (float_raw_bits(av) != float_raw_bits(bv)) {
                stats->differing_values++;
                stats->differing_by_channel[channel]++;
                pixel_differs = 1;
                if (isfinite(av) && isfinite(bv)) {
                    double absolute = fabs((double)bv - (double)av);
                    stats->finite_delta_values++;
                    stats->sum_absolute_difference += absolute;
                    if (stats->finite_delta_values == 1u ||
                        absolute > stats->max_absolute_difference) {
                        stats->max_absolute_difference = absolute;
                        stats->max_difference_pixel = pixel;
                        stats->max_difference_channel = channel;
                    }
                } else {
                    stats->nonfinite_differences++;
                }
            }
        }
        if (pixel_differs) {
            stats->differing_pixels++;
        }
    }
}

void diff_print_report(const Image *a, const Image *b, size_t example_limit) {
    DiffStats stats;
    size_t pixel;
    size_t examples = 0u;
    double value_percent;
    double pixel_percent;

    diff_calculate(a, b, &stats);
    value_percent = stats.total_values == 0u
                        ? 0.0
                        : 100.0 * (double)stats.differing_values /
                              (double)stats.total_values;
    pixel_percent = stats.total_pixels == 0u
                        ? 0.0
                        : 100.0 * (double)stats.differing_pixels /
                              (double)stats.total_pixels;

    printf("\nBit-exact comparison (A vs B)\n");
    printf("  Size: %zux%zu, channels compared: %u (%s)\n", a->width,
           a->height, stats.compared_channels,
           stats.compared_channels == 4u ? "RGBA" : "RGB");
    if (a->channels != b->channels) {
        printf("  Note: missing RGB alpha is compared as 1.0\n");
    }
    printf("  Differing channel values: %zu / %zu (%.6f%%)\n",
           stats.differing_values, stats.total_values, value_percent);
    printf("  Pixels containing a difference: %zu / %zu (%.6f%%)\n",
           stats.differing_pixels, stats.total_pixels, pixel_percent);
    printf("  Differences by channel:");
    for (unsigned int channel = 0u; channel < stats.compared_channels; ++channel) {
        printf(" %s=%zu", image_channel_name(channel),
               stats.differing_by_channel[channel]);
    }
    putchar('\n');
    if (stats.finite_delta_values > 0u) {
        printf("  Mean absolute finite delta: %.9g\n",
               stats.sum_absolute_difference /
                   (double)stats.finite_delta_values);
        printf("  Maximum absolute finite delta: %.9g at (%zu,%zu) %s\n",
               stats.max_absolute_difference,
               stats.max_difference_pixel % a->width,
               stats.max_difference_pixel / a->width,
               image_channel_name(stats.max_difference_channel));
    }
    if (stats.nonfinite_differences > 0u) {
        printf("  Non-finite differing values: %zu\n",
               stats.nonfinite_differences);
    }

    if (stats.differing_values == 0u) {
        printf("  The compared float32 values are bit-for-bit identical.\n\n");
        return;
    }

    printf("  Example differences (delta is B - A):\n");
    for (pixel = 0u; pixel < stats.total_pixels && examples < example_limit;
         ++pixel) {
        unsigned int channel;
        for (channel = 0u;
             channel < stats.compared_channels && examples < example_limit;
             ++channel) {
            float av = image_comparison_value(a, pixel, channel);
            float bv = image_comparison_value(b, pixel, channel);
            if (float_raw_bits(av) != float_raw_bits(bv)) {
                printf("    (%zu,%zu) %s: A=%.9g [0x%08x], "
                       "B=%.9g [0x%08x]",
                       pixel % a->width, pixel / a->width,
                       image_channel_name(channel), (double)av,
                       (unsigned int)float_raw_bits(av), (double)bv,
                       (unsigned int)float_raw_bits(bv));
                if (isfinite(av) && isfinite(bv)) {
                    printf(", delta=%.9g", (double)bv - (double)av);
                } else {
                    printf(", delta=non-finite");
                }
                putchar('\n');
                examples++;
            }
        }
    }
    putchar('\n');
}
