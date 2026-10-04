#include "exr_loader.h"

#include "exr.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checked_float_count(int32_t width, int32_t height,
                               unsigned int channels, size_t *out) {
    size_t pixels;

    if (width <= 0 || height <= 0 || channels == 0u ||
        (size_t)width > SIZE_MAX / (size_t)height) {
        return 0;
    }
    pixels = (size_t)width * (size_t)height;
    if (pixels > SIZE_MAX / (size_t)channels) {
        return 0;
    }
    *out = pixels * (size_t)channels;
    return *out <= SIZE_MAX / sizeof(float);
}

static int channel_slot(const char *name) {
    if (strcmp(name, "R") == 0) {
        return 0;
    }
    if (strcmp(name, "G") == 0) {
        return 1;
    }
    if (strcmp(name, "B") == 0) {
        return 2;
    }
    if (strcmp(name, "A") == 0) {
        return 3;
    }
    return -1;
}

int exr_loader_load(const char *path, Image *out, char *error,
                    size_t error_size) {
    exr_image decoded;
    exr_result result;
    exr_part *part;
    int channel_indices[4] = {-1, -1, -1, -1};
    unsigned int output_channels;
    int channel;
    size_t sample_count;
    size_t pixel_count;
    size_t pixel;
    float *pixels;

    memset(&decoded, 0, sizeof(decoded));
    result = exr_load_from_file(path, NULL, &decoded);
    if (!EXR_OK(result)) {
        (void)snprintf(error, error_size, "OpenEXR decode failed: %s",
                       exr_result_string(result));
        return 0;
    }
    if (decoded.num_parts != 1) {
        (void)snprintf(error, error_size,
                       "OpenEXR must contain exactly one image part (found %d)",
                       decoded.num_parts);
        exr_image_free(&decoded);
        return 0;
    }

    part = &decoded.parts[0];
    if (part->is_deep || part->images == NULL) {
        exr_image_free(&decoded);
        (void)snprintf(error, error_size,
                       "Deep OpenEXR images are not supported");
        return 0;
    }
    if (part->header.num_channels != 3 && part->header.num_channels != 4) {
        exr_image_free(&decoded);
        (void)snprintf(error, error_size,
                       "OpenEXR must have exactly R, G, B and optional A channels");
        return 0;
    }

    for (channel = 0; channel < part->header.num_channels; ++channel) {
        const exr_channel *description = &part->header.channels[channel];
        int slot = channel_slot(description->name);
        if (slot < 0 || channel_indices[slot] >= 0) {
            exr_image_free(&decoded);
            (void)snprintf(error, error_size,
                           "Unsupported OpenEXR channel set; expected R, G, B and optional A");
            return 0;
        }
        if (description->pixel_type != EXR_PIXEL_FLOAT) {
            (void)snprintf(error, error_size,
                           "OpenEXR channel '%s' is not 32-bit float",
                           description->name);
            exr_image_free(&decoded);
            return 0;
        }
        if (description->x_sampling != 1 || description->y_sampling != 1) {
            (void)snprintf(error, error_size,
                           "Subsampled OpenEXR channel '%s' is not supported",
                           description->name);
            exr_image_free(&decoded);
            return 0;
        }
        channel_indices[slot] = channel;
    }

    if (channel_indices[0] < 0 || channel_indices[1] < 0 ||
        channel_indices[2] < 0) {
        exr_image_free(&decoded);
        (void)snprintf(error, error_size,
                       "OpenEXR is missing one or more R, G, B channels");
        return 0;
    }
    output_channels = channel_indices[3] >= 0 ? 4u : 3u;
    if (!checked_float_count(part->width, part->height, output_channels,
                             &sample_count)) {
        exr_image_free(&decoded);
        (void)snprintf(error, error_size,
                       "OpenEXR dimensions are invalid or too large");
        return 0;
    }

    pixels = (float *)malloc(sample_count * sizeof(*pixels));
    if (pixels == NULL) {
        exr_image_free(&decoded);
        (void)snprintf(error, error_size,
                       "Not enough memory for OpenEXR pixels");
        return 0;
    }
    pixel_count = (size_t)part->width * (size_t)part->height;
    for (pixel = 0u; pixel < pixel_count; ++pixel) {
        unsigned int slot;
        for (slot = 0u; slot < output_channels; ++slot) {
            const float *plane =
                (const float *)part->images[channel_indices[slot]];
            /* memcpy retains every source float bit, including NaN payloads. */
            memcpy(&pixels[pixel * output_channels + slot], &plane[pixel],
                   sizeof(float));
        }
    }

    out->width = (size_t)part->width;
    out->height = (size_t)part->height;
    out->channels = output_channels;
    out->source_bit_depth = 32u;
    out->format = IMAGE_FORMAT_EXR;
    out->pixels = pixels;
    exr_image_free(&decoded);
    return 1;
}
