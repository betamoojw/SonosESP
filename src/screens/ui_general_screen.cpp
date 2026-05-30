/**
 * General Settings Screen — card-based dark theme.
 *
 * Currently a single "Lyrics" card with the synced-lyrics toggle. Uses the
 * shared card helpers from ui_settings_card.h so this stays consistent with
 * the Clock settings screen.
 */

#include "ui_common.h"
#include "config.h"
#include "lyrics.h"
#include "ui_settings_card.h"

// Forward declaration (defined in ui_sidebar.cpp)
lv_obj_t* createSettingsSidebar(lv_obj_t* screen, int activeIdx);

void createGeneralScreen() {
    scr_general = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_general, lv_color_hex(0x121212), 0);

    // Sidebar — General is index 0
    lv_obj_t* content = createSettingsSidebar(scr_general, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_pad_row(content, 0, 0);

    // ── Screen title ─────────────────────────────────────────────────────────
    lv_obj_t* lbl_title = lv_label_create(content);
    lv_label_set_text(lbl_title, "General");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
    lv_obj_set_style_pad_bottom(lbl_title, 12, 0);

    // ────────────────────────────────────────────────────────────────────────
    // CARD — Lyrics
    // ────────────────────────────────────────────────────────────────────────
    {
        lv_obj_t* card = addCard(content, "Lyrics");

        addSettingLabel(card, "Show synced lyrics");
        addDescLabel(card, "Display time-synced lyrics over the album art (LRCLIB, no API key)");

        lv_obj_t* sw_lyrics = addSwitch(card, lyrics_enabled);
        lv_obj_add_event_cb(sw_lyrics, [](lv_event_t* e) {
            lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
            lyrics_enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
            wifiPrefs.putBool("lyrics", lyrics_enabled);
            setLyricsVisible(lyrics_enabled && lyrics_ready);
        }, LV_EVENT_VALUE_CHANGED, NULL);
    }
}
