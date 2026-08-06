
#pragma once

#include "UI/screen.hpp"


// FORWARD DECLARATIONS =====================================================================================

namespace APP::UI {

    // CONSTANTS ============================================================================================

    static constexpr size_t         MAX_DEVICES = 20;

    // MACROS ===============================================================================================

    // TYPES ================================================================================================

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================

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

    private:

        bool is_mac_seen(const uint8_t* bda);

        void clear_seen_macs();

        void start_scan();

        void update_device_list();

        void clear_selection();

        void stop_bluetooth();
        
        static void timer_callback(lv_timer_t* timer);

        static void scan_btn_event_cb(lv_event_t* e);

        static void adv_btn_event_cb(lv_event_t* e);

        static void connect_btn_event_cb(lv_event_t* e);


        lv_obj_t*               m_screen = nullptr;
        lv_obj_t*               m_list = nullptr;
        lv_obj_t*               m_scan_btn = nullptr;
        lv_obj_t*               m_status_label = nullptr;
        lv_obj_t*               m_adv_btn = nullptr;
        lv_obj_t*               m_connect_btn = nullptr;
        lv_timer_t*             m_update_timer = nullptr;
        bool                    m_scanning = false;
        ble_device_t            m_devices[MAX_DEVICES];
        size_t                  m_device_count = 0;
        ble_device_t            m_selected_device = {};
        bool                    m_device_selected = false;
        lv_obj_t*               m_selected_item = nullptr; 

    };

}
