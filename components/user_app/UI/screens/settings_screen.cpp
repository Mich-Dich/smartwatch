#include "util/pch.hpp"
#include "settings_screen.hpp"

#include "user_app.hpp"
#include "UI/util.hpp"
#include "UI/screen_manager.hpp"
#include "UI/screens/main_screen.hpp"


// FORWARD DECLARATIONS ================================================================================================

extern "C" {

    void setBrightens(uint8_t brig);

}


namespace APP::UI {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    // CLASS IMPLEMENTATION ============================================================================================

    settings_screen::settings_screen() = default;


    settings_screen::~settings_screen() { destroy(); }


    // CLASS PUBLIC ====================================================================================================

    void settings_screen::init() {

        // Create the root screen – same as bluetooth/wifi
        m_screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(m_screen, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(m_screen, LV_OPA_COVER, 0);
        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);

        // ---- Geometric background (exactly like bluetooth_screen) ----
        APP::UI::util::create_geometric_pattern_1(
            m_screen,
            lv_color_mix(highlight_color, lv_color_hex(0x000000), 80),
            lv_color_mix(support_color, lv_color_hex(0x000000), 80)
        );

        // ---- Top status label (creates a buffer for swipe‑down gestures) ----
        lv_obj_t* title_label = lv_label_create(m_screen);
        lv_label_set_text(title_label, "Settings");
        lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(title_label, &inconsolata_regular_26, 0);
        lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);

