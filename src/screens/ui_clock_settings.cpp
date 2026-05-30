/**
 * Clock Settings Screen — card-based dark theme.
 *
 * Sections grouped into 4 cards (Display, Photo Background, Time Zone, Weather)
 * for visual hierarchy and easier scanning vs the previous flat list. Same NVS
 * keys, same callbacks — no behavior change for existing settings.
 *
 * Adds optional manual location override (issue #74): the Weather card includes
 * a "Custom location..." entry in the city dropdown that reveals lat/lon/name
 * input fields. Saves to NVS on defocus; fetchClockWeather() reads from NVS to
 * stay race-free with the bg task.
 */

#include "ui_common.h"
#include "config.h"
#include "clock_screen.h"

// Forward declaration (defined in ui_sidebar.cpp)
lv_obj_t* createSettingsSidebar(lv_obj_t* screen, int activeIdx);

// ─────────────────────────────────────────────────────────────────────────────
// Local theme tokens (slightly elevated from screen bg = 0x121212)
// ─────────────────────────────────────────────────────────────────────────────
#define CLK_CARD_BG     lv_color_hex(0x1A1A1A)
#define CLK_CARD_BORDER lv_color_hex(0x2A2A2A)
#define CLK_INPUT_BG    lv_color_hex(0x222222)
#define CLK_INPUT_BORD  lv_color_hex(0x3A3A3A)

// ─────────────────────────────────────────────────────────────────────────────
// Helper — create a card container with a title + accent underline.
// Returns the card object; controls added inside it stack via flex column.
// ─────────────────────────────────────────────────────────────────────────────
static lv_obj_t* addCard(lv_obj_t* parent, const char* title) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, CLK_CARD_BG, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_border_color(card, CLK_CARD_BORDER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_set_style_margin_bottom(card, 14, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lbl = lv_label_create(card);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, COL_TEXT, 0);

    // Accent underline strip under the title
    lv_obj_t* underline = lv_obj_create(card);
    lv_obj_set_size(underline, 36, 2);
    lv_obj_set_style_bg_color(underline, COL_ACCENT, 0);
    lv_obj_set_style_bg_opa(underline, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(underline, 1, 0);
    lv_obj_set_style_border_width(underline, 0, 0);
    lv_obj_set_style_margin_bottom(underline, 4, 0);
    lv_obj_clear_flag(underline, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(underline, LV_OBJ_FLAG_CLICKABLE);

    return card;
}

static void addSettingLabel(lv_obj_t* parent, const char* text) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, COL_TEXT, 0);
    lv_obj_set_style_pad_top(lbl, 6, 0);
}

static void addDescLabel(lv_obj_t* parent, const char* text) {
    lv_obj_t* lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl, COL_TEXT2, 0);
    lv_obj_set_width(lbl, lv_pct(100));
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
}

static lv_obj_t* addSwitch(lv_obj_t* parent, bool initial) {
    lv_obj_t* sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 50, 26);
    lv_obj_set_style_margin_top(sw, 4, 0);
    lv_obj_set_style_radius(sw, 13, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, COL_ACCENT,
        (lv_style_selector_t)((uint32_t)LV_PART_INDICATOR | (uint32_t)LV_STATE_CHECKED));
    lv_obj_set_style_radius(sw, 13, LV_PART_INDICATOR);
    lv_obj_set_style_pad_all(sw, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw, COL_TEXT, LV_PART_KNOB);
    lv_obj_set_style_radius(sw, 11, LV_PART_KNOB);
    lv_obj_set_style_pad_all(sw, -3, LV_PART_KNOB);
    if (initial) lv_obj_add_state(sw, LV_STATE_CHECKED);
    return sw;
}

