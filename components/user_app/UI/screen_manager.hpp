
#pragma once

#include "screen.hpp"


// FORWARD DECLARATIONS =====================================================================================

namespace APP::UI::screen_manager {

    // CONSTANTS ============================================================================================

    // MACROS ===============================================================================================

    // TYPES ================================================================================================

    using callback_func =          void (*)();   // pointer to function: void f()

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================
    
    void init();

    void register_screen(const std::string& name, std::unique_ptr<screen> screen);
    
    void switch_to(const std::string& name);

    screen* get_current();

    void handle_event(lv_event_t* e);

    u8 add_sleep_callback(const callback_func callback);

    void remove_sleep_callback(const u8 index);

    u8 add_wake_callback(const callback_func callback);

    void remove_wake_callback(const u8 index);

    bool is_charger_connected();
}
