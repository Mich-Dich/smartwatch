
#pragma once

#include "UI/screen.hpp"


// FORWARD DECLARATIONS =====================================================================================

namespace APP::UI {

    // CONSTANTS ============================================================================================

    // MACROS ===============================================================================================

    // TYPES ================================================================================================

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================

    // @brief Settings screen – brightness, highlight/support colors, and manual time setting.
    class settings_screen : public screen {
    public:

        settings_screen();
        ~settings_screen();

        void init() override;

        void show() override;

        void hide() override;

        void destroy() override;

        void recreate_ui() override;

        lv_obj_t* get_root() override;

        bool handle_event(lv_event_t* e) override;

    private:

        void create_ui_elements();


        void sync_ui();


        // @brief Callback for the brightness slider.
        static void slider_event_cb(lv_event_t* e);


        // @brief Callback for the color wheels.
        static void color_wheel_event_cb(lv_event_t* e);


        // @brief
        static void color_wheel_release_cb(lv_event_t* e);


        // @brief Callback for the Set Time button.
        static void set_time_btn_event_cb(lv_event_t* e);


        // @brief Callback to apply fade mask to rollers (like LVGL example)
        static void roller_mask_event_cb(lv_event_t* e);



        lv_obj_t*           m_screen = nullptr;
        lv_obj_t*           m_pattern_canvas = nullptr;
        lv_obj_t*           m_slider = nullptr;
        lv_obj_t*           m_brightness_value_label = nullptr;
        lv_obj_t*           m_label = nullptr;

        // Color selectors
        lv_obj_t*           m_highlight_wheel = nullptr;
        lv_obj_t*           m_highlight_preview = nullptr;
        lv_obj_t*           m_highlight_label = nullptr;

        lv_obj_t*           m_support_wheel = nullptr;
        lv_obj_t*           m_support_preview = nullptr;
        lv_obj_t*           m_support_label = nullptr;

        lv_obj_t*           m_time_container = nullptr;
        lv_obj_t*           m_hour_roller = nullptr;
        lv_obj_t*           m_minute_roller = nullptr;
        lv_obj_t*           m_set_time_btn = nullptr;

        bool                m_pending_recreate = false;
        int                 m_wheel_press_count = 0;

    };

}
