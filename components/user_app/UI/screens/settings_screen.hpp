
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

    class settings_screen : public screen {
    public:

        settings_screen();
        ~settings_screen();

        void init() override;
        void show() override;
        void hide() override;
        void destroy() override;
        lv_obj_t* get_root() override;
        bool handle_event(lv_event_t* e) override;

    private:

        static void slider_event_cb(lv_event_t* e);

        static void color_wheel_event_cb(lv_event_t* e);

        static void set_time_btn_event_cb(lv_event_t* e);

        lv_obj_t* m_screen = nullptr;
        lv_obj_t* m_slider = nullptr;
        lv_obj_t* m_label = nullptr;

        // Color selectors
        lv_obj_t* m_highlight_wheel = nullptr;
        lv_obj_t* m_highlight_preview = nullptr;
        lv_obj_t* m_highlight_label = nullptr;

        lv_obj_t* m_support_wheel = nullptr;
        lv_obj_t* m_support_preview = nullptr;
        lv_obj_t* m_support_label = nullptr;

        lv_obj_t* m_time_container = nullptr;
        lv_obj_t* m_hour_roller = nullptr;
        lv_obj_t* m_minute_roller = nullptr;
        lv_obj_t* m_set_time_btn = nullptr;
    };

}
