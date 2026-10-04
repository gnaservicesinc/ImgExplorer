#include "compare.h"
#include "export.h"
#include "image.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(condition, message)                                               \
    do {                                                                        \
        if (!(condition)) {                                                     \
            fprintf(stderr, "FAIL: %s (line %d)\n", (message), __LINE__);       \
            failures++;                                                         \
        }                                                                       \
    } while (0)

static int make_path(char *out, size_t size, const char *directory,
                     const char *name) {
    int result = snprintf(out, size, "%s/%s", directory, name);
    return result >= 0 && (size_t)result < size;
}

static void check_load(const char *directory, const char *name,
                       unsigned int expected_channels,
                       unsigned int expected_depth, ImageFormat expected_format,
                       float expected_sample, size_t sample_index) {
    char path[4096];
    char error[512];
    Image image;

    image_init(&image);
    CHECK(make_path(path, sizeof(path), directory, name), "fixture path fits");
    CHECK(image_load(&image, path, error, sizeof(error)), error);
    if (image.pixels != NULL) {
        CHECK(image.width == 2u && image.height == 2u, "fixture dimensions");
        CHECK(image.channels == expected_channels, "fixture channel count");
        CHECK(image.source_bit_depth == expected_depth, "source bit depth");
        CHECK(image.format == expected_format, "source format");
        CHECK(fabsf(image.pixels[sample_index] - expected_sample) < 1.0e-6f,
              "decoded sample value");
    }
    image_free(&image);
}

static void test_comparison_semantics(void) {
    float rgb_values[3] = {0.25f, 0.5f, 0.75f};
    float rgba_values[4] = {0.25f, 0.5f, 0.75f, 1.0f};
    Image rgb = {1u, 1u, 3u, 32u, IMAGE_FORMAT_EXR, rgb_values, NULL};
    Image rgba = {1u, 1u, 4u, 32u, IMAGE_FORMAT_EXR, rgba_values, NULL};
    DiffStats stats;

    diff_calculate(&rgb, &rgba, &stats);
    CHECK(stats.compared_channels == 4u, "RGB/RGBA compares four channels");
    CHECK(stats.differing_values == 0u, "implicit alpha is one");

    rgba_values[3] = 0.5f;
    diff_calculate(&rgb, &rgba, &stats);
    CHECK(stats.differing_values == 1u, "changed alpha is detected");
    CHECK(stats.differing_by_channel[3] == 1u, "alpha count is reported");

    rgb_values[0] = -0.0f;
    rgba_values[0] = 0.0f;
    rgba_values[3] = 1.0f;
    diff_calculate(&rgb, &rgba, &stats);
    CHECK(stats.differing_values == 1u, "negative zero differs by raw bits");
}

static void test_png_diff_and_exports(const char *directory) {
    char a_path[4096];
    char b_path[4096];
    char image_export[4096];
    char diff_export[4096];
    char error[512];
    char content[2048];
    Image a;
    Image b;
    DiffStats stats;
    FILE *file;
    size_t used;

    image_init(&a);
    image_init(&b);
    CHECK(make_path(a_path, sizeof(a_path), directory, "rgb8_a.png"), "A path");
    CHECK(make_path(b_path, sizeof(b_path), directory, "rgb8_b.png"), "B path");
    CHECK(make_path(image_export, sizeof(image_export), directory, "core_image.txt"),
          "image export path");
    CHECK(make_path(diff_export, sizeof(diff_export), directory, "core_diff.txt"),
          "diff export path");
    CHECK(image_load(&a, a_path, error, sizeof(error)), error);
    CHECK(image_load(&b, b_path, error, sizeof(error)), error);
    if (a.pixels != NULL && b.pixels != NULL) {
        diff_calculate(&a, &b, &stats);
        CHECK(stats.total_values == 12u, "2x2 RGB has twelve values");
        CHECK(stats.differing_values == 1u, "fixture has one changed value");
        CHECK(stats.differing_pixels == 1u, "fixture has one changed pixel");
        CHECK(stats.differing_by_channel[1] == 1u, "green value differs");
        CHECK(export_image_values(&a, image_export, error, sizeof(error)), error);
        CHECK(export_difference_values(&a, &b, diff_export, error, sizeof(error)),
              error);
    }

    file = fopen(image_export, "rb");
    CHECK(file != NULL, "read image export");
    if (file != NULL) {
        used = fread(content, 1u, sizeof(content) - 1u, file);
        content[used] = '\0';
        (void)fclose(file);
        CHECK(strchr(content, ',') != NULL, "channels use commas");
        CHECK(strchr(content, '\t') != NULL, "pixels use tabs");
        CHECK(strchr(content, '\n') != NULL, "rows use newlines");
    }

    file = fopen(diff_export, "rb");
    CHECK(file != NULL, "read diff export");
    if (file != NULL) {
        used = fread(content, 1u, sizeof(content) - 1u, file);
        content[used] = '\0';
        (void)fclose(file);
        CHECK(strstr(content, "0.003921") != NULL, "diff contains B-A delta");
    }
    image_free(&a);
    image_free(&b);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s FIXTURE_DIRECTORY\n", argv[0]);
        return 2;
    }

    check_load(argv[1], "rgb8_a.png", 3u, 8u, IMAGE_FORMAT_PNG,
               128.0f / 255.0f, 1u);
    check_load(argv[1], "rgba8.png", 4u, 8u, IMAGE_FORMAT_PNG,
               128.0f / 255.0f, 1u);
    check_load(argv[1], "rgb16.png", 3u, 16u, IMAGE_FORMAT_PNG,
               32768.0f / 65535.0f, 1u);
    check_load(argv[1], "rgba16.png", 4u, 16u, IMAGE_FORMAT_PNG,
               32768.0f / 65535.0f, 1u);
    check_load(argv[1], "exr_rgb.exr", 3u, 32u, IMAGE_FORMAT_EXR, -2.0f, 3u);
    check_load(argv[1], "exr_rgba.exr", 4u, 32u, IMAGE_FORMAT_EXR, 0.5f, 7u);
    test_comparison_semantics();
    test_png_diff_and_exports(argv[1]);

    if (failures != 0) {
        fprintf(stderr, "%d core test(s) failed\n", failures);
        return 1;
    }
    printf("Core loader/comparison/export tests passed.\n");
    return 0;
}
