#include "compare.h"
#include "export.h"
#include "image.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_CAPACITY 4096u
#define ERROR_CAPACITY 512u

typedef enum ReadStatus {
    READ_STATUS_EOF = 0,
    READ_STATUS_OK,
    READ_STATUS_TOO_LONG
} ReadStatus;

static ReadStatus read_line(char *buffer, size_t capacity) {
    size_t length;
    int character;

    if (fgets(buffer, (int)capacity, stdin) == NULL) {
        return READ_STATUS_EOF;
    }
    length = strlen(buffer);
    if (length > 0u && buffer[length - 1u] == '\n') {
        buffer[length - 1u] = '\0';
        if (length > 1u && buffer[length - 2u] == '\r') {
            buffer[length - 2u] = '\0';
        }
        return READ_STATUS_OK;
    }
    if (feof(stdin)) {
        return READ_STATUS_OK;
    }
    do {
        character = fgetc(stdin);
    } while (character != '\n' && character != EOF);
    return READ_STATUS_TOO_LONG;
}

static int parse_menu_choice(const char *text, long *choice) {
    char *end;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno != 0 || end == text) {
        return 0;
    }
    while (*end != '\0' && isspace((unsigned char)*end)) {
        ++end;
    }
    if (*end != '\0') {
        return 0;
    }
    *choice = value;
    return 1;
}

static void print_image_summary(char label, const Image *image) {
    if (image->pixels == NULL) {
        printf("  %c: not loaded\n", label);
        return;
    }
    printf("  %c: %s\n", label, image->path);
    printf("     %zux%zu %s, %u-bit %s source, float32 in memory\n",
           image->width, image->height,
           image->channels == 4u ? "RGBA" : "RGB", image->source_bit_depth,
           image_format_name(image->format));
}

static void print_menu(const Image *a, const Image *b) {
    printf("\nImage Explorer\n");
    print_image_summary('A', a);
    print_image_summary('B', b);
    printf("\n"
           "  1. Load or replace image A\n"
           "  2. Load or replace image B\n"
           "  3. Compare A and B\n"
           "  4. Export full difference grid (B - A)\n"
           "  5. Export image A float grid\n"
           "  6. Export image B float grid\n"
           "  7. Show image details\n"
           "  0. Quit\n"
           "Choice: ");
}

static void load_interactively(Image *image, char label) {
    char path[INPUT_CAPACITY];
    char error[ERROR_CAPACITY];
    ReadStatus status;

    printf("Path for image %c (blank cancels): ", label);
    status = read_line(path, sizeof(path));
    if (status == READ_STATUS_EOF || path[0] == '\0') {
        printf("Load cancelled.\n");
        return;
    }
    if (status == READ_STATUS_TOO_LONG) {
        printf("Path is too long.\n");
        return;
    }
    if (!image_load(image, path, error, sizeof(error))) {
        printf("Could not load image %c: %s\n", label, error);
        return;
    }
    printf("Loaded image %c: %zux%zu %s from %u-bit %s.\n", label,
           image->width, image->height,
           image->channels == 4u ? "RGBA" : "RGB", image->source_bit_depth,
           image_format_name(image->format));
}

static int output_path_allowed(const char *path) {
    FILE *existing = fopen(path, "rb");
    char answer[16];

    if (existing == NULL) {
        return 1;
    }
    (void)fclose(existing);
    printf("'%s' already exists. Overwrite it? [y/N]: ", path);
    if (read_line(answer, sizeof(answer)) != READ_STATUS_OK) {
        return 0;
    }
    return (answer[0] == 'y' || answer[0] == 'Y') && answer[1] == '\0';
}

static void export_interactively(const Image *a, const Image *b, int operation) {
    char path[INPUT_CAPACITY];
    char error[ERROR_CAPACITY];
    ReadStatus status;
    int ok;

    if ((operation == 5 && a->pixels == NULL) ||
        (operation == 6 && b->pixels == NULL)) {
        printf("The selected image is not loaded.\n");
        return;
    }
    if (operation == 4 && !images_can_compare(a, b, error, sizeof(error))) {
        printf("Cannot export a difference: %s\n", error);
        return;
    }

    printf("Output path (blank cancels): ");
    status = read_line(path, sizeof(path));
    if (status == READ_STATUS_EOF || path[0] == '\0') {
        printf("Export cancelled.\n");
        return;
    }
    if (status == READ_STATUS_TOO_LONG) {
        printf("Path is too long.\n");
        return;
    }
    if (!output_path_allowed(path)) {
        printf("Export cancelled.\n");
        return;
    }

    if (operation == 4) {
        ok = export_difference_values(a, b, path, error, sizeof(error));
    } else {
        ok = export_image_values(operation == 5 ? a : b, path, error,
                                 sizeof(error));
    }
    if (ok) {
        printf("Wrote '%s'.\n", path);
    } else {
        printf("Export failed: %s\n", error);
    }
}

static void compare_interactively(const Image *a, const Image *b) {
    char error[ERROR_CAPACITY];
    if (!images_can_compare(a, b, error, sizeof(error))) {
        printf("Cannot compare: %s\n", error);
        return;
    }
    diff_print_report(a, b, 8u);
}

int main(void) {
    Image a;
    Image b;
    char input[64];
    long choice;

    image_init(&a);
    image_init(&b);
    (void)setvbuf(stdout, NULL, _IONBF, 0);
    printf("Image Explorer stores all loaded samples as 32-bit floats.\n"
           "PNG samples are normalized to [0,1]; no gamma conversion is applied.\n");

    for (;;) {
        ReadStatus status;
        print_menu(&a, &b);
        status = read_line(input, sizeof(input));
        if (status == READ_STATUS_EOF) {
            printf("\nEnd of input; quitting.\n");
            break;
        }
        if (status != READ_STATUS_OK || !parse_menu_choice(input, &choice)) {
            printf("Please enter one of the numbered menu choices.\n");
            continue;
        }

        switch (choice) {
            case 0:
                image_free(&a);
                image_free(&b);
                printf("Goodbye.\n");
                return 0;
            case 1:
                load_interactively(&a, 'A');
                break;
            case 2:
                load_interactively(&b, 'B');
                break;
            case 3:
                compare_interactively(&a, &b);
                break;
            case 4:
            case 5:
            case 6:
                export_interactively(&a, &b, (int)choice);
                break;
            case 7:
                printf("\nLoaded image details:\n");
                print_image_summary('A', &a);
                print_image_summary('B', &b);
                break;
            default:
                printf("Please enter a choice from 0 through 7.\n");
                break;
        }
    }

    image_free(&a);
    image_free(&b);
    return 0;
}
