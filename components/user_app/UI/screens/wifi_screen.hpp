
#pragma once

#include "UI/screen.hpp"


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

    private:

        void populate_list();
        
        void start_scan();
        
        void on_item_clicked(size_t index);
        
        void on_connect_clicked(size_t index);
        
        void on_forget_clicked(size_t index);
        
        void toggle_expand(size_t index);
        
        void update_status(const char* text);

        void update_status(const std::string& text);

        // Static LVGL callbacks
        static void list_item_event_cb(lv_event_t* e);
        
        static void connect_btn_event_cb(lv_event_t* e);
        
        static void forget_btn_event_cb(lv_event_t* e);
        
        static void scan_btn_event_cb(lv_event_t* e);


        struct NetworkItem {
            std::string ssid;
            bool expanded = false;
            lv_obj_t* cont = nullptr;           // container for the whole item
            lv_obj_t* expand_cont = nullptr;    // container for expandable options
            lv_obj_t* connect_btn = nullptr;
            lv_obj_t* forget_btn = nullptr;
        };

        struct KnownNetwork {
            std::string ssid;
            std::string password;
        };
        static std::vector<KnownNetwork>        s_known_networks;   // static list of known SSID/password

        lv_obj_t*                               m_screen = nullptr;
        lv_obj_t*                               m_list = nullptr;
        lv_obj_t*                               m_status_label = nullptr;
        lv_obj_t*                               m_scan_btn = nullptr;

        std::vector<NetworkItem>                m_networks{};
        bool                                    m_is_scanning = false;

    };

}
