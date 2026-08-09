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

    class wifi_screen : public screen {
    public:

        wifi_screen();
        ~wifi_screen() override;
        
        void init() override;
        
        void show() override;
        
        void hide() override;
        
        void destroy() override;
        
        lv_obj_t* get_root() override { return m_screen; }
        
        bool handle_event(lv_event_t* e) override;

        bool display_toggle_in_manager() { return true; }

        void toggle_change_from_manager(const bool enable);

        bool get_toggle_state() const;

    private:

        // Internal types
        struct known_network {
            std::string ssid;
            std::string password;
        };

        enum class wifi_state {
            idle,
            scanning,
            connecting,
            connected,
            failed,
            disconnected
        };

        struct ui_update_msg {
            char text[128];
        };

        // Lifecycle helpers
        void start_scan_async();

        void populate_list();

        void clear_selection();

        void on_connect_clicked();

        void update_status(const char* text);

        void update_status(const std::string& text);

        void process_ui_queue();

        void check_ntp_sync();

        void stop_wifi();

        void select_network_by_index(size_t idx);

        static void scan_timer_cb(lv_timer_t* timer);

        static void connect_btn_event_cb(lv_event_t* e);

        static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

        static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

        // UI objects
        lv_obj_t*                               m_screen = nullptr;
        lv_obj_t*                               m_list = nullptr;
        lv_obj_t*                               m_status_label = nullptr;
        lv_obj_t*                               m_connect_btn = nullptr;        // floating connect button

        // Selection state
        lv_obj_t*                               m_selected_item = nullptr;
        size_t                                  m_selected_index = 0;
        bool                                    m_device_selected = false;

        // Network data
        std::vector<std::string>                m_networks{};                   // just SSIDs
        static std::vector<known_network>       m_known_networks;               // static list of known SSID/password

        // WiFi state
        bool                                    m_is_scanning = false;
        wifi_state                              m_wifi_state  = wifi_state::idle;
        std::string                             m_connected_ssid{};
        std::string                             m_pending_selection_ssid{};

        bool                                    m_ntp_started = false;          // NTP sync
        QueueHandle_t                           m_ui_queue = nullptr;           // UI update queue

        lv_timer_t*                             m_scan_timer = nullptr;
        bool                                    m_is_visible = false;
        bool                                    m_scan_results_ready = false;
        portMUX_TYPE                            m_networks_mutex = portMUX_INITIALIZER_UNLOCKED;
    };

}
