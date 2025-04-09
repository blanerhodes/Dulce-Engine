#include <stdio.h>

/*
#define STB_RECT_PACK_IMPLEMENTATION
#include "../vendor/stb/stb_rect_pack.h" // optional, used for better bitmap packing

#define STB_TRUETYPE_IMPLEMENTATION
#include "../vendor/stb/stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../vendor/stb/stb_image_write.h"

#define NUM_SIZES 16
#define NUM_GLYPHS 95

void BuildFontAtlas(char* ttf_path, char* png_path) {
    // read font file
    FILE* fp = fopen(ttf_path, "rb");
    int ttf_size_max = 1e6; // most likely large enough, 1MB
    unsigned char* ttf_buffer = (unsigned char*)malloc(ttf_size_max);
    fread(ttf_buffer, 1, ttf_size_max, fp);
    fclose(fp);

    // setup glyph info stuff, check stb_truetype.h for definition of structs
    stbtt_packedchar glyph_metrics[16][95];
    stbtt_pack_range ranges[32] = { {72, 32, NULL, 95, glyph_metrics[0],  0, 0},
                                   {68, 32, NULL, 95, glyph_metrics[1],  0, 0},
                                   {64, 32, NULL, 95, glyph_metrics[2],  0, 0},
                                   {60, 32, NULL, 95, glyph_metrics[3],  0, 0},
                                   {56, 32, NULL, 95, glyph_metrics[4],  0, 0},
                                   {52, 32, NULL, 95, glyph_metrics[5],  0, 0},
                                   {48, 32, NULL, 95, glyph_metrics[6],  0, 0},
                                   {44, 32, NULL, 95, glyph_metrics[7],  0, 0},
                                   {40, 32, NULL, 95, glyph_metrics[8],  0, 0},
                                   {36, 32, NULL, 95, glyph_metrics[9],  0, 0},
                                   {32, 32, NULL, 95, glyph_metrics[10], 0, 0},
                                   {28, 32, NULL, 95, glyph_metrics[11], 0, 0},
                                   {24, 32, NULL, 95, glyph_metrics[12], 0, 0},
                                   {20, 32, NULL, 95, glyph_metrics[13], 0, 0},
                                   {16, 32, NULL, 95, glyph_metrics[14], 0, 0},
                                   {12, 32, NULL, 95, glyph_metrics[15], 0, 0} };

    // make a most likely large enough bitmap, adjust to font type, number of sizes and glyphs and oversampling
    int width = 1024;
    int max_height = 1024;
    unsigned char* bitmap = (unsigned char*)malloc(max_height * width);

    // do the packing, based on the ranges specified
    stbtt_pack_context pc;
    stbtt_PackBegin(&pc, bitmap, width, max_height, 0, 1, NULL);
    stbtt_PackSetOversampling(&pc, 1, 1); // say, choose 3x1 oversampling for subpixel positioning
    stbtt_PackFontRanges(&pc, ttf_buffer, 0, ranges, NUM_SIZES);
    stbtt_PackEnd(&pc);

    // get the global metrics for each size/range
    stbtt_fontinfo info;
    stbtt_InitFont(&info, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));

    float ascents[NUM_SIZES];
    float descents[NUM_SIZES];
    float linegaps[NUM_SIZES];

    for (int i = 0; i < NUM_SIZES; i++) {
        float size = ranges[i].font_size;
        float scale = stbtt_ScaleForPixelHeight(&info, ranges[i].font_size);
        int a, d, l;
        stbtt_GetFontVMetrics(&info, &a, &d, &l);

        ascents[i] = a * scale;
        descents[i] = d * scale;
        linegaps[i] = l * scale;
    }

    // calculate fill rate and crop the bitmap
    int filled = 0;
    int height = 0;
    for (int j = 0; j < NUM_SIZES; j++) {
        for (int i = 0; i < NUM_GLYPHS; i++) {
            if (glyph_metrics[j][i].y1 > height) height = glyph_metrics[j][i].y1;
            filled += (glyph_metrics[j][i].x1 - glyph_metrics[j][i].x0) * (glyph_metrics[j][i].y1 - glyph_metrics[j][i].y0);
        }
    }

    // create file
    printf("height = %d, fill rate = %.1f%%\n", height, 100 * filled / (double)(width * height)); fflush(stdout);
    stbi_write_png(png_path, width, height, 1, bitmap, 0);

    // print info
    if (0) {
        for (int j = 0; j < NUM_SIZES; j++) {
            printf("size    %.3f:\n", ranges[j].font_size);
            printf("ascent  %.3f:\n", ascents[j]);
            printf("descent %.3f:\n", descents[j]);
            printf("linegap %.3f:\n", linegaps[j]);
            stbtt_packedchar* m = glyph_metrics[j];
            for (int i = 0; i < NUM_GLYPHS; i++) {
                printf("    '%c':  (x0,y0) = (%4d,%4d),  (x1,y1) = (%4d,%4d),  (xoff,yoff) = (%+5.1f,%+5.1f),  (xoff2,yoff2) = (%+5.1f,%+5.1f),  xadvance = %.3f\n",
                    32 + i, m[i].x0, m[i].y0, m[i].x1, m[i].y1, m[i].xoff, m[i].yoff, m[i].xoff2, m[i].yoff2, m[i].xadvance);
            }
        }
    }

    // cleanup
    free(ttf_buffer);
    free(bitmap);
}*/