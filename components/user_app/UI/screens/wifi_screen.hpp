#pragma once

#include "UI/screen.hpp"
#include <vector>
#include <string>

// FORWARD DECLARATIONS =====================================================================================

namespace APP::UI {

    // CONSTANTS ============================================================================================

    // MACROS ===============================================================================================

    // TYPES ================================================================================================

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================

    // @brief Screen that displays available WiFi networks and allows connection.
    class wifi_screen : public screen {
    public:

        wifi_screen();
        ~wifi_screen() override;

        void init() override;

        void show() override;

        void hide() override;

        void destroy() override;

        void recreate_ui() override;

        lv_obj_t* get_root() override { return m_screen; }

        bool handle_event(lv_event_t* e) override;


        // @brief Show a toggle in the manager overlay to enable/disable WiFi.
        bool display_toggle_in_manager() { return true; }


        // @brief Enable or disable WiFi from the manager overlay.
        // @param enable  True to turn WiFi on, false to turn it off.
        void toggle_change_from_manager(const bool enable);


        // @brief Get the current WiFi enabled state.
        // @return  True if WiFi is initialised and active.
        bool get_toggle_state() const;

    private:

        // @brief A known network with SSID and password (static list).
        struct known_network {
            std::string                     ssid;
            std::string                     password;
        };


        // @brief Internal WiFi state machine.
        enum class wifi_state {
            idle = 0,                       // not connected
            scanning,                       // scanning for networks
            connecting,                     // attempting to connect
            connected,                      // successfully connected
            failed,                         // connection failed
            disconnected                    // disconnected after being connected
        };


        // @brief Message structure for UI updates from event handlers.
        struct ui_update_msg {
            char text[128];
        };

        // @brief Start an asynchronous WiFi scan.
        void start_scan_async();


        // @brief Populate the list UI with current scan results.
        void populate_list();


        // @brief Clear any selected network (deselect).
        void clear_selection();


        // @brief Handle the Connect button click – attempt to connect to selected network.
        void on_connect_clicked();


        // @brief Update the status label with a new text (string or C‑string).
        void update_status(const char* text);


        // @brief Update the status label with a new text (string or C‑string).
        void update_status(const std::string& text);


        // @brief Process pending UI messages from the queue.
        void process_ui_queue();


        // @brief Check if NTP sync completed and update the main clock.
        void check_ntp_sync();


        // @brief Fully deinitialise WiFi and stop NTP.
        void stop_wifi();


        // @brief Select a network by its index in the list.
        void select_network_by_index(size_t idx);


        void create_ui_elements();


        // @brief Timer callback for periodic scanning (every 3 seconds).
        static void scan_timer_cb(lv_timer_t* timer);


        // @brief Event callback for the connect button.
        static void connect_btn_event_cb(lv_event_t* e);


        // @brief WiFi event handler (scan done, connected, disconnected).
        static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);


        // @brief IP event handler (got IP).
        static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);


        // ----- UI objects -----
        lv_obj_t*                           m_screen = nullptr;             // root screen object
        lv_obj_t*                           m_pattern_canvas = nullptr;
        lv_obj_t*                           m_list = nullptr;               // list of networks
        lv_obj_t*                           m_status_label = nullptr;       // status text
        lv_obj_t*                           m_connect_btn = nullptr;        // floating connect button (appears next to selected item)

        // ----- Selection state -----
        lv_obj_t*                           m_selected_item = nullptr;      // currently highlighted list item
        size_t                              m_selected_index = 0;           // index of selected network
        bool                                m_device_selected = false;      // whether a network is selected

        // ----- Network data -----
        std::vector<std::string>            m_networks;                     // list of SSIDs from last scan
        static std::vector<known_network>   m_known_networks;               // predefined known networks

        // ----- WiFi state -----
        bool                                m_is_scanning = false;
        wifi_state                          m_wifi_state = wifi_state::idle;
        std::string                         m_connected_ssid;               // SSID currently connected (if any)
        std::string                         m_pending_selection_ssid;       // SSID to re‑select after scan
        bool                                m_ntp_started = false;          // whether NTP client is active
        QueueHandle_t                       m_ui_queue = nullptr;           // queue for UI updates from ISR/events
        lv_timer_t*                         m_scan_timer = nullptr;         // periodic scan timer
        bool                                m_is_visible = false;           // whether the screen is currently shown
        bool                                m_scan_results_ready = false;   // flag to signal new scan results
        portMUX_TYPE                        m_networks_mutex = portMUX_INITIALIZER_UNLOCKED;   // mutex for network list
    };

}
