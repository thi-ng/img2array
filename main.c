#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

// Uncomment any of the next lines limit image format support:
// #define STBI_ONLY_PNG
// #define STBI_ONLY_JPEG
// #define STBI_ONLY_TGA
// #define STBI_ONLY_GIF
// #define STBI_ONLY_BMP
// #define STBI_ONLY_PSD
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_DEFINE
#include "stb.h"

#define ALIGN_PREFIX ""
#define ALIGN_SUFFIX "__attribute__ ((aligned (4)))"
#define NUM_COLOR_MODES 4

#ifdef _WIN32
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

typedef struct {
    char *id;
    uint8_t stride;
    uint8_t row;
} ColorMode;

typedef struct {
    char *path;
    char *name;
    char *alignStart;
    char *alignEnd;
    ColorMode *mode;
    uint8_t modeID;
} Config;

static ColorMode modes[NUM_COLOR_MODES] = {
    {.id = "argb8888", .stride = 4, .row = 4},
    {.id = "rgb888", .stride = 3, .row = 6},
    {.id = "rgb565", .stride = 2, .row = 8},
    {.id = "argb4444", .stride = 2, .row = 8}};

static Config config = {.mode = &modes[0],
                        .modeID = 0,
                        .name = NULL,
                        .alignStart = ALIGN_PREFIX,
                        .alignEnd = ALIGN_SUFFIX};

static void usage();
static void filename(char *path, char *dest, uint32_t size);
static int convert(const Config *config);

int main(int argc, char **argv) {
    char c;
    char name[64];
    uint8_t id;
    uint8_t err = 0;
    while (!err && (c = getopt(argc, argv, ":a:e:m:n:")) != -1) {
        switch (c) {
        case 'a':
            config.alignStart = optarg;
            break;
        case 'e':
            config.alignEnd = optarg;
            break;
        case 'm':
            id = atoi(optarg);
            if (id < NUM_COLOR_MODES) {
                config.modeID = id;
                config.mode = &modes[id];
            } else {
                fprintf(stderr, "invalid color mode: %d\n", id);
                err = 1;
            }
            break;
        case 'n':
            config.name = optarg;
            break;
        case '?':
            fprintf(stderr, "Option -%c requires an argument\n", optopt);
            err = 1;
            break;
        default:
            err = 1;
        }
    }
    if (!err && optind >= argc) {
        fprintf(stderr, "Missing input file\n");
        err = 1;
    }
    if (!err && optind < argc - 1) {
        fprintf(stderr, "Extraneous arguments given\n");
        err = 1;
    }
    if (err) {
        usage();
        return 1;
    }
    config.path = argv[optind];
    if (config.name == NULL) {
        filename(config.path, name, 64);
        config.name = name;
    }
    return convert(&config);
}

static int convert(const Config *config) {
    int32_t w, h, comps;
    const uint8_t *pixels = stbi_load(config->path, &w, &h, &comps, 4);
    if (pixels == NULL) {
        fprintf(stderr, "error loading image: %s\n", config->path);
        return 1;
    }

    const ColorMode *mode = config->mode;
    const uint32_t srclen = w * h * 4;
    const uint32_t destlen = w * h * mode->stride;
    fprintf(stdout,
            "#pragma once\n#include <stdint.h>\n%s\nconst uint8_t %s_%s[%u] "
            "%s = {\n",
            config->alignStart, config->name, mode->id, destlen, config->alignEnd);

    for (uint32_t i = 0, k = mode->row; i < srclen; i += 4) {
        uint8_t r = pixels[i];
        uint8_t g = pixels[i + 1];
        uint8_t b = pixels[i + 2];
        uint8_t a = pixels[i + 3];
        switch (config->modeID) {
        case 0:
            fprintf(stdout, "0x%02x, 0x%02x, 0x%02x, 0x%02x", b, g, r, a);
            break;
        case 1:
            fprintf(stdout, "0x%02x, 0x%02x, 0x%02x", b, g, r);
            break;
        case 2:
            fprintf(stdout, "0x%02x, 0x%02x", (b >> 3) | ((g & 0x1c) << 3),
                    (g >> 5) | (r & 0xf8));
            break;
        case 3:
            fprintf(stdout, "0x%02x, 0x%02x", (g & 0xf0) | (b >> 4),
                    (a & 0xf0) | (r >> 4));
            break;
        default:
            break;
        }
        if (i < srclen - 4) {
            fprintf(stdout, ", ");
        }
        if (!(--k)) {
            fprintf(stdout, "\n");
            k = mode->row;
        }
    }
    fprintf(stdout, "\n};\n");
    return 0;
}

static void filename(char *path, char *dest, uint32_t size) {
    char *start = strrchr(path, PATH_SEP);
    if (start != NULL) {
        start++;
    } else {
        start = path;
    }
    char *end = strrchr(path, '.');
    if (end == NULL) {
        end = strrchr(path, '\0');
    }
    uint32_t len = (uint32_t)(end - start);
    if (len > size - 1) {
        len = size - 1;
    }
    strncpy(dest, start, len);
    dest[len] = 0;
}

static void usage() {
    fprintf(stderr, "Usage:\timg2array [options] image [ > dest.h ]\n");
    fprintf(stderr,
            "\t-a STRING\tarray alignment prefix\n\t\t\t(default: \"%s\")\n",
            ALIGN_PREFIX);
    fprintf(stderr,
            "\t-e STRING\tarray alignment suffix\n\t\t\t(default: \"%s\")\n",
            ALIGN_SUFFIX);
    fprintf(stderr, "\t-n STRING\tarray base name\n"
                    "\t\t\t(default: image filename w/o ext)\n");
    fprintf(stderr, "\t-m INT\t\tdestination color mode (default 0):\n");
    for (uint8_t i = 0; i < NUM_COLOR_MODES; i++) {
        fprintf(stderr, "\t\t\t%u = %s\n", i, modes[i].id);
    }
}
