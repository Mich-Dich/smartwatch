
#include "util/pch.hpp"
#include "stopwatch.hpp"

#include "util/display.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================
        
    constexpr size_t                    SAMPLE_RATE  = 16000;

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    static bool                         s_running = false;

    static u32                          s_accumulated_hs = 0;           // hundredths of a second

    static u32                          s_start_tick = 0;               // lv_tick_get() when started

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    void stopwatch_screen::create(lv_obj_t* screen) {

        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

        // Background patterns
        APP::display::draw_background_pattern_002(screen);

        // Arc ring for minute progress
        m_arc = lv_arc_create(screen);
        lv_obj_set_size(m_arc, 300, 300);          // fits inside the 390×390 screen
        lv_obj_center(m_arc);
        lv_obj_set_style_arc_color(m_arc, lv_color_hex(0x3399FF), LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(m_arc, 6, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(m_arc, lv_color_hex(0x222244), LV_PART_MAIN);
        lv_obj_set_style_arc_width(m_arc, 6, LV_PART_MAIN);
        lv_arc_set_range(m_arc, 0, 360);
        lv_arc_set_value(m_arc, 0);
        lv_arc_set_bg_angles(m_arc, 0, 360);
        lv_arc_set_rotation(m_arc, 270);           // start from top
        lv_obj_remove_flag(m_arc, LV_OBJ_FLAG_CLICKABLE);  // no touch on arc

        // Time label (centered)
        m_time_label = lv_label_create(screen);
        lv_obj_set_style_text_color(m_time_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(m_time_label, &inconsolata_regular_64, 0);
        lv_label_set_text(m_time_label, "00:00.00");
        lv_obj_center(m_time_label);

        // Start/Stop button (bottom left)
        m_btn_start_stop = lv_button_create(screen);
        lv_obj_set_size(m_btn_start_stop, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_align(m_btn_start_stop, LV_ALIGN_BOTTOM_MID, 0, -140);
        lv_obj_set_style_bg_color(m_btn_start_stop, lv_color_hex(0x0066CC), 0); // blue start
        lv_obj_set_style_radius(m_btn_start_stop, 8, 0);
        lv_obj_t* lbl_start = lv_label_create(m_btn_start_stop);
        lv_label_set_text(lbl_start, "Start");
        lv_obj_set_style_text_font(lbl_start, &lv_font_montserrat_28, 0);
        lv_obj_center(lbl_start);
        lv_obj_add_event_cb(m_btn_start_stop, start_stop_btn_cb, LV_EVENT_CLICKED, this);

        // Reset button (bottom right)
        m_btn_reset = lv_button_create(screen);
        lv_obj_set_size(m_btn_reset, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_align(m_btn_reset, LV_ALIGN_BOTTOM_MID, 0, -70);
        lv_obj_set_style_bg_color(m_btn_reset, lv_color_hex(0x444444), 0);
        lv_obj_set_style_radius(m_btn_reset, 8, 0);
        lv_obj_t* lbl_reset = lv_label_create(m_btn_reset);
        lv_label_set_text(lbl_reset, "Reset");
        lv_obj_set_style_text_font(lbl_reset, &lv_font_montserrat_28, 0);
        lv_obj_center(lbl_reset);
        lv_obj_add_event_cb(m_btn_reset, reset_btn_cb, LV_EVENT_CLICKED, this);
    }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

    void stopwatch_screen::start_stop_btn_cb(lv_event_t* e) {

        stopwatch_screen* self = static_cast<stopwatch_screen*>(lv_event_get_user_data(e));
        if (!s_running) {

            s_start_tick = lv_tick_get();                                                       // Start
            s_running = true;
            lv_obj_set_style_bg_color(self->m_btn_start_stop, lv_color_hex(0xCC6600), 0);       // Change button to orange "Stop"
            lv_obj_t* label = lv_obj_get_child(self->m_btn_start_stop, 0);
            lv_label_set_text(label, "Stop");

            self->m_update_timer = lv_timer_create(update_timer_cb, 50, self);                  // Start update timer (every 50 ms for smooth hundredths)
        
        } else {                                                                                // Stop

            s_accumulated_hs += (lv_tick_get() - s_start_tick) / 10;                            // ms -> hundredths
            s_running = false;
            lv_obj_set_style_bg_color(self->m_btn_start_stop, lv_color_hex(0x0066CC), 0);       // Change button back to blue "Start"
            lv_obj_t* label = lv_obj_get_child(self->m_btn_start_stop, 0);
            lv_label_set_text(label, "Start");

            if (self->m_update_timer) {                                                         // Stop the timer
                lv_timer_delete(self->m_update_timer);
                self->m_update_timer = nullptr;
            }
            self->update_display();                                                             // show final time
        }
    }

    
    void stopwatch_screen::reset_btn_cb(lv_event_t* e) {

        stopwatch_screen* self = static_cast<stopwatch_screen*>(lv_event_get_user_data(e));
        if (s_running) {      // Stop if running
            s_running = false;

            if (self->m_update_timer) {
                lv_timer_delete(self->m_update_timer);
                self->m_update_timer = nullptr;
            }
            lv_obj_set_style_bg_color(self->m_btn_start_stop, lv_color_hex(0x0066CC), 0);
            lv_obj_t* label = lv_obj_get_child(self->m_btn_start_stop, 0);
            lv_label_set_text(label, "Start");
        }

        s_accumulated_hs = 0;
        self->update_display();
    }


    void stopwatch_screen::update_timer_cb(lv_timer_t* timer) {

        stopwatch_screen* self = static_cast<stopwatch_screen*>(lv_timer_get_user_data(timer));
        self->update_display();
    }


    void stopwatch_screen::update_display() {

        u32 total_hs = s_accumulated_hs;
        if (s_running) {

            u32 elapsed_ms = lv_tick_get() - s_start_tick;
            total_hs += elapsed_ms / 10;
        }

        u32 minutes = total_hs / 6000;          // 6000 hs = 60 seconds
        u32 seconds = (total_hs % 6000) / 100;
        u32 hundredths = total_hs % 100;

        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d.%02d", minutes, seconds, hundredths);
        lv_label_set_text(m_time_label, buf);

        // Arc value: seconds + hundredths mapped to 0-360 (one full revolution per minute)
        u32 sec_hs = (seconds * 100) + hundredths;   // 0-5999
        lv_arc_set_value(m_arc, (sec_hs * 360) / 6000);
    }

}
