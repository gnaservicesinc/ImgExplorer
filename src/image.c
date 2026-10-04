#include "image.h"

#include "exr_loader.h"

#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STBI_ONLY_PNG
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

_Static_assert(sizeof(float) == 4u && FLT_RADIX == 2 && FLT_MANT_DIG == 24 &&
                   FLT_MAX_EXP == 128,
               "Image Explorer requires IEEE-754 binary32 float");

static const unsigned char PNG_SIGNATURE[8] = {
    0x89u, 0x50u, 0x4eu, 0x47u, 0x0du, 0x0au, 0x1au, 0x0au
};
static const unsigned char EXR_SIGNATURE[4] = {0x76u, 0x2fu, 0x31u, 0x01u};

static void set_error(char *error, size_t error_size, const char *message) {
    if (error != NULL && error_size > 0u) {
        (void)snprintf(error, error_size, "%s", message);
    }
}

static char *duplicate_string(const char *text) {
    size_t length;
    char *copy;

    if (text == NULL) {
        return NULL;
    }
    length = strlen(text);
    if (length == SIZE_MAX) {
        return NULL;
    }
    copy = (char *)malloc(length + 1u);
    if (copy != NULL) {
        memcpy(copy, text, length + 1u);
    }
    return copy;
}

static int checked_sample_count(size_t width, size_t height,
                                unsigned int channels, size_t *out) {
    size_t pixels;

    if (width == 0u || height == 0u || channels == 0u ||
        width > SIZE_MAX / height) {
        return 0;
    }
    pixels = width * height;
    if (pixels > SIZE_MAX / (size_t)channels) {
        return 0;
    }
    *out = pixels * (size_t)channels;
    return *out <= SIZE_MAX / sizeof(float);
}

static int load_png(const char *path, Image *out, char *error,
                    size_t error_size) {
    int width = 0;
    int height = 0;
    int channels = 0;
    int is_16_bit;
    size_t count;
    size_t index;
    float *pixels;

    is_16_bit = stbi_is_16_bit(path);
    if (is_16_bit) {
        stbi_us *source = stbi_load_16(path, &width, &height, &channels, 0);
        if (source == NULL) {
            (void)snprintf(error, error_size, "PNG decode failed: %s",
                           stbi_failure_reason());
            return 0;
        }
        if ((channels != 3 && channels != 4) || width <= 0 || height <= 0 ||
            !checked_sample_count((size_t)width, (size_t)height,
                                  (unsigned int)channels, &count)) {
            stbi_image_free(source);
            set_error(error, error_size,
                      "PNG must be a valid 16-bit RGB or RGBA image");
            return 0;
        }
        pixels = (float *)malloc(count * sizeof(*pixels));
        if (pixels == NULL) {
            stbi_image_free(source);
            set_error(error, error_size, "Not enough memory for PNG pixels");
            return 0;
        }
        for (index = 0u; index < count; ++index) {
            pixels[index] = (float)source[index] * (1.0f / 65535.0f);
        }
        stbi_image_free(source);
    } else {
        stbi_uc *source = stbi_load(path, &width, &height, &channels, 0);
        if (source == NULL) {
            (void)snprintf(error, error_size, "PNG decode failed: %s",
                           stbi_failure_reason());
            return 0;
        }
        if ((channels != 3 && channels != 4) || width <= 0 || height <= 0 ||
            !checked_sample_count((size_t)width, (size_t)height,
                                  (unsigned int)channels, &count)) {
            stbi_image_free(source);
            set_error(error, error_size,
                      "PNG must be a valid 8-bit RGB or RGBA image");
            return 0;
        }
        pixels = (float *)malloc(count * sizeof(*pixels));
        if (pixels == NULL) {
            stbi_image_free(source);
            set_error(error, error_size, "Not enough memory for PNG pixels");
            return 0;
        }
        for (index = 0u; index < count; ++index) {
            pixels[index] = (float)source[index] * (1.0f / 255.0f);
        }
        stbi_image_free(source);
    }

    out->width = (size_t)width;
    out->height = (size_t)height;
    out->channels = (unsigned int)channels;
    out->source_bit_depth = is_16_bit ? 16u : 8u;
    out->format = IMAGE_FORMAT_PNG;
    out->pixels = pixels;
    return 1;
}

void image_init(Image *image) {
    if (image != NULL) {
        memset(image, 0, sizeof(*image));
    }
}

void image_free(Image *image) {
    if (image != NULL) {
        free(image->pixels);
        free(image->path);
        image_init(image);
    }
}

int image_load(Image *image, const char *path, char *error, size_t error_size) {
    FILE *file;
    unsigned char signature[8] = {0};
    size_t bytes_read;
    Image loaded;
    int ok;

    if (image == NULL || path == NULL || path[0] == '\0') {
        set_error(error, error_size, "An image path is required");
        return 0;
    }

    file = fopen(path, "rb");
    if (file == NULL) {
        (void)snprintf(error, error_size, "Cannot open '%s'", path);
        return 0;
    }
    bytes_read = fread(signature, 1u, sizeof(signature), file);
    if (ferror(file)) {
        (void)fclose(file);
        (void)snprintf(error, error_size, "Cannot read '%s'", path);
        return 0;
    }
    (void)fclose(file);

    image_init(&loaded);
    if (bytes_read >= sizeof(PNG_SIGNATURE) &&
        memcmp(signature, PNG_SIGNATURE, sizeof(PNG_SIGNATURE)) == 0) {
        ok = load_png(path, &loaded, error, error_size);
    } else if (bytes_read >= sizeof(EXR_SIGNATURE) &&
               memcmp(signature, EXR_SIGNATURE, sizeof(EXR_SIGNATURE)) == 0) {
        ok = exr_loader_load(path, &loaded, error, error_size);
    } else {
        set_error(error, error_size,
                  "Unsupported file: expected a PNG or OpenEXR signature");
        return 0;
    }

    if (!ok) {
        image_free(&loaded);
        return 0;
    }
    loaded.path = duplicate_string(path);
    if (loaded.path == NULL) {
        image_free(&loaded);
        set_error(error, error_size, "Not enough memory for the image path");
        return 0;
    }

    image_free(image);
    *image = loaded;
    return 1;
}

const char *image_format_name(ImageFormat format) {
    switch (format) {
        case IMAGE_FORMAT_PNG:
            return "PNG";
        case IMAGE_FORMAT_EXR:
            return "OpenEXR";
        default:
            return "unknown";
    }
}

const char *image_channel_name(unsigned int channel) {
    static const char *const names[] = {"R", "G", "B", "A"};
    return channel < 4u ? names[channel] : "?";
}
