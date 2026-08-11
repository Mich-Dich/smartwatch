
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

    // @brief countdown timer with set, start/pause, reset.
    class timer_screen : public screen {
    public:
        timer_screen();
        ~timer_screen() override;

        void init() override;
        void show() override;
        void hide() override;
        void destroy() override;
        void recreate_ui() override;
        lv_obj_t* get_root() override { return m_screen; }
        bool handle_event(lv_event_t* e) override { return false; }

    private:

        // Internal helpers
        void create_ui_elements();
        void sync_ui();
        void update_display();
        void start_timer();
        void pause_timer();
        void reset_timer();
        void timer_tick();
        void on_timer_complete();

        // Alarm helpers
        void show_alarm();                           // create and show alarm overlay
        void dismiss_alarm();                        // remove alarm, restore normal UI
        void create_rings();                         // create the animated ring objects
        static void alarm_btn_cb(lv_event_t* e);     // callback for clock button

        // LVGL callbacks
        static void tick_cb(lv_timer_t* timer);
        static void start_pause_btn_cb(lv_event_t* e);
        static void reset_btn_cb(lv_event_t* e);
        static void roller_mask_event_cb(lv_event_t* e);


        // UI elements
        lv_obj_t*                   m_screen = nullptr;
        lv_obj_t*                   m_pattern_canvas = nullptr;
        lv_obj_t*                   m_time_display = nullptr;          // big label showing HH:MM:SS
        lv_obj_t*                   m_status_label = nullptr;          // shows "Running", "Paused", "Time's up!"
        lv_obj_t*                   m_hour_roller = nullptr;
        lv_obj_t*                   m_minute_roller = nullptr;
        lv_obj_t*                   m_second_roller = nullptr;
        lv_obj_t*                   m_start_pause_btn = nullptr;
        lv_obj_t*                   m_reset_btn = nullptr;

        // Alarm UI
        lv_obj_t*                   m_alarm_cont = nullptr;            // full‑screen container for alarm
        lv_obj_t*                   m_clock_btn = nullptr;             // big button with clock symbol
        std::vector<lv_obj_t*>      m_rings;              // expanding rings (circle objects)

        // Timer state
        int32_t                     m_remaining_seconds = 0;
        int32_t                     m_set_seconds = 0;
        bool                        m_is_running = false;
        bool                        m_is_paused = false;
        
        u32                         m_keep_alive_handle = 0;
        lv_timer_t*                 m_tick_timer = nullptr;
    };

}
