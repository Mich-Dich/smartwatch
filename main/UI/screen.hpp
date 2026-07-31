
#pragma once

#include "lvgl.h"



// FORWARD DECLARATIONS =====================================================================================

namespace APP::UI {

    // CONSTANTS ============================================================================================

    // MACROS ===============================================================================================
    
    #define SCREEN_NAME(name_str)       const char* name() const override { return name_str; }

    // TYPES ================================================================================================

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================

    class screen {
    public:
    
        virtual ~screen() = default;


        DEFAULT_GETTER_SETTER(lv_obj_t*, screen_obj)


        enum class type :u8 {
            
            normal = 0,
            home,
            menu,
        };

        
        // Called once to create all UI elements on the given LVGL screen object.
        virtual void create(lv_obj_t* screen_obj) = 0;

        
        // Optional: override to return a user‑friendly name.
        virtual const char* name() const { return "screen"; }

        
        // Called when the screen becomes active (optional).
        virtual void on_enter() {}

        
        // Called when the screen is left (optional).
        virtual void on_exit() {}

    protected:

        lv_obj_t*           m_screen_obj = nullptr;   // set by the manager after creation

    };

}
