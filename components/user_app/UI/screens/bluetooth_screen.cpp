
#include "util/pch.hpp"
#include "bluetooth_screen.hpp"

#include <ble_scan_bsp.h>                   // provides ble_Queue, ble_scan_setconf()

#include "user_app.hpp"
#include "UI/util.hpp"



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

        APP::UI::util::create_geometric_pattern_0(
            m_screen,
            lv_color_mix(highlight_color, lv_color_hex(0x000000), 80), 
            lv_color_mix(support_color, lv_color_hex(0x000000), 80)
        );

        // Status label (unchanged)
        m_status_label = lv_label_create(m_screen);
        lv_label_set_text(m_status_label, "Bluetooth devices");
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(m_status_label, &inconsolata_regular_26, 0);
        lv_obj_align(m_status_label, LV_ALIGN_TOP_MID, 0, 20);

        // List container – make background transparent
        m_list = lv_list_create(m_screen);
        lv_obj_set_size(m_list, LV_PCT(90), LV_PCT(70));
        lv_obj_align(m_list, LV_ALIGN_TOP_MID, 0, 70);
        // REMOVE the solid background
        lv_obj_set_style_bg_opa(m_list, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(m_list, 0, 0);
        lv_obj_set_style_pad_row(m_list, 4, 0);
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

        // Connect button – floating, hidden initially, using Bluetooth symbol
        m_connect_btn = lv_btn_create(m_screen);
        lv_obj_set_size(m_connect_btn, 40, 36);                      // smaller, square-ish
        lv_obj_set_style_bg_color(m_connect_btn, lv_color_hex(0x0066FF), 0); // blue
        lv_obj_set_style_radius(m_connect_btn, 4, 0);
        lv_obj_add_event_cb(m_connect_btn, connect_btn_event_cb, LV_EVENT_CLICKED, this);
        lv_obj_add_flag(m_connect_btn, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t* connect_label = lv_label_create(m_connect_btn);
        lv_label_set_text(connect_label, LV_SYMBOL_BLUETOOTH);      // use symbol
        lv_obj_set_style_text_color(connect_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(connect_label);

        // Timer
        m_update_timer = lv_timer_create(timer_callback, 500, this);
        lv_timer_pause(m_update_timer);

        clear_seen_macs();
        ESP_LOGI(TAG, "bluetooth_screen initialised");
    }


    void bluetooth_screen::show() {

        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        if (m_bluetooth_enabled)
            start_scan();
        
        else {                                  // Show disabled state
            lv_obj_clean(m_list);
            lv_label_set_text(m_status_label, "Bluetooth disabled");
            clear_selection();
        }
    }


    void bluetooth_screen::hide() {

        if (m_screen)
            lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        
        if (m_scanning) {                       // Stop scanning but do NOT deinit BLE
            m_scanning = false;
            lv_timer_pause(m_update_timer);
        }
        clear_selection();                      // Optionally clear selection to avoid stale state
    }


    void bluetooth_screen::destroy() {

        deinit_bluetooth();   // fully deinit BLE
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


    bool bluetooth_screen::handle_event(lv_event_t* e)      { return false; }

    
    void bluetooth_screen::toggle_change_from_manager(const bool enable) {
            
        if (enable) {
            if (!m_bluetooth_enabled) {
                init_bluetooth();
                start_scan();               // start scanning if screen is visible
            }
        } else
            deinit_bluetooth();                 // UI already updated inside deinit
    }


    bool bluetooth_screen::get_toggle_state() const { return m_bluetooth_enabled; }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

    void bluetooth_screen::start_scan() {

        clear_selection();
        lv_obj_clean(m_list);
        clear_seen_macs();
        lv_label_set_text(m_status_label, "Scanning...");
        ble_scan_setconf();     // Start BLE scan (duration = 3 seconds, defined in ble_scan_bsp)
        m_scanning = true;
        lv_timer_resume(m_update_timer);
    }


    bool bluetooth_screen::is_mac_seen(const uint8_t* bda) {

        for (size_t i = 0; i < m_device_count; ++i)
            if (memcmp(m_devices[i].bda, bda, 6) == 0)
                return true;

        return false;
    }


    void bluetooth_screen::clear_seen_macs() {

        memset(m_devices, 0, sizeof(m_devices));
        m_device_count = 0;
    }


    void bluetooth_screen::update_device_list() {

        if (!m_scanning)
            return;

        ble_device_t dev;
        bool added = false;

        while (uxQueueMessagesWaiting(ble_Queue) > 0) {
            if (xQueueReceive(ble_Queue, &dev, 0) == pdTRUE) {
                if (is_mac_seen(dev.bda))
                    continue;

                // Store the full device
                if (m_device_count >= MAX_DEVICES) {
                    ESP_LOGW(TAG, "Device list full");
                    break;
                }
                m_devices[m_device_count] = dev;
                size_t idx = m_device_count;
                m_device_count++;

                // Create a list button for this device
                char display[64];
                if (dev.name[0] != '\0')
                    snprintf(display, sizeof(display), "%s ", dev.name);
                else
                    snprintf(display, sizeof(display), "%02X:%02X:%02X:%02X:%02X:%02X",
                        dev.bda[0], dev.bda[1], dev.bda[2], dev.bda[3], dev.bda[4], dev.bda[5]);

                lv_obj_t* btn = lv_list_add_btn(m_list, nullptr, display);
                lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
                lv_obj_set_style_bg_color(btn, lv_color_hex(0x000000), 0);
                lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
                lv_obj_set_style_border_width(btn, 1, 0);
                lv_obj_set_style_border_color(btn, lv_color_hex(0x333333), 0);
                lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_BOTTOM, 0);
                lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);

                // Store the device pointer (stable because we never reallocate)
                lv_obj_set_user_data(btn, &m_devices[idx]);

                // Click event to select this device
                lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                    lv_obj_t* btn = lv_event_get_target(e);
                    bluetooth_screen* screen = (bluetooth_screen*)lv_event_get_user_data(e);
                    ble_device_t* dev = (ble_device_t*)lv_obj_get_user_data(btn);
                    if (dev) {
                        // Clear previous selection
                        screen->clear_selection();

                        // Store selected device
                        screen->m_selected_device = *dev;
                        screen->m_device_selected = true;
                        screen->m_selected_item = btn;

                        // Highlight this item
                        lv_obj_set_style_bg_opa(btn, LV_OPA_50, 0);
                        lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), 0);

                        // Reparent the connect button to the selected list item
                        lv_obj_set_parent(screen->m_connect_btn, btn);
                        // Align to the right side of the item with a small margin
                        lv_obj_align(screen->m_connect_btn, LV_ALIGN_RIGHT_MID, 10, 0);
                        lv_obj_clear_flag(screen->m_connect_btn, LV_OBJ_FLAG_HIDDEN);
                        // Bring to front so it's above the item's content
                        lv_obj_move_foreground(screen->m_connect_btn);

                        ESP_LOGI(TAG, "Selected device: %s", dev->name[0] ? dev->name : "Unknown");
                    }
                }, LV_EVENT_CLICKED, this);

                added = true;
            }
        }

        if (added) {
            char status[64];
            snprintf(status, sizeof(status), "Found %zu device%s",
                    m_device_count, (m_device_count > 1) ? "s" : "");
            lv_label_set_text(m_status_label, status);
        }
    }


    void bluetooth_screen::clear_selection() {

        if (m_selected_item) {
            lv_obj_set_style_bg_opa(m_selected_item, LV_OPA_TRANSP, 0);
            lv_obj_set_style_bg_color(m_selected_item, lv_color_hex(0x000000), 0);
            m_selected_item = nullptr;
        }
        m_device_selected = false;
        memset(&m_selected_device, 0, sizeof(m_selected_device));
        // Reparent to a safe parent (the screen) and hide
        if (m_connect_btn) {
            lv_obj_set_parent(m_connect_btn, m_screen);
            lv_obj_add_flag(m_connect_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }


    void bluetooth_screen::stop_bluetooth() {
            
        if (m_scanning) {
            m_scanning = false;
            lv_timer_pause(m_update_timer);
        }
        // Stop any ongoing scan (if your BSP provides a stop function)
        // ble_scan_stop();   // optional
        // Deinitialise BLE scanning to save power
        ble_scan_Deinit();   // assuming this shuts down the BLE controller
        ESP_LOGI(TAG, "Bluetooth disabled");
    }


    void bluetooth_screen::init_bluetooth() {

        if (!m_bluetooth_enabled) {
            
            #ifdef ble_scan_Init                                // Initialize BLE controller
                ble_scan_Init();
            #else
                esp_bt_controller_enable(ESP_BT_MODE_BTDM);     // fallback – enable BT controller
            #endif

            m_bluetooth_enabled = true;
            ESP_LOGI(TAG, "Bluetooth enabled");
        }
    }


    void bluetooth_screen::deinit_bluetooth() {

        if (m_bluetooth_enabled) {

            // Stop any ongoing scan
            if (m_scanning) {
                m_scanning = false;
                lv_timer_pause(m_update_timer);
            }
            
            clear_selection();                                  // Clear UI
            lv_obj_clean(m_list);
            lv_label_set_text(m_status_label, "Bluetooth disabled");
            
            ble_scan_Deinit();                                  // Deinit BLE
            m_bluetooth_enabled = false;
            ESP_LOGI(TAG, "Bluetooth disabled");
        }
    }

    void bluetooth_screen::timer_callback(lv_timer_t* timer) {

        auto* screen = static_cast<bluetooth_screen*>(timer->user_data);
        if (screen)
            screen->update_device_list();
 
        // if (ble_is_connected())
        //     lv_label_set_text(m_status_label, "Connected");
        // else if (m_scanning)
        //     lv_label_set_text(m_status_label, "Scanning ...");
        // else
        //     lv_label_set_text(m_status_label, "Bluetooth");
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


    void bluetooth_screen::connect_btn_event_cb(lv_event_t* e) {

        bluetooth_screen* screen = (bluetooth_screen*)lv_event_get_user_data(e);
        if (!screen)
            return;

        if (screen->m_device_selected) {

            esp_err_t err = ble_connect_to_device(screen->m_selected_device.bda);
            if (err == ESP_OK)
                lv_label_set_text(screen->m_status_label, "Connecting...");
            else
                lv_label_set_text(screen->m_status_label, "Connection failed");

        } else
            lv_label_set_text(screen->m_status_label, "Select a device first");
    }

}
