
#include "util/pch.hpp"
#include "screen_manager.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI::screen_manager {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    static std::vector<std::pair<std::string, std::unique_ptr<screen>>>     g_ordered_screens{};
        
    static screen*                                                          g_current = nullptr;
        
    static std::string                                                      g_current_name;

    static lv_obj_t*                                                        g_gesture_obj = nullptr;

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    static int find_index(const std::string& name);

    static void gesture_cb(lv_event_t* e);

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    static int find_index(const std::string& name) {

        for (size_t i = 0; i < g_ordered_screens.size(); ++i)
            if (g_ordered_screens[i].first == name)
                return static_cast<int>(i);
        
        return -1;
    }


    static void gesture_cb(lv_event_t* e) {

        lv_indev_t* indev = lv_indev_get_act();
        if (!indev) return;

        lv_dir_t dir = lv_indev_get_gesture_dir(indev);
        if (dir == LV_DIR_NONE) return;

        // Find current index
        int cur_idx = find_index(g_current_name);
        if (cur_idx < 0) return;

        int new_idx = cur_idx;
        if (dir == LV_DIR_LEFT) {
            // Swipe left → go to next screen (wrap around)
            new_idx = (cur_idx + 1) % (int)g_ordered_screens.size();
        } else if (dir == LV_DIR_RIGHT) {
            // Swipe right → go to previous screen (wrap around)
            new_idx = (cur_idx - 1 + (int)g_ordered_screens.size()) % (int)g_ordered_screens.size();
        } else {
            return; // ignore other directions (up/down)
        }

        if (new_idx != cur_idx) {
            const std::string& new_name = g_ordered_screens[new_idx].first;
            switch_to(new_name);
        }
    }


    void setup_gesture_detection() {

        lv_disp_t* disp = lv_disp_get_default();
        if (!disp) {
            ESP_LOGE(TAG, "No default display");
            return;
        }
        lv_obj_t* act_scr = lv_disp_get_scr_act(disp);
        if (!act_scr) {
            ESP_LOGE(TAG, "No active screen");
            return;
        }

        // Remove old overlay if any
        if (g_gesture_obj) {
            lv_obj_del(g_gesture_obj);
            g_gesture_obj = nullptr;
        }

        // Create overlay on the active screen
        g_gesture_obj = lv_obj_create(act_scr);
        lv_obj_set_size(g_gesture_obj, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(g_gesture_obj, LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(g_gesture_obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(g_gesture_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_move_foreground(g_gesture_obj);

        lv_obj_add_event_cb(g_gesture_obj, gesture_cb, LV_EVENT_GESTURE, nullptr);
    }
    
    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    void register_screen(const std::string& name, std::unique_ptr<screen> screen) {

        if (!screen) {
            ESP_LOGE(TAG, "Attempt to register null screen under '%s'", name.c_str());
            return;
        }

        // Check for duplicate names
        if (find_index(name) >= 0) {
            ESP_LOGW(TAG, "Screen '%s' already registered - replacing", name.c_str());
            // Remove the old one
            auto it = std::find_if(g_ordered_screens.begin(), g_ordered_screens.end(),
                [&](const auto& pair) { return pair.first == name; });
            if (it != g_ordered_screens.end()) {
                g_ordered_screens.erase(it);
            }
        }

        screen->init();   // let the screen create its UI
        g_ordered_screens.emplace_back(name, std::move(screen));
        ESP_LOGI(TAG, "Registered screen '%s' (total: %zu)", name.c_str(), g_ordered_screens.size());
    }
    

    void switch_to(const std::string& name) {

        int idx = find_index(name);
        if (idx < 0) {
            ESP_LOGE(TAG, "Screen '%s' not found", name.c_str());
            return;
        }

        // Remove gesture callback from the current screen (if any)
        if (g_current) {
            lv_obj_t* old_root = g_current->get_root();
            if (old_root) {
                lv_obj_remove_event_cb(old_root, gesture_cb);
            }
            g_current->hide();
        }

        g_current = g_ordered_screens[idx].second.get();
        g_current_name = name;
        g_current->show();

        lv_obj_t* root = g_current->get_root();
        if (root) {
            lv_scr_load(root);

            // Enable gesture detection on the root screen
            lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(root, LV_OBJ_FLAG_GESTURE_BUBBLE);
            lv_obj_add_event_cb(root, gesture_cb, LV_EVENT_GESTURE, nullptr);
        } else {
            ESP_LOGE(TAG, "Screen '%s' has null root", name.c_str());
        }

        // No longer call setup_gesture_detection()
        ESP_LOGI(TAG, "Switched to screen '%s'", name.c_str());
    }


    screen* get_current()                       { return g_current; }

        
    // Forward LVGL events to the active screen
    void handle_event(lv_event_t* e) {

        if (g_current)
            g_current->handle_event(e);
            
        // If no current screen, the event is silently ignored.
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
