#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

//#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_DEFINE
#include "stb.h"

#define NUM_COLOR_MODES 4

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
                        .name = "image",
                        .alignStart = "",
                        .alignEnd = "__attribute__ ((aligned (4)))"};

static void usage();
static int convert(const Config *config);

int main(int argc, char **argv) {
    char c;
    uint8_t id;
    while ((c = getopt(argc, argv, ":a:e:m:n:")) != -1) {
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
                usage();
                return 1;
            }
            break;
        case 'n':
            config.name = optarg;
            break;
        case '?':
            fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            return 1;
        default:
            usage();
            return 1;
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "Missing input file\n");
        usage();
        return 1;
    }
    config.path = argv[optind];
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
    const uint32_t len = w * h * mode->stride;
    fprintf(stdout,
            "#pragma once\n#include <stdint.h>\n%s\nconst uint8_t %s_%s[%u] "
            "%s = {\n",
            config->alignStart, config->name, mode->id, len, config->alignEnd);

    for (uint32_t i = 0, k = mode->row; i < len; i += 4) {
        uint8_t a = pixels[i];
        uint8_t b = pixels[i + 1];
        uint8_t c = pixels[i + 2];
        uint8_t d = pixels[i + 3];
        switch (config->modeID) {
        case 0:
            fprintf(stdout, "0x%02x, 0x%02x, 0x%02x, 0x%02x", a, b, c, d);
            break;
        case 1:
            fprintf(stdout, "0x%02x, 0x%02x, 0x%02x", a, b, c);
            break;
        case 2:
            fprintf(stdout, "0x%02x, 0x%02x", (a >> 3) | ((b & 0x1c) << 3),
                    (b >> 5) | (c & 0xf8));
            break;
        case 3:
            fprintf(stdout, "0x%02x, 0x%02x", (b & 0xf0) | (a >> 4),
                    (d & 0xf0) | (c >> 4));
            break;
        default:
            break;
        }
        if (i < len - 4) {
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

static void usage() {
    fprintf(stderr, "Usage:\timg2array [options] image [ > dest.h ]\n");
    fprintf(stderr, "\t-a STRING\tarray alignment prefix\n");
    fprintf(stderr, "\t-e STRING\tarray alignment suffix (default: "
                    "\"__attribute__ ((aligned (4)))\")\n");
    fprintf(stderr, "\t-n STRING\tarray name (default: \"image\")\n");
    fprintf(stderr, "\t-m INT\t\tdestination color mode (default 0):\n");
    for (uint8_t i = 0; i < NUM_COLOR_MODES; i++) {
        fprintf(stderr, "\t\t\t%u = %s\n", i, modes[i].id);
    }
}