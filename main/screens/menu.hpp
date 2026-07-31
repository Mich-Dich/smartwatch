
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

    // A full-screen menu that displays all registered screens as side-scrollable tiles.
    class menu_screen : public screen {
    public:

        // @brief Returns the constant identifier "menu". 
        SCREEN_NAME("menu")
    

        // @brief Creates the UI – a horizontal tileview, launch/sync buttons, and the sync state timer.
        // @param screen  LVGL screen object on which to build the menu.
        void create(lv_obj_t* screen) override;

    private:

        // @brief Called when a tile in the carousel is tapped.
        // Finds the corresponding screen and navigates to it.
        static void tile_click_cb(lv_event_t* e);

        std::vector<screen const*>          m_tile_screens{};           // Ordered list of screens shown in the tileview (home + normals).

    };

}
