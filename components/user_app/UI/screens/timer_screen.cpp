
#include "util/pch.hpp"
#include "timer_screen.hpp"

#include "user_app.hpp"
#include "UI/util.hpp"
#include "UI/screen_manager.hpp"



// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    constexpr const char* TAG = "timer_screen";

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    // CLASS IMPLEMENTATION ============================================================================================

    timer_screen::timer_screen() = default;
    
    
    timer_screen::~timer_screen() { destroy(); }

    // CLASS PUBLIC ====================================================================================================

    void timer_screen::init() {

        ESP_LOGI(TAG, "Initialising timer screen");

        m_screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(m_screen, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(m_screen, LV_OPA_COVER, 0);
        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);

        create_ui_elements();
        sync_ui();  // set initial display

        // Create the tick timer but keep it paused until shown
        m_tick_timer = lv_timer_create(tick_cb, 1000, this);
        lv_timer_pause(m_tick_timer);

        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "Timer screen initialised");
    }


    void timer_screen::show() {

        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        if (m_alarm_cont)                       // If alarm is active, keep it visible and do nothing else
            return;

        if (m_is_running && m_is_paused) {      // If timer was running, resume it
            lv_timer_resume(m_tick_timer);
            m_is_paused = false;
        }
        update_display();                       // Otherwise, just show the set time
    }

    void timer_screen::hide() { }
    

    void timer_screen::destroy() {

        if (m_tick_timer) {
            lv_timer_del(m_tick_timer);
            m_tick_timer = nullptr;
        }
        // Free canvas buffer if needed (similar to settings_screen)
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

    
    void timer_screen::recreate_ui() {

        if (!m_screen)
            return;

        if (m_alarm_cont)           // If alarm is active, dismiss it first (releases keep‑alive)
            dismiss_alarm();

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

        // Reset pointers
        m_pattern_canvas = nullptr;
        m_time_display = nullptr;
        m_status_label = nullptr;
        m_hour_roller = nullptr;
        m_minute_roller = nullptr;
        m_second_roller = nullptr;
        m_start_pause_btn = nullptr;
        m_reset_btn = nullptr;

        create_ui_elements();
        sync_ui();
        ESP_LOGI(TAG, "Timer UI recreated");
    }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================
    
    void timer_screen::create_ui_elements() {

        // Background pattern (reuse util)
        m_pattern_canvas = APP::UI::util::create_geometric_pattern_1(
            m_screen,
            lv_color_mix(highlight_color, lv_color_hex(0x000000), 80),
            lv_color_mix(support_color, lv_color_hex(0x000000), 80)
        );

        // Title
        lv_obj_t* title = lv_label_create(m_screen);
        lv_label_set_text(title, "Timer");
        lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(title, &inconsolata_regular_26, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

        // Main time display (large)
        m_time_display = lv_label_create(m_screen);
        lv_label_set_text(m_time_display, "00:00:00");
        lv_obj_set_style_text_color(m_time_display, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(m_time_display, &inconsolata_regular_48, 0);  // use a larger font if available
        lv_obj_align(m_time_display, LV_ALIGN_TOP_MID, 0, 65);

        // Status label (shows running/paused/complete)
        m_status_label = lv_label_create(m_screen);
        lv_label_set_text(m_status_label, "Set time and press Start");
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_text_font(m_status_label, &inconsolata_regular_26, 0);
        lv_obj_align(m_status_label, LV_ALIGN_TOP_MID, 0, 130);

        // Rollers for setting time (use same masked style as settings_screen)
        lv_obj_t* roller_container = lv_obj_create(m_screen);
        lv_obj_set_size(roller_container, LV_PCT(90), LV_SIZE_CONTENT);
        lv_obj_align(roller_container, LV_ALIGN_TOP_MID, 0, 175);
        lv_obj_set_style_bg_opa(roller_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(roller_container, 0, 0);
        lv_obj_set_flex_flow(roller_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(roller_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(roller_container, 5, 0);
        lv_obj_set_style_pad_gap(roller_container, 5, 0);


        m_hour_roller = util::create_roller(roller_container, "H", 23, true, roller_mask_event_cb);
        m_minute_roller = util::create_roller(roller_container, "M", 59, true, roller_mask_event_cb);
        m_second_roller = util::create_roller(roller_container, "S", 59, true, roller_mask_event_cb);

        // Set initial values to 0
        lv_roller_set_selected(m_hour_roller, 0, LV_ANIM_OFF);
        lv_roller_set_selected(m_minute_roller, 0, LV_ANIM_OFF);
        lv_roller_set_selected(m_second_roller, 0, LV_ANIM_OFF);

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


    void timer_screen::roller_mask_event_cb(lv_event_t* e) {

        static int16_t mask_top_id = -1;
        static int16_t mask_bottom_id = -1;

        lv_event_code_t code = lv_event_get_code(e);
        lv_obj_t* obj = lv_event_get_target(e);

        if (code == LV_EVENT_COVER_CHECK) {
            lv_event_set_cover_res(e, LV_COVER_RES_MASKED);
        } else if (code == LV_EVENT_DRAW_MAIN_BEGIN) {
            const lv_font_t* font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
            lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
            lv_coord_t font_h = lv_font_get_line_height(font);

            lv_area_t roller_coords;
            lv_obj_get_coords(obj, &roller_coords);

            lv_area_t rect_area;
            rect_area.x1 = roller_coords.x1;
            rect_area.x2 = roller_coords.x2;
            rect_area.y1 = roller_coords.y1;
            rect_area.y2 = roller_coords.y1 + (lv_obj_get_height(obj) - font_h - line_space) / 2;

            lv_draw_mask_fade_param_t* fade_mask_top = (lv_draw_mask_fade_param_t*)lv_mem_buf_get(sizeof(lv_draw_mask_fade_param_t));
            lv_draw_mask_fade_init(fade_mask_top, &rect_area, LV_OPA_TRANSP, rect_area.y1, LV_OPA_COVER, rect_area.y2);
            mask_top_id = lv_draw_mask_add(fade_mask_top, NULL);

            rect_area.y1 = rect_area.y2 + font_h + line_space - 1;
            rect_area.y2 = roller_coords.y2;

            lv_draw_mask_fade_param_t* fade_mask_bottom = (lv_draw_mask_fade_param_t*)lv_mem_buf_get(sizeof(lv_draw_mask_fade_param_t));
            lv_draw_mask_fade_init(fade_mask_bottom, &rect_area, LV_OPA_COVER, rect_area.y1, LV_OPA_TRANSP, rect_area.y2);
            mask_bottom_id = lv_draw_mask_add(fade_mask_bottom, NULL);
        } else if (code == LV_EVENT_DRAW_POST_END) {
            lv_draw_mask_fade_param_t* fade_mask_top = (lv_draw_mask_fade_param_t*)lv_draw_mask_remove_id(mask_top_id);
            lv_draw_mask_fade_param_t* fade_mask_bottom = (lv_draw_mask_fade_param_t*)lv_draw_mask_remove_id(mask_bottom_id);
            lv_draw_mask_free_param(fade_mask_top);
            lv_draw_mask_free_param(fade_mask_bottom);
            lv_mem_buf_release(fade_mask_top);
            lv_mem_buf_release(fade_mask_bottom);
            mask_top_id = -1;
            mask_bottom_id = -1;
        }
    }


    void timer_screen::sync_ui() {

        // Read rollers and store set time, update display accordingly
        char hour_str[3] = {0}, min_str[3] = {0}, sec_str[3] = {0};
        lv_roller_get_selected_str(m_hour_roller, hour_str, sizeof(hour_str));
        lv_roller_get_selected_str(m_minute_roller, min_str, sizeof(min_str));
        lv_roller_get_selected_str(m_second_roller, sec_str, sizeof(sec_str));

        int h = atoi(hour_str);
        int m = atoi(min_str);
        int s = atoi(sec_str);
        m_set_seconds = h * 3600 + m * 60 + s;

        // If timer is not running, set remaining to set time
        if (!m_is_running && !m_is_paused) {
            m_remaining_seconds = m_set_seconds;
            update_display();
        }
    }


    void timer_screen::update_display() {

        int h = m_remaining_seconds / 3600;
        int m = (m_remaining_seconds % 3600) / 60;
        int s = m_remaining_seconds % 60;
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
        lv_label_set_text(m_time_display, buf);
    }


    void timer_screen::start_timer() {

        if (m_remaining_seconds <= 0) {
            // If at zero, reload from rollers
            sync_ui();
            if (m_remaining_seconds <= 0) {
                lv_label_set_text(m_status_label, "Set a time > 0");
                return;
            }
        }
        m_is_running = true;
        m_is_paused = false;
        lv_timer_resume(m_tick_timer);
        lv_label_set_text(m_status_label, "Running");
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0x88FF88), 0);
        lv_label_set_text(lv_obj_get_child(m_start_pause_btn, 0), "Pause");
    }


    void timer_screen::pause_timer() {

        if (m_is_running) {
            m_is_paused = true;
            lv_timer_pause(m_tick_timer);
            lv_label_set_text(m_status_label, "Paused");
            lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xFFFF88), 0);
            lv_label_set_text(lv_obj_get_child(m_start_pause_btn, 0), "Resume");
        }
    }


    void timer_screen::reset_timer() {
        // If alarm is active, dismiss it first
        if (m_alarm_cont) {
            dismiss_alarm();
            return;
        }

        // ... existing reset logic ...
        if (m_is_running) {
            lv_timer_pause(m_tick_timer);
            m_is_running = false;
            m_is_paused = false;
        }
        sync_ui(); // re-read rollers
        m_remaining_seconds = m_set_seconds;
        update_display();
        lv_label_set_text(m_status_label, "Reset");
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xAAAAAA), 0);
        lv_label_set_text(lv_obj_get_child(m_start_pause_btn, 0), "Start");
    }


    void timer_screen::timer_tick() {

        if (!m_is_running || m_is_paused)
            return;

        if (m_remaining_seconds > 0) {
            m_remaining_seconds--;
            update_display();
            // Update status if nearing end? optional
        }

        if (m_remaining_seconds == 0) {
            // Timer complete
            lv_timer_pause(m_tick_timer);
            m_is_running = false;
            m_is_paused = false;
            lv_label_set_text(m_status_label, "Time's up!");
            lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xFF6666), 0);
            lv_label_set_text(lv_obj_get_child(m_start_pause_btn, 0), "Start");
            // Optionally trigger a buzzer or visual alarm here
            on_timer_complete();
        }
    }


    void timer_screen::on_timer_complete() {
        // Timer finished – show flashy alarm
        ESP_LOGI(TAG, "Timer finished!");
        show_alarm();
    }


    void timer_screen::show_alarm() {
    
        APP::UI::screen_manager::switch_to("Timer");

        // Request keep‑alive to prevent dimming while alarm is active
        if (m_keep_alive_handle == 0) {
            m_keep_alive_handle = APP::UI::screen_manager::request_keep_alive();
            ESP_LOGI(TAG, "Keep-alive requested (handle: %u)", m_keep_alive_handle);
        }

        // Hide normal UI elements (keep background pattern)
        lv_obj_add_flag(m_time_display, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(m_status_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(m_hour_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(m_minute_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(m_second_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(m_start_pause_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(m_reset_btn, LV_OBJ_FLAG_HIDDEN);

        // Create alarm container (full screen, on top)
        if (m_alarm_cont) {
            lv_obj_del(m_alarm_cont);
            m_alarm_cont = nullptr;
        }
        m_alarm_cont = lv_obj_create(m_screen);
        lv_obj_set_size(m_alarm_cont, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(m_alarm_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(m_alarm_cont, 0, 0);
        lv_obj_clear_flag(m_alarm_cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(m_alarm_cont, LV_OBJ_FLAG_CLICKABLE); // let touches pass to button
        lv_obj_move_foreground(m_alarm_cont);

        // Create radiating rings (they go behind the button)
        create_rings();

        // Big clock button (dismiss) – created after rings to be on top
        m_clock_btn = lv_btn_create(m_alarm_cont);
        lv_obj_set_size(m_clock_btn, 160, 160);
        lv_obj_set_style_radius(m_clock_btn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(m_clock_btn, lv_color_hex(0x222222), 0);
        lv_obj_set_style_bg_opa(m_clock_btn, LV_OPA_80, 0);
        lv_obj_set_style_border_width(m_clock_btn, 4, 0);
        lv_obj_set_style_border_color(m_clock_btn, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(m_clock_btn);
        lv_obj_add_event_cb(m_clock_btn, alarm_btn_cb, LV_EVENT_CLICKED, this);
        lv_obj_move_foreground(m_clock_btn);        // ensure on top

        lv_obj_t* icon = lv_label_create(m_clock_btn);
        lv_label_set_text(icon, LV_SYMBOL_STOP);
        lv_obj_set_style_text_color(icon, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(icon, &inconsolata_regular_48, 0);
        lv_obj_center(icon);
    }


    void timer_screen::dismiss_alarm() {

        if (m_keep_alive_handle != 0) {             // Release keep‑alive
            APP::UI::screen_manager::release_keep_alive(m_keep_alive_handle);
            ESP_LOGI(TAG, "Keep-alive released (handle: %u)", m_keep_alive_handle);
            m_keep_alive_handle = 0;
        }

        // Delete alarm container and rings
        if (m_alarm_cont) {
            lv_obj_del(m_alarm_cont);
            m_alarm_cont = nullptr;
        }

        m_clock_btn = nullptr;
        m_rings.clear();

        // Unhide normal UI
        lv_obj_clear_flag(m_time_display, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(m_status_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(m_hour_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(m_minute_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(m_second_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(m_start_pause_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(m_reset_btn, LV_OBJ_FLAG_HIDDEN);

        // Reset timer to set time (stop any running state)
        if (m_is_running) {
            lv_timer_pause(m_tick_timer);
            m_is_running = false;
            m_is_paused = false;
        }
        sync_ui(); // re-read rollers
        m_remaining_seconds = m_set_seconds;
        update_display();
        lv_label_set_text(m_status_label, "Reset");
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xAAAAAA), 0);
        lv_label_set_text(lv_obj_get_child(m_start_pause_btn, 0), "Start");
    }


    void timer_screen::create_rings() {
        
        // Create a set of rings (circles with border only) that expand and fade, repeating infinitely.
        const int ring_count = 8;
        const int border_width = 12;   // thick rings
        const int max_size = 500;      // large enough to cover the screen
        for (int i = 0; i < ring_count; ++i) {
            lv_obj_t* ring = lv_obj_create(m_alarm_cont);
            lv_obj_set_size(ring, 0, 0);
            lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(ring, border_width, 0);
            lv_obj_set_style_border_opa(ring, LV_OPA_COVER, 0);
            // Alternate colours: black and white
            lv_color_t col = (i % 2 == 0) ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000);
            lv_obj_set_style_border_color(ring, col, 0);
            lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
            lv_obj_center(ring); // centre of alarm container
            lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE); // don't block touches

            m_rings.push_back(ring);

            // Animate size from 0 to max_size with delay and infinite repeat
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, ring);
            lv_anim_set_exec_cb(&a, [](void* var, int32_t val) {
                lv_obj_set_size((lv_obj_t*)var, val, val);
            });
            lv_anim_set_values(&a, 0, max_size);
            lv_anim_set_time(&a, 2000);
            lv_anim_set_delay(&a, i * 150);
            lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
            lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
            lv_anim_start(&a);

            // Also animate opacity from 255 to 0 with same timing
            lv_anim_t a_opa;
            lv_anim_init(&a_opa);
            lv_anim_set_var(&a_opa, ring);
            lv_anim_set_exec_cb(&a_opa, [](void* var, int32_t val) {
                lv_obj_set_style_border_opa((lv_obj_t*)var, val, 0);
            });
            lv_anim_set_values(&a_opa, 255, 0);
            lv_anim_set_time(&a_opa, 2000);
            lv_anim_set_delay(&a_opa, i * 150);
            lv_anim_set_path_cb(&a_opa, lv_anim_path_linear);
            lv_anim_set_repeat_count(&a_opa, LV_ANIM_REPEAT_INFINITE);
            lv_anim_start(&a_opa);
        }
    }


    void timer_screen::alarm_btn_cb(lv_event_t* e) {
        timer_screen* self = static_cast<timer_screen*>(lv_event_get_user_data(e));
        if (self) {
            self->dismiss_alarm();
        }
    }


    void timer_screen::tick_cb(lv_timer_t* timer) {

        timer_screen* self = static_cast<timer_screen*>(timer->user_data);
        if (self)
            self->timer_tick();
    }


    void timer_screen::start_pause_btn_cb(lv_event_t* e) {

        timer_screen* self = static_cast<timer_screen*>(lv_event_get_user_data(e));
        if (!self) return;

        if (!self->m_is_running) {
            // Not running: start
            self->sync_ui(); // ensure set time is fresh
            self->start_timer();
        } else if (self->m_is_paused) {
            // Paused: resume
            self->m_is_paused = false;
            lv_timer_resume(self->m_tick_timer);
            lv_label_set_text(self->m_status_label, "Running");
            lv_obj_set_style_text_color(self->m_status_label, lv_color_hex(0x88FF88), 0);
            lv_label_set_text(lv_obj_get_child(self->m_start_pause_btn, 0), "Pause");
        } else {
            // Running: pause
            self->pause_timer();
        }
    }


    void timer_screen::reset_btn_cb(lv_event_t* e) {

        timer_screen* self = static_cast<timer_screen*>(lv_event_get_user_data(e));
        if (self)
            self->reset_timer();
    }

}
