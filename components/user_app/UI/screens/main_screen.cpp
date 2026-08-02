
#include "util/pch.hpp"
#include "main_screen.hpp"

#include "UI/util.hpp"


// FORWARD DECLARATIONS ================================================================================================

extern "C" {

    void setBrightens(u8 brig);

}

namespace APP::UI {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================
    
    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    f32 get_battery_voltage () {

        f32 adc_value;
        adc_get_value(&adc_value);      // Read the battery voltage
        return adc_value;
    }


    u8 convert_battery_voltage_to_percent(const f32 voltage) {

        constexpr f32 V_EMPTY = 3.45f;
        constexpr f32 V_FULL  = 3.82f;
        f32 percent = (voltage - V_EMPTY) * 100.0f / (V_FULL - V_EMPTY);
        
        // if (percent < 0.f)
        //     percent = 0.f;
        if (percent > 100.f)
            percent = 100.f;

        return static_cast<u8>(percent);
    }

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    // CLASS IMPLEMENTATION ============================================================================================

    main_screen::main_screen()          = default;


    main_screen::~main_screen()         { destroy(); }

    // CLASS PUBLIC ====================================================================================================

    void main_screen::init() {

        ESP_LOGI(TAG, "Initialising main screen");

        // Setup UI from SquareLine (if any) – keep this if needed for other widgets
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
            // Do not hide newly created labels (they aren't created yet)
            lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
        }

        // Create Hour label
        m_hour_label = lv_label_create(scr);
        lv_obj_set_style_text_color(m_hour_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(m_hour_label, &inconsolata_regular_128, 0);
        lv_label_set_text(m_hour_label, "00");
        lv_obj_align(m_hour_label, LV_ALIGN_CENTER, 0, -60); // adjust Y offset as needed

        // Create Minute label
        m_minute_label = lv_label_create(scr);
        lv_obj_set_style_text_color(m_minute_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(m_minute_label, &inconsolata_regular_128, 0);
        lv_label_set_text(m_minute_label, "00");
        lv_obj_align(m_minute_label, LV_ALIGN_CENTER, 0, 60); // adjust Y offset

        // Create Second label (next to minute, blue, smaller)
        m_second_label = lv_label_create(scr);
        lv_obj_set_style_text_color(m_second_label, lv_color_hex(0x0066FF), 0); // blue
        lv_obj_set_style_text_font(m_second_label, &inconsolata_regular_48, 0); // or lv_font_montserrat_48
        lv_label_set_text(m_second_label, "00");
        // Align to the right of the minute label
        lv_obj_align_to(m_second_label, m_minute_label, LV_ALIGN_OUT_RIGHT_MID, 15, -20);

        // Create Battery label (top right)
        m_battery_label = lv_label_create(scr);
        lv_obj_set_style_text_color(m_battery_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(m_battery_label, &inconsolata_regular_26, 0);
        lv_label_set_text(m_battery_label, "0.0V");
        lv_obj_align(m_battery_label, LV_ALIGN_TOP_RIGHT, -10, 10); // 10px padding

        update_clock();                     // Update all labels immediately
        start_clock();                      // Start the timer to update every second

        ESP_LOGI(TAG, "main_screen initialised");
    }


    void main_screen::show()            { lv_obj_clear_flag(m_ui.screen, LV_OBJ_FLAG_HIDDEN); }


    void main_screen::hide()            { lv_obj_add_flag(m_ui.screen, LV_OBJ_FLAG_HIDDEN); }


    // LVGL objects are usually automatically freed when the screen is deleted,
    // but we could call lv_obj_del(m_ui.screen) if needed.
    // For now, we just clear the pointer.
    void main_screen::destroy()         { }


    lv_obj_t* main_screen::get_root()   { return m_ui.screen; }


    // No longer handles any events – all navigation is done by screen_manager.
    bool main_screen::handle_event(lv_event_t* e)   { return false; }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================
    
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
                if (m_clock.hours >= 24)
                    m_clock.hours = 0;
            }
        }

        // Format strings
        char hour_str[3], min_str[3], sec_str[3];
        snprintf(hour_str, sizeof(hour_str), "%02d", m_clock.hours);
        snprintf(min_str, sizeof(min_str), "%02d", m_clock.minutes);
        snprintf(sec_str, sizeof(sec_str), "%02d", m_clock.seconds);

        // Update labels
        if (m_hour_label)   lv_label_set_text(m_hour_label, hour_str);
        if (m_minute_label) lv_label_set_text(m_minute_label, min_str);
        if (m_second_label) lv_label_set_text(m_second_label, sec_str);

        // Get raw percentage
        const u8 raw = convert_battery_voltage_to_percent(get_battery_voltage());

        // Store in buffer
        m_battery_buffer[m_battery_index] = raw;
        m_battery_index = (m_battery_index + 1) % BATTERY_BUFFER_SIZE;
        if (m_battery_count < BATTERY_BUFFER_SIZE) 
            m_battery_count++;

        // Compute average over available readings
        u16 sum = 0;
        for (u8 i = 0; i < m_battery_count; i++)
            sum += m_battery_buffer[i];

        // Display the averaged percentage
        const i16 avg = sum / m_battery_count;
        char volt_str[12];
        snprintf(volt_str, sizeof(volt_str), "%.2fV %d%%", get_battery_voltage(), avg);
        // snprintf(volt_str, sizeof(volt_str), "%d%%", avg);
        if (m_battery_label)
            lv_label_set_text(m_battery_label, volt_str);
    }

}
