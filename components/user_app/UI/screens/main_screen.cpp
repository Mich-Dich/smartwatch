
#include "util/pch.hpp"
#include "main_screen.hpp"


// FORWARD DECLARATIONS ================================================================================================

extern "C" {

    void setBrightens(uint8_t brig);

}

namespace APP::UI {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    EventGroupHandle_t      TaskEven;
    TaskHandle_t            pxBleTask;
    TaskHandle_t            pxWifiTask;
    QueueHandle_t           ble_Queue;   // declared extern in ble_scan_bsp.h

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================
    
    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    // CLASS IMPLEMENTATION ============================================================================================

    main_screen::main_screen() = default;


    main_screen::~main_screen()     { destroy(); }

    // CLASS PUBLIC ====================================================================================================

    void main_screen::init() {

        ESP_LOGI(TAG, "Initialising main screen");

        // Set up the UI from SquareLine (if any)
        setup_ui(&m_ui);
        events_init(&m_ui);

        lv_obj_t* scr = m_ui.screen;
        if (!scr) {
            ESP_LOGE(TAG, "Screen object is null");
            return;
        }

        // Force a black background
        lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

        // Hide all existing widgets from SquareLine (they are from the old design)
        uint32_t child_cnt = lv_obj_get_child_cnt(scr);
        for (uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t* child = lv_obj_get_child(scr, i);
            if (child != m_time_label) {  // not yet created, but safe
                lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
            }
        }

        // Create a large label for the digital clock
        m_time_label = lv_label_create(scr);
        lv_obj_set_style_text_color(m_time_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(m_time_label, &lv_font_montserrat_12, 0); // big font
        lv_label_set_text(m_time_label, "00:00:00");
        lv_obj_align(m_time_label, LV_ALIGN_CENTER, 0, 0);

        // Initialise clock state
        m_clock.hours = 7;
        m_clock.minutes = 30;
        m_clock.seconds = 30;

        // Update the label immediately
        update_clock();

        // Start the timer to update every second
        start_clock();

        ESP_LOGI(TAG, "main_screen initialised");
    }


    void main_screen::show()            { lv_obj_clear_flag(m_ui.screen, LV_OBJ_FLAG_HIDDEN); }


    void main_screen::hide()            { lv_obj_add_flag(m_ui.screen, LV_OBJ_FLAG_HIDDEN); }


    void main_screen::destroy() {
        // LVGL objects are usually automatically freed when the screen is deleted,
        // but we could call lv_obj_del(m_ui.screen) if needed.
        // For now, we just clear the pointer.
    }


    lv_obj_t* main_screen::get_root() {
        return m_ui.screen;
    }


    // No longer handles any events – all navigation is done by screen_manager.
    bool main_screen::handle_event(lv_event_t* e)   { return false; }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

    void main_screen::out_time(APP::clock_module* clock) {
        
        clock->out_hours = clock->hours * 5;
        clock->out_minutes = clock->minutes;
        clock->out_seconds = clock->seconds;

        uint8_t bat = clock->out_minutes / 12;
        clock->out_hours += bat;

        int16_t hour_angle = clock->out_hours * 6 - 90;
        int16_t min_angle  = clock->out_minutes * 6 - 90;
        int16_t sec_angle  = clock->out_seconds * 6 - 90;

        lv_img_set_angle(m_ui.screen_img_1, hour_angle * 10);
        lv_img_set_angle(m_ui.screen_img_2, min_angle * 10);
        lv_img_set_angle(m_ui.screen_img_3, sec_angle * 10);
    }

    
    void main_screen::start_clock() {

        // Static wrapper for the timer callback
        static auto clock_callback = [](void* arg) {
            auto* screen = static_cast<main_screen*>(arg);
            screen->update_clock();
        };

        const esp_timer_create_args_t args = {
            .callback = clock_callback,
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "main_clock",
            .skip_unhandled_events = false
        };
        esp_timer_handle_t timer = nullptr;
        ESP_ERROR_CHECK(esp_timer_create(&args, &timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(timer, 1000 * 1000)); // 1s
    }


    void main_screen::update_clock() {

        // Increment time
        m_clock.seconds++;
        if (m_clock.seconds >= 60) {
            m_clock.seconds = 0;
            m_clock.minutes++;
            if (m_clock.minutes >= 60) {
                m_clock.minutes = 0;
                m_clock.hours++;
                if (m_clock.hours >= 24) {
                    m_clock.hours = 0;
                }
            }
        }

        // Format HH:MM:SS
        char time_str[9];
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
                m_clock.hours, m_clock.minutes, m_clock.seconds);

        // Update the label
        if (m_time_label) {
            lv_label_set_text(m_time_label, time_str);
        }
    }

}
