
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

    // A full‑screen stopwatch with a 60‑second arc timer and hundredths‑of‑a‑second precision.
    // Uses the LVGL tick timer for elapsed time measurements.
    class stopwatch_screen : public screen {
    public:

        // Returns the screen’s unique name for navigation purposes.
        SCREEN_NAME("Stopwatch")


        // @brief Creates all LVGL objects (arc, labels, buttons) on the given screen.
        // @param screen The LVGL screen object to populate.
        void create(lv_obj_t* screen) override;

    private:

        // @brief Callback for the Start/Stop button.
        // Toggles the running state and updates button appearance.
        // @param e LVGL event containing a pointer to the stopwatch_screen instance via user_data.
        static void start_stop_btn_cb(lv_event_t* e);
        

        // @brief Callback for the Reset button.
        // Stops the stopwatch if running and resets the accumulated time to zero.
        // @param e LVGL event containing a pointer to the stopwatch_screen instance via user_data.
        static void reset_btn_cb(lv_event_t* e);
        

        // @brief Periodic timer callback that refreshes the time display.
        // Called every 50 ms while the stopwatch is running.
        // @param timer LVGL timer object whose user_data points to the stopwatch_screen instance.
        static void update_timer_cb(lv_timer_t* timer);
        

        // @brief Updates the time label and arc progress based on the current elapsed time.
        // Handles both running and paused states.
        void update_display();

        lv_obj_t*                   m_time_label = nullptr;
        lv_obj_t*                   m_btn_start_stop = nullptr;
        lv_obj_t*                   m_btn_reset = nullptr;
        lv_obj_t*                   m_arc = nullptr;
        lv_timer_t*                 m_update_timer = nullptr;

    };

}
