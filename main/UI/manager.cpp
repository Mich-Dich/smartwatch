
#include "util/pch.hpp"
#include "manager.hpp"

#include "UI/screen.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI::screen_manager {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    constexpr const char*                                               TAG = "screen_manager";

    static constexpr u32                                                DIM_DELAY_MS = 4 * 1000;    // 30 seconds

    static constexpr u8                                                 DIM_BRIGHTNESS = 10;     // 0‑255, 10 = very dim

    static constexpr u8                                                 NORMAL_BRIGHTNESS = 100; // adjust to your display

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    static std::unordered_map<APP::UI::screen::type, APP::UI::screen*>  s_special_screens{};    // special screen like: home, menu

    static std::vector<APP::UI::screen*>                                s_screens{};            // normal screen
    
    static APP::UI::screen*                                             s_current = nullptr;
    
    static std::vector<APP::UI::screen*>                                s_nav_stack{};          // screens pushed by navigate_to(…, true)

    static lv_timer_t*                                                  s_dim_timer = nullptr;

    static u32                                                          s_last_touch_tick  = 0;

    static bool                                                         s_is_dimmed = false;

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    static void global_gesture_handler(lv_event_t* e);
    
    static screen* find_by_name(const char* name);

    static void touch_activity_cb(lv_event_t* e);

    static void dim_timer_cb(lv_timer_t* t);

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================
    
    // Global (static) gesture callback
    static void global_gesture_handler(lv_event_t* e) {

        lv_dir_t dir = lv_indev_get_gesture_dir(lv_event_get_indev(e));

        // Right swipe = go back
        if (dir == LV_DIR_BOTTOM) {

            go_to_menu();
            ESP_LOGI(TAG, "MENU gesture recognised");
        }

        // Swipe up to go home
        if (dir == LV_DIR_TOP) {

            go_home();
            ESP_LOGI(TAG, "HOME gesture recognised");
        }
        touch_activity_cb(e);
    }


    screen* find_by_name(const char* name) {

        auto it = std::find_if(s_screens.begin(), s_screens.end(), 
            [name](const screen* s) { return strcmp(s->name(), name) == 0; });
            
        return (it != s_screens.end()) ? *it : nullptr;
    }


    static void touch_activity_cb(lv_event_t* e) {

        ESP_LOGI(TAG, "Touch registered");

        s_last_touch_tick = lv_tick_get();
        if (s_is_dimmed) {                                              // If display is dimmed, restore it immediately
            bsp_display_brightness_set(NORMAL_BRIGHTNESS);              // or bsp_display_backlight_on()
            s_is_dimmed = false;
        }
    }

    // Timer callback – checks idle time every 500 ms
    static void dim_timer_cb(lv_timer_t* t) {

        if (s_is_dimmed) return;                                        // already dim, touch will wake it
        if (lv_tick_elaps(s_last_touch_tick) >= DIM_DELAY_MS) {
            bsp_display_brightness_set(DIM_BRIGHTNESS);                 // dim the display
            s_is_dimmed = true;
        }
    }

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    void init(lv_display_t* display) {

        // The default active screen will be used as the home screen initially.
        // Usually the first screen created becomes the “home”.
    }


    void add_screen(screen* screen, const screen::type type) {

        switch (type) {                                                                 // save screen pointer

            case screen::type::normal:      s_screens.push_back(screen); break;
            default:                        s_special_screens[type] = screen; break;    // overwrite if needed
        }

        // Create the LVGL screen object (but don't load it yet)
        screen->set_screen_obj(lv_obj_create(nullptr));                                 // create a new screen
        screen->create(screen->get_screen_obj());
        lv_obj_add_event_cb(screen->get_screen_obj(), global_gesture_handler, LV_EVENT_GESTURE, nullptr);
        lv_obj_add_event_cb(screen->get_screen_obj(), touch_activity_cb, LV_EVENT_PRESSING, nullptr);

        if (type == screen::type::home) {                                               // if home screen -> show it

            lv_screen_load(screen->get_screen_obj());                                   // show it immediately
            s_current = screen;
            screen->on_enter();
        }
    }


    void navigate_to(const char* name, bool push) {

        screen* target = find_by_name(name);
        if (!target || target == s_current)
            return;

        if (s_current)
            s_current->on_exit();

        if (push && s_current)
            s_nav_stack.push_back(s_current);

        lv_screen_load_anim(target->get_screen_obj(), LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        s_current = target;
        s_current->on_enter();
    }

    
    void go_back() {

        if (s_nav_stack.empty()) {
            go_home();
            return;
        }

        if (s_current)
            s_current->on_exit();

        screen* previous = s_nav_stack.back();
        s_nav_stack.pop_back();

        lv_screen_load_anim(previous->get_screen_obj(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
        s_current = previous;
        s_current->on_enter();
    }


    void go_home() {

        if (s_special_screens.contains(screen::type::home)) {
            screen* p_home = s_special_screens.at(screen::type::home);
            if (p_home && p_home != s_current) {

                if (s_current)
                    s_current->on_exit();
                lv_screen_load_anim(p_home->get_screen_obj(), LV_SCR_LOAD_ANIM_MOVE_TOP, 300, 0, false);
                s_current = p_home;
                s_current->on_enter();
            }
        }
        s_nav_stack.clear();
    }


    void go_to_menu() {

        if (s_special_screens.contains(screen::type::menu)) {
            screen* p_menu = s_special_screens.at(screen::type::menu);
            if (p_menu && p_menu != s_current) {
                
                if (s_current)
                    s_current->on_exit();
                lv_screen_load_anim(p_menu->get_screen_obj(), LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 300, 0, false);
                s_current = p_menu;
                s_current->on_enter();
            }
        }
        s_nav_stack.clear();
    }


    screen const * current_screen() { return s_current; }


    screen const * get_home_screen() {

        if (s_special_screens.contains(screen::type::home))
            return s_special_screens.at(screen::type::home);
        return nullptr;
    }


    const std::vector<screen*>& get_normal_screens() { return s_screens; }


    void init_dim_timer() {

        // Register for ALL press events on the active input device
        lv_indev_t* indev = lv_indev_get_act();                                         // the touchscreen
        if (indev)
            lv_indev_add_event_cb(indev, touch_activity_cb, LV_EVENT_PRESSED, nullptr);

        s_last_touch_tick = lv_tick_get();                                              // Initialize the “last touch” time to now
        s_dim_timer = lv_timer_create(dim_timer_cb, 500, nullptr);                      // Create a timer that checks idle state
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
