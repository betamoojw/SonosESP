/*******************************************************************************
 * Size: 80 px
 * Bpp: 4
 * Opts: --font node_modules/@mdi/font/fonts/materialdesignicons-webfont.ttf --size 80 --format lvgl --bpp 4 --no-compress --range 0xF1ABC -o src/fonts/lv_font_mdi_80.c
 ******************************************************************************/

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef LV_FONT_MDI_80
#define LV_FONT_MDI_80 1
#endif

#if LV_FONT_MDI_80

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+F1ABC "󱪼" */
    0x8, 0x88, 0x88, 0x83, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1f, 0xff, 0xff, 0xf7,
    0x1f, 0xff, 0xff, 0xf7, 0x1, 0x11, 0x11, 0x10
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 1280, .box_w = 8, .box_h = 48, .ofs_x = 36, .ofs_y = 6}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 989884, .range_length = 1, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lv_font_mdi_80 = {
#else
lv_font_t lv_font_mdi_80 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 48,          /*The maximum line height required by the font*/
    .base_line = -6,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 1,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if LV_FONT_MDI_80*/