        // ---- Brightness section (moved down to make room) ----
        m_label = lv_label_create(m_screen);
        lv_label_set_text(m_label, "Brightness");
        lv_obj_set_style_text_color(m_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(m_label, LV_ALIGN_TOP_MID, 0, 70);        // was 20

        m_slider = lv_slider_create(m_screen);
        lv_slider_set_range(m_slider, 0, 255);
        lv_slider_set_value(m_slider, 128, LV_ANIM_OFF);
        lv_obj_set_width(m_slider, 200);
        lv_obj_align(m_slider, LV_ALIGN_TOP_MID, 0, 110);      // was 60
        lv_obj_add_event_cb(m_slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, this);

        // ---- Color selectors (moved down) ----
        lv_obj_t* color_container = lv_obj_create(m_screen);
        lv_obj_set_size(color_container, LV_PCT(90), LV_SIZE_CONTENT);
        lv_obj_align(color_container, LV_ALIGN_TOP_MID, 0, 160); // was 120
        lv_obj_set_style_bg_opa(color_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(color_container, 0, 0);
        lv_obj_clear_flag(color_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(color_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(color_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // ---- Highlight color wheel ----
        lv_obj_t* highlight_group = lv_obj_create(color_container);
        lv_obj_set_size(highlight_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(highlight_group, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(highlight_group, 0, 0);
        lv_obj_set_flex_flow(highlight_group, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(highlight_group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        m_highlight_label = lv_label_create(highlight_group);
        lv_label_set_text(m_highlight_label, "Highlight");
        lv_obj_set_style_text_color(m_highlight_label, lv_color_hex(0xFFFFFF), 0);

        m_highlight_wheel = lv_colorwheel_create(highlight_group, true);
        lv_obj_set_size(m_highlight_wheel, 100, 100);
        lv_colorwheel_set_mode(m_highlight_wheel, LV_COLORWHEEL_MODE_HUE);
        lv_obj_add_event_cb(m_highlight_wheel, color_wheel_event_cb, LV_EVENT_VALUE_CHANGED, this);

        m_highlight_preview = lv_obj_create(highlight_group);
        lv_obj_set_size(m_highlight_preview, 40, 20);
        lv_obj_set_style_border_width(m_highlight_preview, 1, 0);
        lv_obj_set_style_border_color(m_highlight_preview, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_color(m_highlight_preview, highlight_color, 0);
        lv_obj_set_style_bg_opa(m_highlight_preview, LV_OPA_COVER, 0);

        // ---- Support color wheel ----
        lv_obj_t* support_group = lv_obj_create(color_container);
        lv_obj_set_size(support_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(support_group, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(support_group, 0, 0);
        lv_obj_set_flex_flow(support_group, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(support_group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        m_support_label = lv_label_create(support_group);
        lv_label_set_text(m_support_label, "Support");
        lv_obj_set_style_text_color(m_support_label, lv_color_hex(0xFFFFFF), 0);

        m_support_wheel = lv_colorwheel_create(support_group, true);
        lv_obj_set_size(m_support_wheel, 100, 100);
        lv_colorwheel_set_mode(m_support_wheel, LV_COLORWHEEL_MODE_HUE);
        lv_obj_add_event_cb(m_support_wheel, color_wheel_event_cb, LV_EVENT_VALUE_CHANGED, this);

        m_support_preview = lv_obj_create(support_group);
        lv_obj_set_size(m_support_preview, 40, 20);
        lv_obj_set_style_border_width(m_support_preview, 1, 0);
        lv_obj_set_style_border_color(m_support_preview, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_color(m_support_preview, support_color, 0);
        lv_obj_set_style_bg_opa(m_support_preview, LV_OPA_COVER, 0);


        // ---- Time setting section ----
        m_time_container = lv_obj_create(m_screen);
        lv_obj_set_size(m_time_container, LV_PCT(90), LV_SIZE_CONTENT);
        lv_obj_align(m_time_container, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_set_style_bg_opa(m_time_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(m_time_container, 0, 0);
        lv_obj_set_flex_flow(m_time_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(m_time_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Label above the rollers? We'll put a small label in the flex row.
        // Instead, we can have a separate label above the row, but we'll keep it simple.
        // We'll create three items: Hours, Minutes, and Set button.

        // ---- Hours roller ----
        lv_obj_t* hour_group = lv_obj_create(m_time_container);
        lv_obj_set_size(hour_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(hour_group, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(hour_group, 0, 0);
        lv_obj_set_flex_flow(hour_group, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(hour_group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* hour_label = lv_label_create(hour_group);
        lv_label_set_text(hour_label, "H");
        lv_obj_set_style_text_color(hour_label, lv_color_hex(0xFFFFFF), 0);

        m_hour_roller = lv_roller_create(hour_group);
        // Generate options "00\n01\n...\n23"
        std::string hour_opts;
        for (int i = 0; i < 24; ++i) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02d", i);
            hour_opts += buf;
            hour_opts += "\n";
        }
        hour_opts.pop_back(); // remove trailing newline
        lv_roller_set_options(m_hour_roller, hour_opts.c_str(), LV_ROLLER_MODE_NORMAL);
        lv_obj_set_size(m_hour_roller, 40, 80);

        // ---- Minutes roller ----
        lv_obj_t* minute_group = lv_obj_create(m_time_container);
        lv_obj_set_size(minute_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(minute_group, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(minute_group, 0, 0);
        lv_obj_set_flex_flow(minute_group, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(minute_group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* minute_label = lv_label_create(minute_group);
        lv_label_set_text(minute_label, "M");
        lv_obj_set_style_text_color(minute_label, lv_color_hex(0xFFFFFF), 0);

        m_minute_roller = lv_roller_create(minute_group);
        std::string min_opts;
        for (int i = 0; i < 60; ++i) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02d", i);
            min_opts += buf;
            min_opts += "\n";
        }
        min_opts.pop_back();
        lv_roller_set_options(m_minute_roller, min_opts.c_str(), LV_ROLLER_MODE_NORMAL);
        lv_obj_set_size(m_minute_roller, 40, 80);

        // ---- Set button ----
        m_set_time_btn = lv_btn_create(m_time_container);
        lv_obj_set_size(m_set_time_btn, 50, 50);
        lv_obj_add_event_cb(m_set_time_btn, set_time_btn_event_cb, LV_EVENT_CLICKED, this);

        lv_obj_t* btn_label = lv_label_create(m_set_time_btn);
        lv_label_set_text(btn_label, "Set");
        lv_obj_center(btn_label);


        // Initially hidden (screen_manager will show it)
        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);

        ESP_LOGI(TAG, "Settings screen initialised");
    }


    void settings_screen::show() {

        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_HIDDEN);

        // Sync color wheels with current global colors
        if (m_highlight_wheel) {

            lv_color_t col = highlight_color;
            lv_colorwheel_set_rgb(m_highlight_wheel, col);
            lv_obj_set_style_bg_color(m_highlight_preview, col, 0);
        }

        if (m_support_wheel) {

            lv_color_t col = support_color;
            lv_colorwheel_set_rgb(m_support_wheel, col);
            lv_obj_set_style_bg_color(m_support_preview, col, 0);
        }


        // Sync time rollers to current system time
        time_t now = time(nullptr);
        struct tm tm_info;
        localtime_r(&now, &tm_info);
        // If time is valid (year > 1970), use it; otherwise default 0
        int hour = (tm_info.tm_year > 70) ? tm_info.tm_hour : 0;
        int minute = (tm_info.tm_year > 70) ? tm_info.tm_min : 0;

        if (m_hour_roller)
            lv_roller_set_selected(m_hour_roller, hour, LV_ANIM_OFF);

        if (m_minute_roller)
            lv_roller_set_selected(m_minute_roller, minute, LV_ANIM_OFF);
    }


    void settings_screen::hide()                        { lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN); }


    lv_obj_t* settings_screen::get_root()               { return m_screen; }


    void settings_screen::destroy() {

        if (m_screen) {
            lv_obj_del(m_screen);
            m_screen = nullptr;
        }
    }


    bool settings_screen::handle_event(lv_event_t* e)   { return false; }


    // CLASS PRIVATE ===================================================================================================

    void settings_screen::slider_event_cb(lv_event_t* e) {

        lv_obj_t* slider = lv_event_get_target(e);
        uint8_t value = lv_slider_get_value(slider);
        setBrightens(value);
    }


    void settings_screen::color_wheel_event_cb(lv_event_t* e) {

        lv_obj_t* wheel = lv_event_get_target(e);
        lv_color_t color = lv_colorwheel_get_rgb(wheel);

        settings_screen* self = (settings_screen*)lv_event_get_user_data(e);
        if (!self)
            return;

        if (wheel == self->m_highlight_wheel) {

            set_highlight_color(color);
            lv_obj_set_style_bg_color(self->m_highlight_preview, color, 0);

        } else if (wheel == self->m_support_wheel) {

            set_support_color(color);
            lv_obj_set_style_bg_color(self->m_support_preview, color, 0);
        }
    }


    void settings_screen::set_time_btn_event_cb(lv_event_t* e) {

        settings_screen* self = (settings_screen*)lv_event_get_user_data(e);
        if (!self)
            return;

        // Get selected strings from rollers
        char hour_str[3] = {0};
        char min_str[3] = {0};
        lv_roller_get_selected_str(self->m_hour_roller, hour_str, sizeof(hour_str));
        lv_roller_get_selected_str(self->m_minute_roller, min_str, sizeof(min_str));

        int hour = atoi(hour_str);
        int minute = atoi(min_str);

        // Validate
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {

            ESP_LOGW(TAG, "Invalid time values");
            return;
        }

        // Create clock struct and update main screen
        APP::clock new_time;
        new_time.hours = hour;
        new_time.minutes = minute;
        new_time.seconds = 0;   // seconds are set to 0 on manual set
        APP::UI::main_screen::set_clock(new_time);

        ESP_LOGI(TAG, "Manual time set to %02d:%02d:00", hour, minute);
    }

}
