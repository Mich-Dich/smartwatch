
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

    class home_assist_screen : public screen {
    public:

        home_assist_screen();
        ~home_assist_screen() override;

        void init() override;
    
        void show() override;
    
        void hide() override;
    
        void destroy() override;
    
        lv_obj_t* get_root() override { return m_screen; }
    
        bool handle_event(lv_event_t* e) override;

    private:

        void fetch_data();
    
        void update_ui(int number, const char* status);
    
        void update_ui_error(const char* error);

        static void timer_cb(lv_timer_t* timer);
    
        static void refresh_btn_cb(lv_event_t* e);

        lv_obj_t*               m_screen = nullptr;
        lv_obj_t*               m_status_label = nullptr;
        lv_obj_t*               m_number_label = nullptr;
        lv_obj_t*               m_refresh_btn = nullptr;
        lv_timer_t*             m_fetch_timer = nullptr;
        bool                    m_is_visible = false;
        
    };

}
