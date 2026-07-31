
#include "util/pch.hpp"
#include "settings_screen.hpp"

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

        // Create a new LVGL screen object
        m_screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(m_screen, lv_color_hex(0x000000), 0); // black background

        // Label
        m_label = lv_label_create(m_screen);
        lv_label_set_text(m_label, "Brightness");
        lv_obj_align(m_label, LV_ALIGN_TOP_MID, 0, 20);

        // Slider
        m_slider = lv_slider_create(m_screen);
        lv_slider_set_range(m_slider, 0, 255);
        lv_slider_set_value(m_slider, 128, LV_ANIM_OFF); // default
        lv_obj_set_width(m_slider, 200);
        lv_obj_align(m_slider, LV_ALIGN_CENTER, 0, -20);
        lv_obj_add_event_cb(m_slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, this);

        // Initially hidden (the screen_manager will show it when needed)
        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);

        ESP_LOGI(TAG, "Settings screen initialised");
    }


    void settings_screen::show()            { lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_HIDDEN); }


    void settings_screen::hide()            { lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN); }


    lv_obj_t* settings_screen::get_root()   { return m_screen; }


    void settings_screen::destroy() {

        if (m_screen) {

            lv_obj_del(m_screen);
            m_screen = nullptr;
        }
    }
    

    bool settings_screen::handle_event(lv_event_t* e) {

        // We handle events via static callbacks, but we can also do direct handling here.
        return false;
    }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

    void settings_screen::slider_event_cb(lv_event_t* e) {

        lv_obj_t* slider = lv_event_get_target(e);
        uint8_t value = lv_slider_get_value(slider);
        setBrightens(value);
    }

}
