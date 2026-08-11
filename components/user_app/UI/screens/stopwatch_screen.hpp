
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

    // @brief A stopwatch screen that counts up from zero with start/pause/reset.
    class stopwatch_screen : public screen {
    public:

        stopwatch_screen();
        ~stopwatch_screen() override;


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

        void update_display();

        void start_timer();

        void pause_timer();

        void reset_timer();

        void timer_tick();

        // LVGL callbacks
        static void tick_cb(lv_timer_t* timer);

        static void start_pause_btn_cb(lv_event_t* e);

        static void reset_btn_cb(lv_event_t* e);

        // UI elements
        lv_obj_t*       m_screen = nullptr;
        lv_obj_t*       m_pattern_canvas = nullptr;
        lv_obj_t*       m_time_display = nullptr;       // large label showing elapsed time
        lv_obj_t*       m_status_label = nullptr;       // shows "Running", "Paused", "Stopped"
        lv_obj_t*       m_start_pause_btn = nullptr;
        lv_obj_t*       m_reset_btn = nullptr;

        // Timer state
        bool            m_is_running = false;
        bool            m_is_paused = false;
        u32             m_elapsed_ms = 0;                // total elapsed milliseconds
        lv_timer_t*     m_tick_timer = nullptr;
    };

}