static lv_obj_t* makeDropdown(lv_obj_t* parent, const char* options,
                              uint16_t selected, bool open_up) {
    lv_obj_t* dd = lv_dropdown_create(parent);
    lv_dropdown_set_options(dd, options);
    lv_dropdown_set_selected(dd, selected);
    lv_obj_set_width(dd, lv_pct(100));
    lv_obj_set_style_bg_color(dd, CLK_INPUT_BG, 0);
    lv_obj_set_style_text_color(dd, COL_TEXT, 0);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_14, 0);
    lv_obj_set_style_border_color(dd, CLK_INPUT_BORD, 0);
    lv_obj_set_style_radius(dd, 8, 0);
    lv_obj_set_style_pad_all(dd, 10, 0);
    if (open_up) lv_dropdown_set_dir(dd, LV_DIR_TOP);
    lv_obj_t* list = lv_dropdown_get_list(dd);
    if (list) {
        lv_obj_set_height(list, 260);
        lv_obj_set_style_bg_color(list, lv_color_hex(0x222222), 0);
        lv_obj_set_style_text_color(list, COL_TEXT, 0);
        lv_obj_set_style_text_font(list, &lv_font_montserrat_14, 0);
        lv_obj_set_style_border_color(list, CLK_INPUT_BORD, 0);
    }
    return dd;
}

static lv_obj_t* makeSlider(lv_obj_t* parent, int min, int max, int value) {
    lv_obj_t* s = lv_slider_create(parent);
    lv_obj_set_width(s, lv_pct(100));
    lv_obj_set_height(s, 20);
    lv_slider_set_range(s, min, max);
    lv_slider_set_value(s, value, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s, COL_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s, COL_ACCENT, LV_PART_KNOB);
    lv_obj_set_style_radius(s, 10, LV_PART_MAIN);
    lv_obj_set_style_radius(s, 10, LV_PART_INDICATOR);
    lv_obj_set_style_pad_all(s, 2, LV_PART_KNOB);
    return s;
}

static lv_obj_t* makeTextarea(lv_obj_t* parent, const char* initial,
                              const char* accept_chars, int max_len,
                              const char* placeholder) {
    lv_obj_t* ta = lv_textarea_create(parent);
    lv_textarea_set_one_line(ta, true);
    if (initial && initial[0]) lv_textarea_set_text(ta, initial);
    if (placeholder) lv_textarea_set_placeholder_text(ta, placeholder);
    if (accept_chars) lv_textarea_set_accepted_chars(ta, accept_chars);
    if (max_len > 0) lv_textarea_set_max_length(ta, max_len);
    lv_obj_set_width(ta, lv_pct(100));
    lv_obj_set_style_bg_color(ta, CLK_INPUT_BG, 0);
    lv_obj_set_style_text_color(ta, COL_TEXT, 0);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_14, 0);
    lv_obj_set_style_border_color(ta, CLK_INPUT_BORD, 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_radius(ta, 8, 0);
    lv_obj_set_style_pad_all(ta, 10, 0);
    return ta;
}

// ─────────────────────────────────────────────────────────────────────────────
// Custom location sub-card — state shared between dropdown + textarea callbacks
// ─────────────────────────────────────────────────────────────────────────────
static lv_obj_t* custom_loc_card = nullptr;
static lv_obj_t* custom_lat_ta   = nullptr;
static lv_obj_t* custom_lon_ta   = nullptr;
static lv_obj_t* custom_name_ta  = nullptr;
static lv_obj_t* custom_kb       = nullptr;

