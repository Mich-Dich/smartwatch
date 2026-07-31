
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

    class music_screen : public screen {
    public:

        SCREEN_NAME("Music")

        void create(lv_obj_t* screen) override;

    private:

        lv_obj_t*                       m_list = nullptr;       // the scrollable list container
        std::vector<std::string>        m_files;                // full filenames (for future playback)

    };

}
