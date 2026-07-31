
#include "util/pch.hpp"
#include "settings.hpp"

#include "util/wifi.hpp"
#include "esp_timer.h"



// FORWARD DECLARATIONS ================================================================================================
namespace APP {
    extern esp_lcd_panel_handle_t       lcd_panel;
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

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    void settings_screen::create(lv_obj_t* screen) {

        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
        lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(screen, 20, 0);

        // Title
        lv_obj_t* title = lv_label_create(screen);
        lv_label_set_text(title, "Settings");
        lv_obj_set_style_text_color(title, lv_color_hex(0x6699CC), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_36, 0);
        lv_obj_set_style_pad_bottom(title, 20, 0);

        // --- Brightness ---
        lv_obj_t* brightness_label = lv_label_create(screen);
        lv_label_set_text(brightness_label, "Brightness");
        lv_obj_set_style_text_color(brightness_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(brightness_label, &lv_font_montserrat_22, 0);

        m_brightness_slider = lv_slider_create(screen);
        lv_obj_set_size(m_brightness_slider, LV_PCT(80), 10);
        lv_slider_set_range(m_brightness_slider, 10, 100);   // min 10% to avoid black screen
        lv_slider_set_value(m_brightness_slider, 80, LV_ANIM_OFF);
        lv_obj_add_event_cb(m_brightness_slider, brightness_slider_cb, LV_EVENT_VALUE_CHANGED, this);

        // --- 24‑Hour format ---
        lv_obj_t* format_label = lv_label_create(screen);
        lv_label_set_text(format_label, "24-Hour Format");
        lv_obj_set_style_text_color(format_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(format_label, &lv_font_montserrat_22, 0);
        lv_obj_set_style_pad_top(format_label, 10, 0);

        m_24h_switch = lv_switch_create(screen);
        lv_obj_add_event_cb(m_24h_switch, format_switch_cb, LV_EVENT_VALUE_CHANGED, this);

        // --- Sync Time ---
        lv_obj_set_style_pad_top(screen, 30, 0);  // extra space before button
        lv_obj_t* sync_btn = lv_button_create(screen);
        lv_obj_set_style_bg_color(sync_btn, lv_color_hex(0x336699), 0);
        lv_obj_set_style_pad_all(sync_btn, 10, 0);
        lv_obj_set_style_radius(sync_btn, 8, 0);

        lv_obj_t* btn_label = lv_label_create(sync_btn);
        lv_label_set_text(btn_label, "Sync Time");
        lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_24, 0);
        lv_obj_center(btn_label);

        lv_obj_add_event_cb(sync_btn, sync_time_button_cb, LV_EVENT_CLICKED, this);

        // Start the sync UI monitor timer
        lv_timer_create(sync_ui_update_timer_cb, 100, this);
    }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

    void settings_screen::brightness_slider_cb(lv_event_t* e) {

        lv_obj_t* slider = lv_event_get_target_obj(e);
        int32_t value = lv_slider_get_value(slider);
        if (lcd_panel)
            esp_lcd_panel_set_brightness(lcd_panel, value); // Convert percentage to hardware brightness (typical range 0–255 or 0–100)

        ESP_LOGI("Settings", "Brightness set to %ld%%", value);
    }


    void settings_screen::format_switch_cb(lv_event_t* e) {

        lv_obj_t* sw = lv_event_get_target_obj(e);
        bool is_24h = lv_obj_has_state(sw, LV_STATE_CHECKED);
        // Store globally (could use NVS later)
        // For now, a global variable can be read by the clock screen.
        // Example: g_use_24h_format = is_24h;
        ESP_LOGI("Settings", "Time format: %s", is_24h ? "24h" : "12h");
    }


    void settings_screen::sync_time_button_cb(lv_event_t* e) {

        auto* self = static_cast<settings_screen*>(lv_event_get_user_data(e));
        if (self->m_sync_running) return;

        lv_obj_t* btn = lv_event_get_target_obj(e);
        self->m_sync_btn = btn;
        lv_obj_add_state(btn, LV_STATE_DISABLED);
        lv_obj_t* label = lv_obj_get_child(btn, 0);
        lv_label_set_text(label, "Syncing...");
        self->m_sync_running = true;

        xTaskCreate(sync_time_task, "sync_task", 4096, self, 5, NULL);
    }


    void settings_screen::sync_ui_update_timer_cb(lv_timer_t* timer) {

        auto* self = static_cast<settings_screen*>(lv_timer_get_user_data(timer));
        if (!self || !self->m_sync_done) return;

        if (self->m_sync_btn) {
            lv_obj_clear_state(self->m_sync_btn, LV_STATE_DISABLED);
            lv_obj_t* label = lv_obj_get_child(self->m_sync_btn, 0);
            lv_label_set_text(label, self->m_sync_success ? "Synced!" : "Failed");

            lv_timer_create([](lv_timer_t* t) {
                lv_obj_t* btn = static_cast<lv_obj_t*>(lv_timer_get_user_data(t));
                if (btn) {
                    lv_obj_t* label = lv_obj_get_child(btn, 0);
                    lv_label_set_text(label, "Sync Time");
                }
                lv_timer_delete(t);
            }, 2000, self->m_sync_btn);
        }

        self->m_sync_running = false;
        self->m_sync_done    = false;
    }


    void settings_screen::sync_time_task(void* param) {

        auto* self = static_cast<settings_screen*>(param);
        const TickType_t start_ticks = xTaskGetTickCount();
        const TickType_t timeout_ticks = pdMS_TO_TICKS(45000);

        bool ok = false;
        while (1) {
            ok = APP::wifi::sync_time();
            if (ok || (xTaskGetTickCount() - start_ticks) >= timeout_ticks) break;
            vTaskDelay(pdMS_TO_TICKS(5000));
        }

        self->m_sync_success = ok;
        self->m_sync_done = true;
        vTaskDelete(NULL);
    }

}
