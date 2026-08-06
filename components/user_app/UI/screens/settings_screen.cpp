#include "util/pch.hpp"
#include "settings_screen.hpp"

#include "user_app.hpp"
#include "UI/util.hpp"                    // <-- for create_geometric_pattern_0
#include "UI/screen_manager.hpp"


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
        lv_obj_set_flex_align(color_container, LV_FLEX_ALIGN_SPACE_EVENLY,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // ---- Highlight color wheel ----
        lv_obj_t* highlight_group = lv_obj_create(color_container);
        lv_obj_set_size(highlight_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(highlight_group, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(highlight_group, 0, 0);
        lv_obj_set_flex_flow(highlight_group, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(highlight_group, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

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
        lv_obj_set_flex_align(support_group, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

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

}
