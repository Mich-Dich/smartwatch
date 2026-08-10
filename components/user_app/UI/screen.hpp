
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

    // @brief Abstract base class for all screens.
    //        Each screen manages its own LVGL UI objects and lifecycle.
    class screen {
    public:

        virtual ~screen() = default;


        // @brief Create and initialise all UI objects for this screen.
        //        Called once during registration.
        virtual void init() = 0;


        // @brief Make the screen visible (show) and start any background tasks.
        virtual void show() = 0;


        // @brief Hide the screen (e.g., when switching to another screen).
        virtual void hide() = 0;


        // @brief Free all resources used by this screen.
        virtual void destroy() = 0;


        // @brief recreate the UI after some values have changed, like highlight-color
        virtual void recreate_ui() = 0;


        // @brief Handle LVGL events that are not handled by the screen's own callbacks.
        // @param e  The LVGL event.
        // @return   True if the event was handled; false otherwise.
        virtual bool handle_event(lv_event_t* e) { return false; }


        // @brief Whether this screen should show a toggle switch in the manager overlay.
        // @return  True if a toggle should be shown; false otherwise.
        virtual bool display_toggle_in_manager() { return false; }


        // @brief Called when the manager overlay's toggle is changed by the user.
        // @param enable  New state (true = on, false = off).
        virtual void toggle_change_from_manager(bool enable) {}


        // @brief Get the current state of the toggle (if applicable).
        // @return  Current state (true = on, false = off).
        virtual bool get_toggle_state() const { return false; }


        // @brief Get the root LVGL object (the screen itself).
        // @return  LVGL object pointer.
        virtual lv_obj_t* get_root() = 0;

    };

}
