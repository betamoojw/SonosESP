#pragma once
/**
 * Material Design Icons (MDI 7.4) — player UI icon set
 *
 * Font files: src/fonts/lv_font_mdi_24.c  (24px, bpp4)  — top nav bar icons
 *             src/fonts/lv_font_mdi_32.c  (32px, bpp4)  — shuffle/repeat/volume
 *             src/fonts/lv_font_mdi_40.c  (40px, bpp4)  — play/pause + prev/next
 *
 * Generated with:
 *   npx lv_font_conv --font node_modules/@mdi/font/fonts/materialdesignicons-webfont.ttf
 *     --size 32|40 --format lvgl --bpp 4 --no-compress
 *     --range 0xF040A,0xF03E4,0xF04AE,0xF04AD,0xF049D,0xF0456,0xF0458,0xF057E,
 *             0xF0387,0xF0411,0xF0493,0xF004D,0xF1720,0xF0439,0xF0384
 *
 * Each macro is the UTF-8 byte sequence for the MDI codepoint.
 * Usage:  lv_label_set_text(lbl, MDI_PLAY);
 *         lv_obj_set_style_text_font(lbl, &lv_font_mdi_40, 0);
 */

// MDI codepoint → UTF-8 string literal
#define MDI_PLAY               "\xF3\xB0\x90\x8A"   // U+F040A  mdi-play
#define MDI_PAUSE              "\xF3\xB0\x8F\xA4"   // U+F03E4  mdi-pause
#define MDI_SKIP_PREV          "\xF3\xB0\x92\xAE"   // U+F04AE  mdi-skip-previous
#define MDI_SKIP_NEXT          "\xF3\xB0\x92\xAD"   // U+F04AD  mdi-skip-next
#define MDI_SHUFFLE            "\xF3\xB0\x92\x9D"   // U+F049D  mdi-shuffle
#define MDI_REPEAT             "\xF3\xB0\x91\x96"   // U+F0456  mdi-repeat
#define MDI_REPEAT_ONCE        "\xF3\xB0\x91\x98"   // U+F0458  mdi-repeat-once
#define MDI_VOLUME_HIGH        "\xF3\xB0\x95\xBE"   // U+F057E  mdi-volume-high
#define MDI_MUSIC_NOTE         "\xF3\xB0\x8E\x87"   // U+F0387  mdi-music-note
#define MDI_MUSIC_BOX          "\xF3\xB0\x8E\x84"   // U+F0384  mdi-music-box
#define MDI_PLAYLIST           "\xF3\xB0\x90\x91"   // U+F0411  mdi-playlist-play
#define MDI_COG                "\xF3\xB0\x92\x93"   // U+F0493  mdi-cog
#define MDI_ARROW_LEFT         "\xF3\xB0\x81\x8D"   // U+F004D  mdi-arrow-left
#define MDI_BROADCAST          "\xF3\xB1\x9C\xA0"   // U+F1720  mdi-broadcast
#define MDI_RADIO              "\xF3\xB0\x90\xB9"   // U+F0439  mdi-radio

LV_FONT_DECLARE(lv_font_mdi_24);
LV_FONT_DECLARE(lv_font_mdi_32);
LV_FONT_DECLARE(lv_font_mdi_40);
