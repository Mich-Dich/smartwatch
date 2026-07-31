
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

        SCREEN_NAME("Settings")

        void create(lv_obj_t* screen) override;

    private:

        // Static callbacks for sync
        static void sync_time_button_cb(lv_event_t* e);
        static void sync_ui_update_timer_cb(lv_timer_t* timer);
        static void sync_time_task(void* param);

        // Static callback for slider / switch (examples)
        static void brightness_slider_cb(lv_event_t* e);
        static void format_switch_cb(lv_event_t* e);

        // Sync members
        lv_obj_t*               m_sync_btn = nullptr;
        std::atomic<bool>       m_sync_running{false};
        volatile bool           m_sync_done = false;
        volatile bool           m_sync_success = false;

        // UI elements we may need later
        lv_obj_t*               m_brightness_slider = nullptr;
        lv_obj_t*               m_24h_switch = nullptr;
    };

}
