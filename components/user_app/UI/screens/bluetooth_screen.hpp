
#pragma once

#include "UI/screen.hpp"


// FORWARD DECLARATIONS =====================================================================================

namespace APP::UI {

    // CONSTANTS ============================================================================================

    // @brief Maximum number of Bluetooth devices stored in the list.
    static constexpr size_t         MAX_DEVICES = 20;

    // MACROS ===============================================================================================

    // TYPES ================================================================================================

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================

    // @brief Screen that scans for and connects to Bluetooth devices.
    class bluetooth_screen : public screen {
    public:

        bluetooth_screen();
        ~bluetooth_screen() override;

        
        void init() override;
        
        void show() override;
        
        void hide() override;
        
        void destroy() override;
        
        lv_obj_t* get_root() override;

        bool handle_event(lv_event_t* e) override;

        
        // @brief Show a toggle in the manager overlay to enable/disable Bluetooth.
        bool display_toggle_in_manager() { return true; }


        // @brief Enable or disable Bluetooth from the manager overlay.
        // @param enable  True to turn Bluetooth on, false to turn it off.
        void toggle_change_from_manager(const bool enable);

        
        // @brief Get the current Bluetooth enabled state.
        // @return  True if Bluetooth is initialised and active.
        bool get_toggle_state() const;

    private:

        // @brief Check if a given MAC address has already been seen.
        bool is_mac_seen(const uint8_t* bda);


        // @brief Clear the list of seen devices.
        void clear_seen_macs();


        // @brief Start a Bluetooth scan.
        void start_scan();


        // @brief Update the device list from the BLE queue.
        void update_device_list();


        // @brief Clear the current selection.
        void clear_selection();


        // @brief Fully deinitialize Bluetooth and stop scanning.
        void stop_bluetooth();


        // @brief Initialise the Bluetooth controller (enable).
        void init_bluetooth();


        // @brief Deinitialise Bluetooth (disable).
        void deinit_bluetooth();

        
        static void timer_callback(lv_timer_t* timer);


        static void scan_btn_event_cb(lv_event_t* e);


        static void adv_btn_event_cb(lv_event_t* e);


        static void connect_btn_event_cb(lv_event_t* e);


        lv_obj_t*               m_screen = nullptr;
        lv_obj_t*               m_list = nullptr;
        lv_obj_t*               m_scan_btn = nullptr;
        lv_obj_t*               m_status_label = nullptr;
        lv_obj_t*               m_adv_btn = nullptr;            // advertising button (not fully used)
        lv_obj_t*               m_connect_btn = nullptr;        // floating connect button
        lv_timer_t*             m_update_timer = nullptr;       // timer to poll the BLE queue

        // ----- BLE state -----
        bool                    m_scanning = false;             // whether a scan is active
        ble_device_t            m_devices[MAX_DEVICES];         // array of discovered devices
        size_t                  m_device_count = 0;             // number of devices currently in the list
        ble_device_t            m_selected_device = {};         // selected device structure
        bool                    m_device_selected = false;      // whether a device is selected
        lv_obj_t*               m_selected_item = nullptr;      // highlighted list item
        bool                    m_bluetooth_enabled = false;    // global Bluetooth state
    };

}
