
#include "util/pch.hpp"
#include "stopwatch_screen.hpp"

#include "UI/util.hpp"
#include "UI/screen_manager.hpp"
#include "user_app.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    constexpr const char*                   TAG = "stopwatch_screen";

    constexpr u32                           TICK_INTERVAL_MS = 100;          // update every 100 ms -> centisecond precision

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    // CLASS IMPLEMENTATION ============================================================================================

    stopwatch_screen::stopwatch_screen() = default;


    stopwatch_screen::~stopwatch_screen() { destroy(); }

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

    void stopwatch_screen::init() {

        ESP_LOGI(TAG, "Initialising stopwatch screen");

        m_screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(m_screen, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(m_screen, LV_OPA_COVER, 0);
        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);

        create_ui_elements();
        update_display();   // initial "00:00.00"

        // Create the tick timer but keep it paused until started
        m_tick_timer = lv_timer_create(tick_cb, TICK_INTERVAL_MS, this);
        lv_timer_pause(m_tick_timer);

        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "Stopwatch screen initialised");
    }


    void stopwatch_screen::show() {

        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_HIDDEN);

        // If the stopwatch was running and paused, resume the tick timer
        if (m_is_running && m_is_paused) {
            lv_timer_resume(m_tick_timer);
            m_is_paused = false;
            lv_label_set_text(m_status_label, "Running");
            lv_obj_set_style_text_color(m_status_label, lv_color_hex(0x88FF88), 0);
            lv_label_set_text(lv_obj_get_child(m_start_pause_btn, 0), "Pause");
        }
        // Otherwise, just show the current elapsed time
        update_display();
    }


    void stopwatch_screen::hide() {
        // Nothing special needed; LVGL will hide the screen
    }


    void stopwatch_screen::destroy() {

        if (m_tick_timer) {
            lv_timer_del(m_tick_timer);
            m_tick_timer = nullptr;
        }

        // Free canvas buffer if any (like in settings_screen)
        void* buffer_to_free = nullptr;
        if (m_pattern_canvas)
            buffer_to_free = lv_obj_get_user_data(m_pattern_canvas);

        if (m_screen)
            lv_obj_del(m_screen);

        if (buffer_to_free)
            lv_mem_free(buffer_to_free);

        m_screen = nullptr;
        m_pattern_canvas = nullptr;
    }


    void stopwatch_screen::recreate_ui() {

        if (!m_screen)
            return;

        // Save current state (running/paused) and elapsed time
        bool was_running = m_is_running;
        bool was_paused = m_is_paused;
        uint32_t elapsed_ms = m_elapsed_ms;

        // Delete all children
        uint32_t child_cnt = lv_obj_get_child_cnt(m_screen);
        std::vector<lv_obj_t*> to_delete;
        to_delete.reserve(child_cnt);
        for (uint32_t i = 0; i < child_cnt; i++)
            to_delete.push_back(lv_obj_get_child(m_screen, i));

        for (lv_obj_t* child : to_delete)
            lv_obj_del(child);

        // Free canvas buffer if it existed
        void* buffer_to_free = nullptr;
        if (m_pattern_canvas)
            buffer_to_free = lv_obj_get_user_data(m_pattern_canvas);
        if (buffer_to_free)
            lv_mem_free(buffer_to_free);

        // Reset UI pointers
        m_pattern_canvas = nullptr;
        m_time_display = nullptr;
        m_status_label = nullptr;
        m_start_pause_btn = nullptr;
        m_reset_btn = nullptr;

        // Recreate UI elements
        create_ui_elements();

        // Restore state
        m_is_running = was_running;
        m_is_paused = was_paused;
        m_elapsed_ms = elapsed_ms;
        update_display();

        // Update button labels and status based on state
        if (m_is_running) {
            if (m_is_paused) {
                lv_label_set_text(m_status_label, "Paused");
                lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xFFFF88), 0);
                lv_label_set_text(lv_obj_get_child(m_start_pause_btn, 0), "Resume");
            } else {
                lv_label_set_text(m_status_label, "Running");
                lv_obj_set_style_text_color(m_status_label, lv_color_hex(0x88FF88), 0);
                lv_label_set_text(lv_obj_get_child(m_start_pause_btn, 0), "Pause");
                // Resume tick timer if it was running before recreate
                lv_timer_resume(m_tick_timer);
            }
        } else {
            lv_label_set_text(m_status_label, "Stopped");
            lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xAAAAAA), 0);
            lv_label_set_text(lv_obj_get_child(m_start_pause_btn, 0), "Start");
            lv_timer_pause(m_tick_timer);
        }

        ESP_LOGI(TAG, "Stopwatch UI recreated");
    }

    // CLASS PRIVATE ===================================================================================================

    void stopwatch_screen::create_ui_elements() {

        // Geometric background pattern
        m_pattern_canvas = APP::UI::util::create_geometric_pattern_1(
            m_screen,
            lv_color_mix(highlight_color, lv_color_hex(0x000000), 80),
            lv_color_mix(support_color, lv_color_hex(0x000000), 80)
        );

        // Title
        lv_obj_t* title = lv_label_create(m_screen);
        lv_label_set_text(title, "Stopwatch");
        lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(title, &inconsolata_regular_26, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

        // Main time display (large)
        m_time_display = lv_label_create(m_screen);
        lv_label_set_text(m_time_display, "00:00.00");
        lv_obj_set_style_text_color(m_time_display, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(m_time_display, &inconsolata_regular_48, 0);
        lv_obj_align(m_time_display, LV_ALIGN_TOP_MID, 0, 65);

        // Status label
        m_status_label = lv_label_create(m_screen);
        lv_label_set_text(m_status_label, "Stopped");
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_text_font(m_status_label, &inconsolata_regular_26, 0);
        lv_obj_align(m_status_label, LV_ALIGN_TOP_MID, 0, 130);

        // Buttons at bottom
        lv_obj_t* btn_container = lv_obj_create(m_screen);
        lv_obj_set_size(btn_container, LV_PCT(90), LV_SIZE_CONTENT);
        lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -15);
        lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_container, 0, 0);
        lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(btn_container, 20, 0);

        // Start/Pause button
        m_start_pause_btn = lv_btn_create(btn_container);
        lv_obj_set_size(m_start_pause_btn, 100, 50);
        lv_obj_set_style_radius(m_start_pause_btn, 0, 0);
        lv_obj_set_style_bg_color(m_start_pause_btn, highlight_color, 0);
        lv_obj_add_event_cb(m_start_pause_btn, start_pause_btn_cb, LV_EVENT_CLICKED, this);
        lv_obj_t* btn_label = lv_label_create(m_start_pause_btn);
        lv_label_set_text(btn_label, "Start");
        lv_obj_set_style_text_color(btn_label, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_font(btn_label, &inconsolata_regular_26, 0);
        lv_obj_center(btn_label);

        // Reset button
        m_reset_btn = lv_btn_create(btn_container);
        lv_obj_set_size(m_reset_btn, 100, 50);
        lv_obj_set_style_radius(m_reset_btn, 0, 0);
        lv_obj_set_style_bg_color(m_reset_btn, support_color, 0);
        lv_obj_add_event_cb(m_reset_btn, reset_btn_cb, LV_EVENT_CLICKED, this);
        lv_obj_t* reset_label = lv_label_create(m_reset_btn);
        lv_label_set_text(reset_label, "Reset");
        lv_obj_set_style_text_color(reset_label, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_font(reset_label, &inconsolata_regular_26, 0);
        lv_obj_center(reset_label);
    }


    void stopwatch_screen::update_display() {

        if (!m_time_display)
            return;

        uint32_t total_sec = m_elapsed_ms / 1000;
        uint32_t centiseconds = (m_elapsed_ms % 1000) / 10;

        uint32_t hours = total_sec / 3600;
        uint32_t minutes = (total_sec % 3600) / 60;
        uint32_t seconds = total_sec % 60;

        char buf[32];
        if (hours > 0) {
            // Format as HH:MM:SS (no centiseconds when hours are present)
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hours, minutes, seconds);
        } else {
            // Format as MM:SS.cc
            snprintf(buf, sizeof(buf), "%02d:%02d.%02d", minutes, seconds, centiseconds);
        }
        lv_label_set_text(m_time_display, buf);
    }


    void stopwatch_screen::start_timer() {

        if (m_is_running)
            return;

        // If stopped, reset elapsed to 0 before starting (optional: allow continue from previous)
        // We'll start from current elapsed, but if it's 0, that's fine.
        m_is_running = true;
        m_is_paused = false;
        lv_timer_resume(m_tick_timer);
        lv_label_set_text(m_status_label, "Running");
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0x88FF88), 0);
        lv_label_set_text(lv_obj_get_child(m_start_pause_btn, 0), "Pause");
    }


    void stopwatch_screen::pause_timer() {

        if (!m_is_running || m_is_paused)
            return;

        m_is_paused = true;
        lv_timer_pause(m_tick_timer);
        lv_label_set_text(m_status_label, "Paused");
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xFFFF88), 0);
        lv_label_set_text(lv_obj_get_child(m_start_pause_btn, 0), "Resume");
    }


    void stopwatch_screen::reset_timer() {

        // Reset to zero, stop if running
        if (m_is_running) {
            lv_timer_pause(m_tick_timer);
            m_is_running = false;
            m_is_paused = false;
        }
        m_elapsed_ms = 0;
        update_display();
        lv_label_set_text(m_status_label, "Stopped");
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xAAAAAA), 0);
        lv_label_set_text(lv_obj_get_child(m_start_pause_btn, 0), "Start");
    }


    void stopwatch_screen::timer_tick() {

        if (!m_is_running || m_is_paused)
            return;

        m_elapsed_ms += TICK_INTERVAL_MS;
        update_display();
    }

    // STATIC CALLBACKS ------------------------------------------------------------------------------------------------

    void stopwatch_screen::tick_cb(lv_timer_t* timer) {

        stopwatch_screen* self = static_cast<stopwatch_screen*>(timer->user_data);
        if (self)
            self->timer_tick();
    }


    void stopwatch_screen::start_pause_btn_cb(lv_event_t* e) {

        stopwatch_screen* self = static_cast<stopwatch_screen*>(lv_event_get_user_data(e));
        if (!self)
            return;

        if (!self->m_is_running)                    // Not running: start
            self->start_timer();
        
        else if (self->m_is_paused) {               // Paused: resume
            self->m_is_paused = false;
            lv_timer_resume(self->m_tick_timer);
            lv_label_set_text(self->m_status_label, "Running");
            lv_obj_set_style_text_color(self->m_status_label, lv_color_hex(0x88FF88), 0);
            lv_label_set_text(lv_obj_get_child(self->m_start_pause_btn, 0), "Pause");
        
        } else                                      // Running: pause
            self->pause_timer();
    }


    void stopwatch_screen::reset_btn_cb(lv_event_t* e) {

        stopwatch_screen* self = static_cast<stopwatch_screen*>(lv_event_get_user_data(e));
        if (self)
            self->reset_timer();
    }

}
