#include "util/pch.hpp"
#include "settings_screen.hpp"

#include "user_app.hpp"
#include "UI/util.hpp"
#include "UI/screen_manager.hpp"
#include "UI/screens/main_screen.hpp"


// FORWARD DECLARATIONS ================================================================================================

extern "C" {

    void setBrightens(u16 brig);

    u16 getBrightens();

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

    settings_screen::settings_screen()          = default;


    settings_screen::~settings_screen()         { destroy(); }

    // CLASS PUBLIC ====================================================================================================

    void settings_screen::init() {

        // Root screen ----------------------------------------------------------------
        m_screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(m_screen, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(m_screen, LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);

        create_ui_elements();                               // Build all child UI elements
        sync_ui();                                          // Sync controls to current state (brightness, colours, time)

        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);      // Hide until shown
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


    void settings_screen::hide() {

        if (m_screen)
            lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);

        if (m_pending_recreate) {
            APP::UI::screen_manager::recreate_screens();
            m_pending_recreate = false;
        }
    }


    void settings_screen::destroy() {

        void* buffer_to_free = nullptr;     // Retrieve buffer before deleting the screen
        if (m_pattern_canvas)
            buffer_to_free = lv_obj_get_user_data(m_pattern_canvas);

        if (m_screen)                       // Delete the screen (which deletes all children, including the canvas)
            lv_obj_del(m_screen);

        if (buffer_to_free)                 // Now free the buffer
            lv_mem_free(buffer_to_free);

        buffer_to_free = nullptr;
        m_screen = nullptr;
        m_pattern_canvas = nullptr;
    }


    void settings_screen::recreate_ui() {

        if (!m_screen)
            return;

        void* buffer_to_free = nullptr;
        if (m_pattern_canvas)
            buffer_to_free = lv_obj_get_user_data(m_pattern_canvas);

        // Delete all children of the screen
        uint32_t child_cnt = lv_obj_get_child_cnt(m_screen);
        std::vector<lv_obj_t*> to_delete;
        to_delete.reserve(child_cnt);
        for (uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t* child = lv_obj_get_child(m_screen, i);
            to_delete.push_back(child);
        }

        for (lv_obj_t* child : to_delete)
            lv_obj_del(child);

        // Now free the buffer (safe, as canvas is gone)
        if (buffer_to_free)
            lv_mem_free(buffer_to_free);

        buffer_to_free = nullptr;

        // Reset pointers that will be re‑assigned
        m_pattern_canvas = nullptr;
        m_pattern_canvas = nullptr;
        m_slider = nullptr;
        m_brightness_value_label = nullptr;
        m_highlight_wheel = nullptr;
        m_highlight_preview = nullptr;
        m_highlight_label = nullptr;
        m_support_wheel = nullptr;
        m_support_preview = nullptr;
        m_support_label = nullptr;
        m_hour_roller = nullptr;
        m_minute_roller = nullptr;
        m_set_time_btn = nullptr;

        create_ui_elements();                   // Recreate the UI with the new colours
        sync_ui();                              // Sync all controls to current state

        ESP_LOGI(TAG, "Settings UI recreated");
    }


    lv_obj_t* settings_screen::get_root()               { return m_screen; }


    bool settings_screen::handle_event(lv_event_t* e)   { return false; }


    // CLASS PRIVATE ===================================================================================================

    void settings_screen::create_ui_elements() {

        // Geometric background (same as other screens)
        m_pattern_canvas = APP::UI::util::create_geometric_pattern_1(
            m_screen,
            lv_color_mix(highlight_color, lv_color_hex(0x000000), 80),
            lv_color_mix(support_color, lv_color_hex(0x000000), 80)
        );

        // Fixed title ----------------------------------------------------------------
        lv_obj_t* title_label = lv_label_create(m_screen);
        lv_label_set_text(title_label, "Settings");
        lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(title_label, &inconsolata_regular_26, 0);
        lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);

        // Scrollable container --------------------------------------------------------
        lv_obj_t* scroll = lv_obj_create(m_screen);
        lv_obj_set_size(scroll, LV_PCT(100), LV_PCT(82));
        lv_obj_align(scroll, LV_ALIGN_TOP_MID, 0, 60);
        lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(scroll, 0, 0);
        lv_obj_set_style_border_side(scroll, LV_BORDER_SIDE_NONE, 0);
        lv_obj_set_style_outline_width(scroll, 0, 0);
        lv_obj_set_style_outline_opa(scroll, LV_OPA_TRANSP, 0);
        lv_obj_set_style_pad_all(scroll, 10, 0);
        lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(scroll, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(scroll, 20, 0);
        lv_obj_add_flag(scroll, LV_OBJ_FLAG_SCROLLABLE);

        {   // Brightness --------------------------------------------------------------
            lv_obj_t* brightness_cont = lv_obj_create(scroll);
            lv_obj_set_size(brightness_cont, LV_PCT(90), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(brightness_cont, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(brightness_cont, 0, 0);
            lv_obj_set_style_border_side(brightness_cont, LV_BORDER_SIDE_NONE, 0);
            lv_obj_set_style_outline_width(brightness_cont, 0, 0);
            lv_obj_set_style_outline_opa(brightness_cont, LV_OPA_TRANSP, 0);
            lv_obj_set_flex_flow(brightness_cont, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(brightness_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_row(brightness_cont, 6, 0);

            lv_obj_t* brightness_label = lv_label_create(brightness_cont);
            lv_label_set_text(brightness_label, "Brightness");
            lv_obj_set_style_text_color(brightness_label, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(brightness_label, &inconsolata_regular_26, 0);

            lv_obj_t* slider_row = lv_obj_create(brightness_cont);
            lv_obj_set_size(slider_row, LV_PCT(95), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(slider_row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(slider_row, 0, 0);
            lv_obj_set_style_border_side(slider_row, LV_BORDER_SIDE_NONE, 0);
            lv_obj_set_style_outline_width(slider_row, 0, 0);
            lv_obj_set_style_outline_opa(slider_row, LV_OPA_TRANSP, 0);
            lv_obj_set_flex_flow(slider_row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(slider_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_all(slider_row, 0, 0);

            m_slider = lv_slider_create(slider_row);
            lv_obj_set_width(m_slider, 180);
            lv_obj_set_style_radius(m_slider, 0, 0);
            lv_obj_set_style_bg_color(m_slider, lv_color_hex(0x333333), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(m_slider, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_radius(m_slider, 0, LV_PART_INDICATOR);
            lv_obj_set_style_bg_color(m_slider, highlight_color, LV_PART_INDICATOR);
            lv_obj_set_style_radius(m_slider, 0, LV_PART_KNOB);
            lv_obj_set_style_bg_color(m_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
            lv_obj_set_style_border_width(m_slider, 0, LV_PART_KNOB);
            lv_slider_set_range(m_slider, 0, 255);
            // Use getBrightens() for initial value
            u16 initial_brightness = getBrightens();
            lv_slider_set_value(m_slider, initial_brightness, LV_ANIM_OFF);
            lv_obj_add_event_cb(m_slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, this);

            // m_brightness_value_label = lv_label_create(slider_row);
            // char buf[8];
            // snprintf(buf, sizeof(buf), "%d%%", (int)(initial_brightness * 100 / 255));
            // lv_label_set_text(m_brightness_value_label, buf);
            // lv_obj_set_style_text_color(m_brightness_value_label, lv_color_hex(0xFFFFFF), 0);
            // lv_obj_set_style_text_font(m_brightness_value_label, &inconsolata_regular_26, 0);
            // lv_obj_set_style_pad_left(m_brightness_value_label, 10, 0);
        }

        {   // Color wheels – stacked vertically ---------------------------------------
            lv_obj_t* color_cont = lv_obj_create(scroll);
            lv_obj_set_size(color_cont, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(color_cont, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(color_cont, 0, 0);
            lv_obj_set_style_border_side(color_cont, LV_BORDER_SIDE_NONE, 0);
            lv_obj_set_style_outline_width(color_cont, 0, 0);
            lv_obj_set_style_outline_opa(color_cont, LV_OPA_TRANSP, 0);
            lv_obj_set_flex_flow(color_cont, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(color_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_row(color_cont, 15, 0);  // spacing between the two wheels

            // Highlight wheel ---------------------------------------------------------
            lv_obj_t* highlight_group = lv_obj_create(color_cont);
            lv_obj_set_size(highlight_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(highlight_group, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(highlight_group, 0, 0);
            lv_obj_set_style_border_side(highlight_group, LV_BORDER_SIDE_NONE, 0);
            lv_obj_set_style_outline_width(highlight_group, 0, 0);
            lv_obj_set_style_outline_opa(highlight_group, LV_OPA_TRANSP, 0);
            lv_obj_set_flex_flow(highlight_group, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(highlight_group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            m_highlight_label = lv_label_create(highlight_group);
            lv_label_set_text(m_highlight_label, "Main Color");
            lv_obj_set_style_text_color(m_highlight_label, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(m_highlight_label, &inconsolata_regular_26, 0);

            m_highlight_wheel = lv_colorwheel_create(highlight_group, true);
            lv_obj_set_size(m_highlight_wheel, 180, 180);
            lv_obj_set_style_radius(m_highlight_wheel, 0, 0);
            lv_colorwheel_set_mode(m_highlight_wheel, LV_COLORWHEEL_MODE_HUE);
            lv_obj_add_event_cb(m_highlight_wheel, color_wheel_event_cb, LV_EVENT_VALUE_CHANGED, this);
            lv_obj_add_event_cb(m_highlight_wheel, color_wheel_release_cb, LV_EVENT_RELEASED, this);
            lv_obj_add_event_cb(m_highlight_wheel, color_wheel_release_cb, LV_EVENT_PRESS_LOST, this);

            m_highlight_preview = lv_obj_create(m_highlight_wheel);
            lv_obj_set_size(m_highlight_preview, 60, 60);
            lv_obj_set_style_radius(m_highlight_preview, 25, 25);
            lv_obj_set_style_border_width(m_highlight_preview, 0, 0);
            lv_obj_set_style_border_color(m_highlight_preview, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_bg_color(m_highlight_preview, highlight_color, 0);
            lv_obj_set_style_bg_opa(m_highlight_preview, LV_OPA_COVER, 0);
            lv_obj_align(m_highlight_preview, LV_ALIGN_CENTER, 0, 0);
            lv_obj_move_foreground(m_highlight_preview);

            // Support wheel -----------------------------------------------------------
            lv_obj_t* support_group = lv_obj_create(color_cont);
            lv_obj_set_size(support_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(support_group, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(support_group, 0, 0);
            lv_obj_set_style_border_side(support_group, LV_BORDER_SIDE_NONE, 0);
            lv_obj_set_style_outline_width(support_group, 0, 0);
            lv_obj_set_style_outline_opa(support_group, LV_OPA_TRANSP, 0);
            lv_obj_set_flex_flow(support_group, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(support_group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            m_support_label = lv_label_create(support_group);
            lv_label_set_text(m_support_label, "Support Color");
            lv_obj_set_style_text_color(m_support_label, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(m_support_label, &inconsolata_regular_26, 0);

            m_support_wheel = lv_colorwheel_create(support_group, true);
            lv_obj_set_size(m_support_wheel, 180, 180);
            lv_obj_set_style_radius(m_support_wheel, 0, 0);
            lv_colorwheel_set_mode(m_support_wheel, LV_COLORWHEEL_MODE_HUE);
            lv_obj_add_event_cb(m_support_wheel, color_wheel_event_cb, LV_EVENT_VALUE_CHANGED, this);
            lv_obj_add_event_cb(m_support_wheel, color_wheel_release_cb, LV_EVENT_RELEASED, this);
            lv_obj_add_event_cb(m_support_wheel, color_wheel_release_cb, LV_EVENT_PRESS_LOST, this);

            m_support_preview = lv_obj_create(m_support_wheel);
            lv_obj_set_size(m_support_preview, 60, 60);
            lv_obj_set_style_radius(m_support_preview, 2, 2);
            lv_obj_set_style_border_width(m_support_preview, 0, 0);
            lv_obj_set_style_border_color(m_support_preview, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_bg_color(m_support_preview, support_color, 0);
            lv_obj_set_style_bg_opa(m_support_preview, LV_OPA_COVER, 0);
            lv_obj_align(m_support_preview, LV_ALIGN_CENTER, 0, 0);
            lv_obj_move_foreground(m_support_preview);
        }

        {   // Time setting with fade‑mask rollers -------------------------------------
            lv_obj_t* time_cont = lv_obj_create(scroll);
            lv_obj_set_size(time_cont, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(time_cont, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(time_cont, 0, 0);
            lv_obj_set_style_border_side(time_cont, LV_BORDER_SIDE_NONE, 0);
            lv_obj_set_style_outline_width(time_cont, 0, 0);
            lv_obj_set_style_outline_opa(time_cont, LV_OPA_TRANSP, 0);
            lv_obj_set_flex_flow(time_cont, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(time_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_row(time_cont, 5, 0);

            lv_obj_t* time_label = lv_label_create(time_cont);
            lv_label_set_text(time_label, "Set Time");
            lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(time_label, &inconsolata_regular_26, 0);

            // Row for rollers and button – use center alignment with fixed margins
            lv_obj_t* time_row = lv_obj_create(time_cont);
            lv_obj_set_size(time_row, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(time_row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(time_row, 0, 0);
            lv_obj_set_style_border_side(time_row, LV_BORDER_SIDE_NONE, 0);
            lv_obj_set_style_outline_width(time_row, 0, 0);
            lv_obj_set_style_outline_opa(time_row, LV_OPA_TRANSP, 0);
            lv_obj_set_flex_flow(time_row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(time_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            // Add small horizontal padding so items don't touch edges
            lv_obj_set_style_pad_left(time_row, 5, 0);
            lv_obj_set_style_pad_right(time_row, 5, 0);

            // Hour roller
            lv_obj_t* hour_group = lv_obj_create(time_row);
            lv_obj_set_size(hour_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(hour_group, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(hour_group, 0, 0);
            lv_obj_set_style_border_side(hour_group, LV_BORDER_SIDE_NONE, 0);
            lv_obj_set_style_outline_width(hour_group, 0, 0);
            lv_obj_set_style_outline_opa(hour_group, LV_OPA_TRANSP, 0);
            lv_obj_set_flex_flow(hour_group, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(hour_group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            lv_obj_t* hour_label = lv_label_create(hour_group);
            lv_label_set_text(hour_label, "Hour");
            lv_obj_set_style_text_color(hour_label, lv_color_hex(0xCCCCCC), 0);

            m_hour_roller = lv_roller_create(hour_group);
            lv_obj_set_style_bg_opa(m_hour_roller, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_border_width(m_hour_roller, 0, LV_PART_MAIN);
            lv_obj_set_style_text_color(m_hour_roller, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
            lv_obj_set_style_text_font(m_hour_roller, &inconsolata_regular_26, LV_PART_MAIN);
            lv_obj_set_style_text_font(m_hour_roller, &inconsolata_regular_48, LV_PART_SELECTED);
            lv_obj_set_style_text_color(m_hour_roller, lv_color_hex(0xFFFFFF), LV_PART_SELECTED);
            lv_obj_set_style_bg_opa(m_hour_roller, LV_OPA_TRANSP, LV_PART_SELECTED);
            lv_obj_set_style_border_width(m_hour_roller, 0, LV_PART_SELECTED);

            std::string hour_opts;
            for (int i = 0; i < 24; ++i) {
                char buf[3];
                snprintf(buf, sizeof(buf), "%02d", i);
                hour_opts += buf;
                hour_opts += "\n";
            }
            hour_opts.pop_back();
            lv_roller_set_options(m_hour_roller, hour_opts.c_str(), LV_ROLLER_MODE_NORMAL);
            lv_obj_set_size(m_hour_roller, 40, 80);          // Reduced width
            lv_obj_set_style_radius(m_hour_roller, 0, 0);
            lv_roller_set_visible_row_count(m_hour_roller, 3);
            lv_obj_add_event_cb(m_hour_roller, roller_mask_event_cb, LV_EVENT_ALL, NULL);

            // Minute roller
            lv_obj_t* minute_group = lv_obj_create(time_row);
            lv_obj_set_size(minute_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(minute_group, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(minute_group, 0, 0);
            lv_obj_set_style_border_side(minute_group, LV_BORDER_SIDE_NONE, 0);
            lv_obj_set_style_outline_width(minute_group, 0, 0);
            lv_obj_set_style_outline_opa(minute_group, LV_OPA_TRANSP, 0);
            lv_obj_set_flex_flow(minute_group, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(minute_group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            lv_obj_t* minute_label = lv_label_create(minute_group);
            lv_label_set_text(minute_label, "Minute");
            lv_obj_set_style_text_color(minute_label, lv_color_hex(0xCCCCCC), 0);

            m_minute_roller = lv_roller_create(minute_group);
            lv_obj_set_style_bg_opa(m_minute_roller, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_border_width(m_minute_roller, 0, LV_PART_MAIN);
            lv_obj_set_style_text_color(m_minute_roller, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
            lv_obj_set_style_text_font(m_minute_roller, &inconsolata_regular_26, LV_PART_MAIN);
            lv_obj_set_style_text_font(m_minute_roller, &inconsolata_regular_48, LV_PART_SELECTED);
            lv_obj_set_style_text_color(m_minute_roller, lv_color_hex(0xFFFFFF), LV_PART_SELECTED);
            lv_obj_set_style_bg_opa(m_minute_roller, LV_OPA_TRANSP, LV_PART_SELECTED);
            lv_obj_set_style_border_width(m_minute_roller, 0, LV_PART_SELECTED);

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
            lv_obj_set_style_radius(m_minute_roller, 0, 0);
            lv_roller_set_visible_row_count(m_minute_roller, 3);
            lv_obj_add_event_cb(m_minute_roller, roller_mask_event_cb, LV_EVENT_ALL, NULL);

            // Set button – slightly smaller
            m_set_time_btn = lv_btn_create(time_row);
            lv_obj_set_size(m_set_time_btn, 48, 40);
            lv_obj_set_style_radius(m_set_time_btn, 0, 0);
            lv_obj_set_style_bg_color(m_set_time_btn, highlight_color, 0);
            lv_obj_add_event_cb(m_set_time_btn, set_time_btn_event_cb, LV_EVENT_CLICKED, this);

            lv_obj_t* btn_label = lv_label_create(m_set_time_btn);
            lv_label_set_text(btn_label, "Set");
            lv_obj_set_style_text_color(btn_label, lv_color_hex(0x000000), 0);
            lv_obj_set_style_text_font(btn_label, &inconsolata_regular_26, 0);
            lv_obj_center(btn_label);
        }
    }


    void settings_screen::sync_ui() {

        // Brightness slider
        if (m_slider) {
            u16 brightness = getBrightens();
            lv_slider_set_value(m_slider, brightness, LV_ANIM_OFF);
            // Update percentage label if it exists
            if (m_brightness_value_label) {
                char buf[8];
                snprintf(buf, sizeof(buf), "%d%%", (int)(brightness * 100 / 255));
                lv_label_set_text(m_brightness_value_label, buf);
            }
        }

        // Colour wheels – set to current global colours
        if (m_highlight_wheel) {
            lv_color_t col = highlight_color;
            lv_colorwheel_set_rgb(m_highlight_wheel, col);
            if (m_highlight_preview)
                lv_obj_set_style_bg_color(m_highlight_preview, col, 0);
        }
        if (m_support_wheel) {
            lv_color_t col = support_color;
            lv_colorwheel_set_rgb(m_support_wheel, col);
            if (m_support_preview)
                lv_obj_set_style_bg_color(m_support_preview, col, 0);
        }

        // Time rollers – set to current system time
        time_t now = time(nullptr);
        struct tm tm_info;
        localtime_r(&now, &tm_info);
        int hour = (tm_info.tm_year > 70) ? tm_info.tm_hour : 0;
        int minute = (tm_info.tm_year > 70) ? tm_info.tm_min : 0;

        if (m_hour_roller)
            lv_roller_set_selected(m_hour_roller, hour, LV_ANIM_OFF);

        if (m_minute_roller)
            lv_roller_set_selected(m_minute_roller, minute, LV_ANIM_OFF);
    }


    void settings_screen::slider_event_cb(lv_event_t* e) {

        lv_obj_t* slider = lv_event_get_target(e);
        u16 value = static_cast<u16>(lv_slider_get_value(slider));

        // map value from 0-255 to 51-255
        value = (value * 204) / 255 + 51;

        setBrightens(static_cast<u8>(value));
        // Update the percentage label
        settings_screen* self = (settings_screen*)lv_event_get_user_data(e);
        if (self && self->m_brightness_value_label) {

            char buf[8];
            snprintf(buf, sizeof(buf), "%d%%", (int)(value * 100 / 255));
            lv_label_set_text(self->m_brightness_value_label, buf);
        }
    }


    void settings_screen::color_wheel_event_cb(lv_event_t* e) {

        lv_obj_t* wheel = lv_event_get_target(e);
        lv_color_t color = lv_colorwheel_get_rgb(wheel);
        settings_screen* self = (settings_screen*)lv_event_get_user_data(e);
        if (!self)
            return;

        if (wheel == self->m_highlight_wheel) {

            set_highlight_color(color);
            if (self->m_highlight_preview)
                lv_obj_set_style_bg_color(self->m_highlight_preview, color, 0);

        } else if (wheel == self->m_support_wheel) {

            set_support_color(color);
            if (self->m_support_preview)
                lv_obj_set_style_bg_color(self->m_support_preview, color, 0);
        }

        self->m_pending_recreate = true;
    }


    void settings_screen::color_wheel_release_cb(lv_event_t* e) {

        settings_screen* self = (settings_screen*)lv_event_get_user_data(e);
        if (!self)
            return;

        if (self->m_pending_recreate) {
            self->m_pending_recreate = false;
            APP::UI::screen_manager::recreate_screens();
            ESP_LOGI(TAG, "Screens recreated after color wheel release");
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


    void settings_screen::roller_mask_event_cb(lv_event_t* e) {

        lv_event_code_t code = lv_event_get_code(e);
        lv_obj_t* obj = lv_event_get_target(e);

        static int16_t mask_top_id = -1;
        static int16_t mask_bottom_id = -1;

        if (code == LV_EVENT_COVER_CHECK)
            lv_event_set_cover_res(e, LV_COVER_RES_MASKED);

        else if (code == LV_EVENT_DRAW_MAIN_BEGIN) {

            const lv_font_t* font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
            lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
            lv_coord_t font_h = lv_font_get_line_height(font);

            lv_area_t roller_coords;
            lv_obj_get_coords(obj, &roller_coords);

            lv_area_t rect_area;
            rect_area.x1 = roller_coords.x1;
            rect_area.x2 = roller_coords.x2;
            rect_area.y1 = roller_coords.y1;
            rect_area.y2 = roller_coords.y1 + (lv_obj_get_height(obj) - font_h - line_space) / 2;

            lv_draw_mask_fade_param_t* fade_mask_top = (lv_draw_mask_fade_param_t*)lv_mem_buf_get(sizeof(lv_draw_mask_fade_param_t));
            lv_draw_mask_fade_init(fade_mask_top, &rect_area, LV_OPA_TRANSP, rect_area.y1, LV_OPA_COVER, rect_area.y2);
            mask_top_id = lv_draw_mask_add(fade_mask_top, NULL);

            rect_area.y1 = rect_area.y2 + font_h + line_space - 1;
            rect_area.y2 = roller_coords.y2;

            lv_draw_mask_fade_param_t* fade_mask_bottom = (lv_draw_mask_fade_param_t*)lv_mem_buf_get(sizeof(lv_draw_mask_fade_param_t));
            lv_draw_mask_fade_init(fade_mask_bottom, &rect_area, LV_OPA_COVER, rect_area.y1, LV_OPA_TRANSP, rect_area.y2);
            mask_bottom_id = lv_draw_mask_add(fade_mask_bottom, NULL);
        }

        else if (code == LV_EVENT_DRAW_POST_END) {

            lv_draw_mask_fade_param_t* fade_mask_top = (lv_draw_mask_fade_param_t*)lv_draw_mask_remove_id(mask_top_id);
            lv_draw_mask_fade_param_t* fade_mask_bottom = (lv_draw_mask_fade_param_t*)lv_draw_mask_remove_id(mask_bottom_id);
            lv_draw_mask_free_param(fade_mask_top);
            lv_draw_mask_free_param(fade_mask_bottom);
            lv_mem_buf_release(fade_mask_top);
            lv_mem_buf_release(fade_mask_bottom);
            mask_top_id = -1;
            mask_bottom_id = -1;
        }
    }

}
