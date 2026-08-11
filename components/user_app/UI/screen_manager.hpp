
#pragma once

#include "screen.hpp"


// FORWARD DECLARATIONS =====================================================================================

namespace APP::UI::screen_manager {

    // CONSTANTS ============================================================================================

    // MACROS ===============================================================================================

    // TYPES ================================================================================================

    // @brief Callback function type for sleep/wake events (no arguments, no return).
    using callback_func =          void (*)();   // pointer to function: void f()

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================

    // @brief Initialise the screen manager (timers, touch polling, etc.).
    //        Must be called once at startup.
    void init();


    // @brief Register a screen with the manager.
    // @param name    Unique identifier for the screen (used in overlay and switching).
    // @param screen  Unique pointer to the screen instance (takes ownership).
    void register_screen(const std::string& name, std::unique_ptr<screen> screen);


    // @brief Switch to the screen with the given name.
    // @param name  Identifier of the screen to switch to.
    void switch_to(const std::string& name);


    // @brief Get a pointer to the currently active screen.
    // @return  Pointer to the current screen, or nullptr if none.
    screen* get_current();


    // @brief Forward an LVGL event to the current screen's handle_event().
    // @param e  LVGL event.
    void handle_event(lv_event_t* e);


    // @brief Register a callback to be executed when the system enters low‑power sleep.
    // @param callback  Function pointer.
    // @return          Index that can be used with remove_sleep_callback().
    u8 add_sleep_callback(const callback_func callback);


    // @brief Remove a previously registered sleep callback.
    // @param index  Index returned by add_sleep_callback().
    void remove_sleep_callback(const u8 index);


    // @brief Register a callback to be executed when the system wakes from low‑power sleep.
    // @param callback  Function pointer.
    // @return          Index for removal.
    u8 add_wake_callback(const callback_func callback);


    // @brief Remove a previously registered wake callback.
    // @param index  Index returned by add_wake_callback().
    void remove_wake_callback(const u8 index);


    // @brief Remove a previously registered wake callback.
    // @param index  Index returned by add_wake_callback().
    bool is_charger_connected();


    void recreate_screens();

    // Request a keep‑alive to prevent display dimming/sleep.
    // Returns a handle (ID) that must be used when releasing.
    u32 request_keep_alive();


    // Release a previously requested keep‑alive.
    // After the last release, dimming is re‑enabled.
    void release_keep_alive(u32& handle);


    // Check if any keep‑alive is currently active.
    bool has_keep_alive();

}
