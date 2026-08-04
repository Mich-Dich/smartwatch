
#include "util/pch.hpp"
#include "bluetooth_screen.hpp"

#include <ble_scan_bsp.h>                   // provides ble_Queue, ble_scan_setconf()


// FORWARD DECLARATIONS ================================================================================================

extern "C" {

    extern QueueHandle_t ble_Queue;

    extern void ble_scan_setconf(void);

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

    bluetooth_screen::bluetooth_screen() = default;


    bluetooth_screen::~bluetooth_screen() { destroy(); }
    
    // CLASS PUBLIC ====================================================================================================

    void bluetooth_screen::init() {

        if (m_screen)
            return;   // already initialised

        ESP_LOGI(TAG, "Initialising bluetooth screen");

        m_screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(m_screen, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(m_screen, LV_OPA_COVER, 0);
        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);

        // Status label
        m_status_label = lv_label_create(m_screen);
        lv_label_set_text(m_status_label, "Press Scan to discover devices");
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(m_status_label, &inconsolata_regular_26, 0);
        lv_obj_align(m_status_label, LV_ALIGN_TOP_MID, 0, 20);

        // List container – enable event bubbling
        m_list = lv_list_create(m_screen);
        lv_obj_set_size(m_list, LV_PCT(90), LV_PCT(70));
        lv_obj_align(m_list, LV_ALIGN_TOP_MID, 0, 70);
        lv_obj_set_style_bg_color(m_list, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_border_width(m_list, 0, 0);
        lv_obj_set_style_pad_row(m_list, 4, 0);
        // Allow events to bubble up to the screen root
        lv_obj_add_flag(m_list, LV_OBJ_FLAG_EVENT_BUBBLE);

        // Create a horizontal container at bottom
        lv_obj_t* btn_container = lv_obj_create(m_screen);
        lv_obj_set_size(btn_container, LV_PCT(90), LV_SIZE_CONTENT);
        lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_container, 0, 0);
        lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Scan button
        m_scan_btn = lv_btn_create(btn_container);
        lv_obj_set_size(m_scan_btn, 100, 40);
        lv_obj_set_style_bg_color(m_scan_btn, lv_color_hex(0x0066FF), 0);
        lv_obj_add_event_cb(m_scan_btn, scan_btn_event_cb, LV_EVENT_CLICKED, this);
        lv_obj_add_flag(m_scan_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_t* scan_label = lv_label_create(m_scan_btn);
        lv_label_set_text(scan_label, "Scan");
        lv_obj_center(scan_label);

        // Advertising toggle button
        m_adv_btn = lv_btn_create(btn_container);
        lv_obj_set_size(m_adv_btn, 100, 40);
        lv_obj_set_style_bg_color(m_adv_btn, lv_color_hex(0x00AA00), 0); // green when off, will update
        lv_obj_add_event_cb(m_adv_btn, adv_btn_event_cb, LV_EVENT_CLICKED, this);
        lv_obj_add_flag(m_adv_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_t* adv_label = lv_label_create(m_adv_btn);
        lv_label_set_text(adv_label, "Advertise");
        lv_obj_center(adv_label);

        // Store the label for later update (optional)
        lv_obj_set_user_data(m_adv_btn, adv_label); // we'll use this to change text

        lv_obj_t* btn_label = lv_label_create(m_scan_btn);
        lv_label_set_text(btn_label, "Scan");
        lv_obj_center(btn_label);

        // Timer
        m_update_timer = lv_timer_create(timer_callback, 500, this);
        lv_timer_pause(m_update_timer);

        clear_seen_macs();

        ESP_LOGI(TAG, "bluetooth_screen initialised");
    }


    void bluetooth_screen::show() {

        if (m_screen)
            lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
    }

    void bluetooth_screen::hide() {

        if (m_screen)
            lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        
        if (m_scanning) {                   // Stop scanning if ongoing
            // Optionally stop the BLE scan, but we let it finish naturally.
            m_scanning = false;
            lv_timer_pause(m_update_timer);
        }
    }

    void bluetooth_screen::destroy() {

        if (m_update_timer) {
            lv_timer_del(m_update_timer);
            m_update_timer = nullptr;
        }
        if (m_screen) {
            lv_obj_del(m_screen);
            m_screen = nullptr;
        }
    }


    lv_obj_t* bluetooth_screen::get_root()                  { return m_screen; }


    // No custom event handling yet
    bool bluetooth_screen::handle_event(lv_event_t* e)      { return false; }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

    void bluetooth_screen::start_scan() {
        
        // Clear the list
        lv_obj_clean(m_list);
        clear_seen_macs();
        lv_label_set_text(m_status_label, "Scanning...");

        // Start BLE scan (duration = 3 seconds, defined in ble_scan_bsp)
        ble_scan_setconf();

        m_scanning = true;
        lv_timer_resume(m_update_timer);
    }


    bool bluetooth_screen::is_mac_seen(const uint8_t* bda) {

        for (size_t i = 0; i < m_seen_count; ++i)
            if (memcmp(m_seen_macs[i].data(), bda, 6) == 0)
                return true;

        return false;
    }


    void bluetooth_screen::add_seen_mac(const uint8_t* bda) {

        if (m_seen_count < MAX_DEVICES) {
            memcpy(m_seen_macs[m_seen_count].data(), bda, 6);
            ++m_seen_count;
        }
    }


    void bluetooth_screen::clear_seen_macs()        { m_seen_count = 0; }


    void bluetooth_screen::update_device_list() {

        if (!m_scanning)
            return;

        ble_device_t dev;
        int count = 0;

        while (uxQueueMessagesWaiting(ble_Queue) > 0) {
            if (xQueueReceive(ble_Queue, &dev, 0) == pdTRUE) {
                if (is_mac_seen(dev.bda))
                    continue;

                add_seen_mac(dev.bda);

                char display[64];
                if (dev.name[0] != '\0')
                    snprintf(display, sizeof(display), "%s", dev.name);
                else
                    snprintf(display, sizeof(display),
                             "%02X:%02X:%02X:%02X:%02X:%02X",
                             dev.bda[0], dev.bda[1], dev.bda[2],
                             dev.bda[3], dev.bda[4], dev.bda[5]);

                lv_obj_t* item = lv_list_add_btn(m_list, nullptr, display);
                // Style and enable event bubbling on the item as well
                lv_obj_set_style_text_color(item, lv_color_hex(0xFFFFFF), 0);
                lv_obj_set_style_bg_color(item, lv_color_hex(0x000000), 0);
                lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, 0);
                lv_obj_set_style_border_width(item, 1, 0);
                lv_obj_set_style_border_color(item, lv_color_hex(0x333333), 0);
                lv_obj_set_style_border_side(item, LV_BORDER_SIDE_BOTTOM, 0);
                // This ensures touches on the item bubble up to the root
                lv_obj_add_flag(item, LV_OBJ_FLAG_EVENT_BUBBLE);
                count++;
            }
        }

        if (count > 0) {
            char status[64];
            snprintf(status, sizeof(status), "Found %zu device%s",
                     m_seen_count, (m_seen_count > 1) ? "s" : "");
            lv_label_set_text(m_status_label, status);
        }
    }


    void bluetooth_screen::timer_callback(lv_timer_t* timer) {

        auto* screen = static_cast<bluetooth_screen*>(timer->user_data);
        if (screen)
            screen->update_device_list();
    }


    void bluetooth_screen::scan_btn_event_cb(lv_event_t* e) {

        auto* screen = static_cast<bluetooth_screen*>(lv_event_get_user_data(e));
        if (screen)
            screen->start_scan();
    }


    void bluetooth_screen::adv_btn_event_cb(lv_event_t* e) {

        auto* screen = static_cast<bluetooth_screen*>(lv_event_get_user_data(e));
        if (!screen)
            return;
        
        bool is_adv = ble_is_advertising();
        if (is_adv) {
            ble_advertising_stop();
            lv_obj_set_style_bg_color(screen->m_adv_btn, lv_color_hex(0x00AA00), 0);
            lv_obj_t* label = lv_obj_get_child(screen->m_adv_btn, 0);
            lv_label_set_text(label, "Advertise");
        } else {
            ble_advertising_start("ESP32_Smartwatch", 0x00FF); // use your desired name and service UUID
            lv_obj_set_style_bg_color(screen->m_adv_btn, lv_color_hex(0xFF0000), 0);
            lv_obj_t* label = lv_obj_get_child(screen->m_adv_btn, 0);
            lv_label_set_text(label, "Stop Adv");
        }
    }
}