// Show/hide sub-card based on city dropdown selection.
static void update_custom_loc_visibility(int city_idx) {
    if (!custom_loc_card) return;
    if (city_idx == CLOCK_LOC_CUSTOM_IDX) {
        lv_obj_clear_flag(custom_loc_card, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(custom_loc_card, LV_OBJ_FLAG_HIDDEN);
        if (custom_kb) lv_obj_add_flag(custom_kb, LV_OBJ_FLAG_HIDDEN);
    }
}

// Persist current textarea contents to NVS + globals. Called on DEFOCUS.
static void persist_custom_loc_from_textareas() {
    if (!custom_lat_ta || !custom_lon_ta || !custom_name_ta) return;
    const char* lat_s  = lv_textarea_get_text(custom_lat_ta);
    const char* lon_s  = lv_textarea_get_text(custom_lon_ta);
    const char* name_s = lv_textarea_get_text(custom_name_ta);
    clock_custom_lat  = lat_s;
    clock_custom_lon  = lon_s;
    clock_custom_name = name_s;
    wifiPrefs.putString(NVS_KEY_CLOCK_WX_CUSTOM_LAT,  lat_s);
    wifiPrefs.putString(NVS_KEY_CLOCK_WX_CUSTOM_LON,  lon_s);
    wifiPrefs.putString(NVS_KEY_CLOCK_WX_CUSTOM_NAME, name_s);
    clock_wx_valid = false;                  // force the bg task to re-render with new city
    clock_weather_needs_refetch = true;      // bg task picks this up next loop tick
    Serial.printf("[CLOCK] Custom location saved: %s, %s (%s)\n", lat_s, lon_s, name_s);
}

// Focus / defocus handler for the three custom-coords textareas.
static void custom_ta_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* ta = (lv_obj_t*)lv_event_get_target(e);
    if (!custom_kb) return;
    if (code == LV_EVENT_FOCUSED) {
        bool is_name = (ta == custom_name_ta);
        lv_keyboard_set_mode(custom_kb,
            is_name ? LV_KEYBOARD_MODE_TEXT_LOWER : LV_KEYBOARD_MODE_NUMBER);
        lv_keyboard_set_textarea(custom_kb, ta);
        lv_obj_clear_flag(custom_kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_scroll_to_view(ta, LV_ANIM_ON);  // bring the field above the keyboard
    } else if (code == LV_EVENT_DEFOCUSED) {
        persist_custom_loc_from_textareas();
        lv_obj_add_flag(custom_kb, LV_OBJ_FLAG_HIDDEN);
    }
}

// ============================================================================
// createClockSettingsScreen
// ============================================================================
void createClockSettingsScreen() {
    scr_clock_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_clock_settings, lv_color_hex(0x121212), 0);

    // Sidebar — Clock is index 6
    lv_obj_t* content = createSettingsSidebar(scr_clock_settings, 6);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_pad_row(content, 0, 0);

    // ── Screen title ─────────────────────────────────────────────────────────
    lv_obj_t* lbl_title = lv_label_create(content);
    lv_label_set_text(lbl_title, "Clock");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, COL_TEXT, 0);
    lv_obj_set_style_pad_bottom(lbl_title, 12, 0);

    // ────────────────────────────────────────────────────────────────────────
    // CARD 1 — Display
    // ────────────────────────────────────────────────────────────────────────
    {
        lv_obj_t* card = addCard(content, "Display");

        addSettingLabel(card, "Activate clock");
        addDescLabel(card, "Choose when the clock/screensaver should appear");
        lv_obj_t* dd_mode = makeDropdown(card,
            "Disabled\n"
            "After inactivity\n"
            "After inactivity (paused only)\n"
            "After inactivity (nothing playing)",
            (uint16_t)clock_mode, false);
        lv_obj_add_event_cb(dd_mode, [](lv_event_t* e) {
            lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
            clock_mode = (int)lv_dropdown_get_selected(dd);
            wifiPrefs.putInt(NVS_KEY_CLOCK_MODE, clock_mode);
        }, LV_EVENT_VALUE_CHANGED, NULL);

        addSettingLabel(card, "Inactivity timeout");
        static lv_obj_t* lbl_timeout_val;
        lbl_timeout_val = lv_label_create(card);
        lv_label_set_text_fmt(lbl_timeout_val, "%d min", clock_timeout_min);
        lv_obj_set_style_text_color(lbl_timeout_val, COL_ACCENT, 0);
        lv_obj_set_style_text_font(lbl_timeout_val, &lv_font_montserrat_14, 0);

        lv_obj_t* sl_timeout = makeSlider(card, 1, 60, clock_timeout_min);
        lv_obj_add_event_cb(sl_timeout, [](lv_event_t* e) {
            lv_obj_t* s = (lv_obj_t*)lv_event_get_target(e);
            clock_timeout_min = lv_slider_get_value(s);
            lv_label_set_text_fmt((lv_obj_t*)lv_event_get_user_data(e),
                                  "%d min", clock_timeout_min);
            wifiPrefs.putInt(NVS_KEY_CLOCK_TIMEOUT, clock_timeout_min);
        }, LV_EVENT_VALUE_CHANGED, lbl_timeout_val);

        addSettingLabel(card, "12-hour format (AM/PM)");
        addDescLabel(card, "Off = 24-hour clock");
        lv_obj_t* sw_12h = addSwitch(card, clock_12h);
        lv_obj_add_event_cb(sw_12h, [](lv_event_t* e) {
            lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
            clock_12h = lv_obj_has_state(sw, LV_STATE_CHECKED);
            wifiPrefs.putBool(NVS_KEY_CLOCK_12H, clock_12h);
        }, LV_EVENT_VALUE_CHANGED, NULL);
    }

    // ────────────────────────────────────────────────────────────────────────
    // CARD 2 — Photo Background
    // ────────────────────────────────────────────────────────────────────────
    {
        lv_obj_t* card = addCard(content, "Photo Background");

        addSettingLabel(card, "Enable random photos");
        addDescLabel(card, "Random photos from Flickr via loremflickr.com (requires WiFi)");
        lv_obj_t* sw_picsum = addSwitch(card, clock_picsum_enabled);
        lv_obj_add_event_cb(sw_picsum, [](lv_event_t* e) {
            lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
            clock_picsum_enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
            wifiPrefs.putBool(NVS_KEY_CLOCK_PICSUM, clock_picsum_enabled);
        }, LV_EVENT_VALUE_CHANGED, NULL);

        addSettingLabel(card, "Photo theme");
        static char kw_opts[256];
        kw_opts[0] = '\0';
        for (int i = 0; i < CLOCK_BG_KW_COUNT; i++) {
            strncat(kw_opts, CLOCK_BG_KEYWORDS[i].label, sizeof(kw_opts) - strlen(kw_opts) - 2);
            if (i < CLOCK_BG_KW_COUNT - 1)
                strncat(kw_opts, "\n", sizeof(kw_opts) - strlen(kw_opts) - 1);
        }
        lv_obj_t* dd_kw = makeDropdown(card, kw_opts, (uint16_t)clock_bg_kw_idx, false);
        lv_obj_add_event_cb(dd_kw, [](lv_event_t* e) {
            lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
            clock_bg_kw_idx = (int)lv_dropdown_get_selected(dd);
            wifiPrefs.putInt(NVS_KEY_CLOCK_KW, clock_bg_kw_idx);
        }, LV_EVENT_VALUE_CHANGED, NULL);

        addSettingLabel(card, "Photo refresh interval");
        static lv_obj_t* lbl_refresh_val;
        lbl_refresh_val = lv_label_create(card);
        lv_label_set_text_fmt(lbl_refresh_val, "%d min", clock_refresh_min);
        lv_obj_set_style_text_color(lbl_refresh_val, COL_ACCENT, 0);
        lv_obj_set_style_text_font(lbl_refresh_val, &lv_font_montserrat_14, 0);

        lv_obj_t* sl_refresh = makeSlider(card, 1, 60, clock_refresh_min);
        lv_obj_add_event_cb(sl_refresh, [](lv_event_t* e) {
            lv_obj_t* s = (lv_obj_t*)lv_event_get_target(e);
            clock_refresh_min = lv_slider_get_value(s);
            lv_label_set_text_fmt((lv_obj_t*)lv_event_get_user_data(e),
                                  "%d min", clock_refresh_min);
            wifiPrefs.putInt(NVS_KEY_CLOCK_REFRESH, clock_refresh_min);
        }, LV_EVENT_VALUE_CHANGED, lbl_refresh_val);
    }

    // ────────────────────────────────────────────────────────────────────────
    // CARD 3 — Time Zone
    // ────────────────────────────────────────────────────────────────────────
    {
        lv_obj_t* card = addCard(content, "Time Zone");

        addDescLabel(card, "Select your local timezone");
        static char tz_opts[4096];
        tz_opts[0] = '\0';
        for (int i = 0; i < CLOCK_ZONES_COUNT; i++) {
            strncat(tz_opts, CLOCK_ZONES[i].name, sizeof(tz_opts) - strlen(tz_opts) - 2);
            if (i < CLOCK_ZONES_COUNT - 1) {
                strncat(tz_opts, "\n", sizeof(tz_opts) - strlen(tz_opts) - 1);
            }
        }
        lv_obj_t* dd_tz = makeDropdown(card, tz_opts, (uint16_t)clock_tz_idx, true);
        lv_obj_add_event_cb(dd_tz, [](lv_event_t* e) {
            lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
            clock_tz_idx = (int)lv_dropdown_get_selected(dd);
            wifiPrefs.putInt(NVS_KEY_CLOCK_TZ, clock_tz_idx);
            setenv("TZ", CLOCK_ZONES[clock_tz_idx].posix, 1);
            tzset();
            Serial.printf("[CLOCK] TZ set to %s (%s)\n",
                          CLOCK_ZONES[clock_tz_idx].name,
                          CLOCK_ZONES[clock_tz_idx].posix);
        }, LV_EVENT_VALUE_CHANGED, NULL);
    }

    // ────────────────────────────────────────────────────────────────────────
    // CARD 4 — Weather (+ Custom location sub-card per issue #74)
    // ────────────────────────────────────────────────────────────────────────
    {
        lv_obj_t* card = addCard(content, "Weather");

        addSettingLabel(card, "Enable widget");
        addDescLabel(card, "Temperature, humidity, wind, and 6-hour forecast (Open-Meteo, no API key needed)");
        lv_obj_t* sw_weather = addSwitch(card, clock_weather_enabled);
        lv_obj_add_event_cb(sw_weather, [](lv_event_t* e) {
            lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
            clock_weather_enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
            wifiPrefs.putBool(NVS_KEY_CLOCK_WEATHER_EN, clock_weather_enabled);
        }, LV_EVENT_VALUE_CHANGED, NULL);

        addSettingLabel(card, "Location");
        addDescLabel(card, "Auto-detect uses your public IP. Pick \"Custom location\" to enter your own coordinates.");

        // Build city dropdown: predefined cities + "Custom location..." appended at the end.
        // CLOCK_LOC_CUSTOM_IDX == CLOCK_CITY_COUNT, so the appended entry is exactly at that index.
        static char city_opts[2048];
        city_opts[0] = '\0';
        for (int i = 0; i < CLOCK_CITY_COUNT; i++) {
            strncat(city_opts, CLOCK_CITIES[i].label, sizeof(city_opts) - strlen(city_opts) - 2);
            strncat(city_opts, "\n", sizeof(city_opts) - strlen(city_opts) - 1);
        }
        strncat(city_opts, "Custom location...", sizeof(city_opts) - strlen(city_opts) - 1);

        lv_obj_t* dd_city = makeDropdown(card, city_opts, (uint16_t)clock_weather_city_idx, true);
        lv_obj_add_event_cb(dd_city, [](lv_event_t* e) {
            lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
            clock_weather_city_idx = (int)lv_dropdown_get_selected(dd);
            wifiPrefs.putInt(NVS_KEY_CLOCK_WEATHER_CITY, clock_weather_city_idx);
            clock_wx_valid = false;
            clock_weather_needs_refetch = true;
            update_custom_loc_visibility(clock_weather_city_idx);
        }, LV_EVENT_VALUE_CHANGED, NULL);

        // ── Custom location sub-card (issue #74) ─────────────────────────────
        // Lives inside the Weather card; hidden unless the user picks Custom.
        custom_loc_card = lv_obj_create(card);
        lv_obj_set_width(custom_loc_card, lv_pct(100));
        lv_obj_set_height(custom_loc_card, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(custom_loc_card, lv_color_hex(0x121212), 0);
        lv_obj_set_style_bg_opa(custom_loc_card, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(custom_loc_card, 10, 0);
        lv_obj_set_style_border_color(custom_loc_card, COL_ACCENT, 0);
        lv_obj_set_style_border_width(custom_loc_card, 1, 0);
        lv_obj_set_style_pad_all(custom_loc_card, 12, 0);
        lv_obj_set_style_pad_row(custom_loc_card, 6, 0);
        lv_obj_set_style_margin_top(custom_loc_card, 8, 0);
        lv_obj_set_flex_flow(custom_loc_card, LV_FLEX_FLOW_COLUMN);
        lv_obj_clear_flag(custom_loc_card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* sub_title = lv_label_create(custom_loc_card);
        lv_label_set_text(sub_title, "Custom coordinates");
        lv_obj_set_style_text_font(sub_title, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(sub_title, COL_ACCENT, 0);

        addSettingLabel(custom_loc_card, "Latitude (-90 to 90)");
        custom_lat_ta = makeTextarea(custom_loc_card,
            clock_custom_lat.c_str(), "0123456789.-", 12, "e.g. 45.5017");
        lv_obj_add_event_cb(custom_lat_ta, custom_ta_event_cb, LV_EVENT_FOCUSED,   NULL);
        lv_obj_add_event_cb(custom_lat_ta, custom_ta_event_cb, LV_EVENT_DEFOCUSED, NULL);

        addSettingLabel(custom_loc_card, "Longitude (-180 to 180)");
        custom_lon_ta = makeTextarea(custom_loc_card,
            clock_custom_lon.c_str(), "0123456789.-", 12, "e.g. -73.5673");
        lv_obj_add_event_cb(custom_lon_ta, custom_ta_event_cb, LV_EVENT_FOCUSED,   NULL);
        lv_obj_add_event_cb(custom_lon_ta, custom_ta_event_cb, LV_EVENT_DEFOCUSED, NULL);

        addSettingLabel(custom_loc_card, "City name (display only)");
        custom_name_ta = makeTextarea(custom_loc_card,
            clock_custom_name.c_str(), nullptr, 60, "e.g. Montreal");
        lv_obj_add_event_cb(custom_name_ta, custom_ta_event_cb, LV_EVENT_FOCUSED,   NULL);
        lv_obj_add_event_cb(custom_name_ta, custom_ta_event_cb, LV_EVENT_DEFOCUSED, NULL);

        addDescLabel(custom_loc_card,
            "Saved automatically when you tap outside the field. Get coordinates from any map app.");

        // Apply initial visibility based on the current saved selection
        update_custom_loc_visibility(clock_weather_city_idx);

        // ── Temperature unit ────────────────────────────────────────────────
        addSettingLabel(card, "Temperature unit");
        addDescLabel(card, "On = Fahrenheit (°F), Off = Celsius (°C)");
        lv_obj_t* sw_fahr = addSwitch(card, clock_wx_fahrenheit);
        lv_obj_add_event_cb(sw_fahr, [](lv_event_t* e) {
            lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
            clock_wx_fahrenheit = lv_obj_has_state(sw, LV_STATE_CHECKED);
            wifiPrefs.putBool(NVS_KEY_CLOCK_WEATHER_FAHR, clock_wx_fahrenheit);
            clock_weather_needs_refetch = true;
        }, LV_EVENT_VALUE_CHANGED, NULL);
    }

    // ─── Floating numeric/text keyboard ─────────────────────────────────────
    // Created at screen level so it overlays content. Hidden until a custom-
    // coords textarea is focused; the focus event-cb wires textarea+mode.
    custom_kb = lv_keyboard_create(scr_clock_settings);
    lv_obj_set_size(custom_kb, lv_pct(100), 220);
    lv_obj_align(custom_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(custom_kb, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_mode(custom_kb, LV_KEYBOARD_MODE_NUMBER);
}
