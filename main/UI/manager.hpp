
#pragma once

#include "UI/screen.hpp"


// FORWARD DECLARATIONS =====================================================================================


namespace APP::UI::screen_manager {

    // CONSTANTS ============================================================================================

    // MACROS ===============================================================================================

    // TYPES ================================================================================================

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // Call once at startup. Creates the very first “home” screen.
    void init(lv_display_t* display);


    // Register a screen (takes ownership of the pointer).
    void add_screen(screen* screen, const screen::type type = screen::type::normal);


    // Switch to the screen with the given name.
    // If `push` is true, the previous screen is pushed on a stack so
    // “back” can return to it.
    void navigate_to(const char* name, const bool push = true);
    

    // Go back to the previous screen (pops from stack).
    // If stack is empty, goes to the home screen.
    void go_back();


    // Return to the home screen (stack is cleared).
    void go_home();

    
    //
    void go_to_menu();


    // Get the currently active screen.
    screen const * current_screen();


    // Returns the home screen (or nullptr)
    screen const * get_home_screen();


    // Returns a const reference to the vector of all normal screens
    const std::vector<screen*>& get_normal_screens();

    
    void init_dim_timer();

    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================

}
