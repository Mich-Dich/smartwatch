
#pragma once



// FORWARD DECLARATIONS =====================================================================================

namespace APP::UI {

    // CONSTANTS ============================================================================================

    // MACROS ===============================================================================================

    // TYPES ================================================================================================

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================
    
    class screen {
    public:
    
        virtual ~screen() = default;

        // Lifecycle
        virtual void init() = 0;          // create UI objects
        virtual void show() = 0;          // make visible (if hidden)
        virtual void hide() = 0;          // hide (e.g., for switching)
        virtual void destroy() = 0;       // free resources

        // Event handling – screen can handle its own events
        virtual bool handle_event(lv_event_t* e) { return false; }

        // Get the root LVGL object of this screen
        virtual lv_obj_t* get_root() = 0;
    };

}
