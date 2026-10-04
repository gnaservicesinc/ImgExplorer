#include "exr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int write_fixture(const char *path, int with_alpha) {
    static float r[4] = {0.25f, -2.0f, 1000.5f, 1.0f};
    static float g[4] = {0.5f, 3.0f, -0.125f, 2.0f};
    static float b[4] = {0.75f, 4.0f, 0.0f, 3.0f};
    static float a[4] = {1.0f, 0.5f, 0.0f, 2.0f};
    exr_channel channels[4];
    void *planes[4];
    exr_part part;
    exr_image image;
    exr_result result;
    int count = with_alpha ? 4 : 3;

    memset(channels, 0, sizeof(channels));
    memset(&part, 0, sizeof(part));
    memset(&image, 0, sizeof(image));

    if (with_alpha) {
        (void)strcpy(channels[0].name, "A");
        (void)strcpy(channels[1].name, "B");
        (void)strcpy(channels[2].name, "G");
        (void)strcpy(channels[3].name, "R");
        planes[0] = a;
        planes[1] = b;
        planes[2] = g;
        planes[3] = r;
    } else {
        (void)strcpy(channels[0].name, "B");
        (void)strcpy(channels[1].name, "G");
        (void)strcpy(channels[2].name, "R");
        planes[0] = b;
        planes[1] = g;
        planes[2] = r;
    }
    for (int i = 0; i < count; ++i) {
        channels[i].pixel_type = EXR_PIXEL_FLOAT;
        channels[i].x_sampling = 1;
        channels[i].y_sampling = 1;
    }

    part.header.part_type = EXR_PART_SCANLINE;
    part.header.compression =
        with_alpha ? EXR_COMPRESSION_ZIP : EXR_COMPRESSION_NONE;
    part.header.data_window.max_x = 1;
    part.header.data_window.max_y = 1;
    part.header.display_window = part.header.data_window;
    part.header.pixel_aspect_ratio = 1.0f;
    part.header.screen_window_width = 1.0f;
    part.header.num_channels = count;
    part.header.channels = channels;
    part.width = 2;
    part.height = 2;
    part.images = planes;
    image.num_parts = 1;
    image.parts = &part;

    result = exr_save_to_file(path, &image, part.header.compression);
    if (!EXR_OK(result)) {
        fprintf(stderr, "Could not create %s: %s\n", path,
                exr_result_string(result));
        return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    char rgb_path[4096];
    char rgba_path[4096];

    if (argc != 2) {
        fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
        return 2;
    }
    if (snprintf(rgb_path, sizeof(rgb_path), "%s/exr_rgb.exr", argv[1]) >=
            (int)sizeof(rgb_path) ||
        snprintf(rgba_path, sizeof(rgba_path), "%s/exr_rgba.exr", argv[1]) >=
            (int)sizeof(rgba_path)) {
        fprintf(stderr, "fixture path is too long\n");
        return 2;
    }
    return write_fixture(rgb_path, 0) && write_fixture(rgba_path, 1) ? 0 : 1;
}
