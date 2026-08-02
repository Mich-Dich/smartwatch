#include "util/pch.hpp"
#include "screen_manager.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI::screen_manager {

    // TYPES ===========================================================================================================

    struct vec_2d {

        i8          x{};
        i8          y{};

    };
    
    // Equality / inequality
    bool operator==(const vec_2d& a, const vec_2d& b)   { return a.x == b.x && a.y == b.y; }
    bool operator!=(const vec_2d& a, const vec_2d& b)   { return !(a == b); }

    // Component‑wise ordering (partial order)
    bool operator< (const vec_2d& a, const vec_2d& b)   { return a.x < b.x && a.y < b.y; }
    bool operator> (const vec_2d& a, const vec_2d& b)   { return b < a; }                     // or a.x > b.x && a.y > b.y
    bool operator<=(const vec_2d& a, const vec_2d& b)   { return a.x <= b.x && a.y <= b.y; }
    bool operator>=(const vec_2d& a, const vec_2d& b)   { return b <= a; }

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    static std::vector<std::pair<std::string, std::unique_ptr<screen>>>     g_ordered_screens{};

    static screen*                                                          g_current = nullptr;

    static std::string                                                      g_current_name;

    static vec_2d                                                           g_touch_min = {};       // -1: not set; 0-100 the coordinate in percent

    static vec_2d                                                           g_touch_max = {};       // -1: not set; 0-100 the coordinate in percent

    static lv_obj_t* overlay_cont = nullptr;
    static lv_obj_t* overlay_list = nullptr;
    static bool overlay_shown = false;

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    static int find_index(const std::string& name);

    static void touch_begin_cb(lv_event_t* e);

    static void touch_end_cb(lv_event_t* e);

    static void touch_cb(lv_event_t* e);

    static vec_2d lv_point_to_percent_coordinates(const lv_point_t point);

    static void create_overlay_if_needed();
    
    static void show_overlay();

    static void hide_overlay();
    
    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    static int find_index(const std::string& name) {

        for (size_t i = 0; i < g_ordered_screens.size(); ++i)
            if (g_ordered_screens[i].first == name)
                return static_cast<int>(i);
        
        return -1;
    }


    static void touch_begin_cb(lv_event_t* e) {

        ESP_LOGI(TAG, "Touch start detected");
        lv_indev_t* indev = lv_indev_get_act();
        if (!indev) 
            return;

        lv_point_t point;
        lv_indev_get_point(indev, &point);                                      // get raw coordinates
        g_touch_min = g_touch_max = lv_point_to_percent_coordinates(point);     // set min and max to same
    }


    static void touch_end_cb(lv_event_t* e) {

        constexpr i8 MAX_Y_START = 5;
        constexpr i8 MIN_Y_HEIGHT = 12;
        constexpr i8 MAX_Y_HEIGHT = 40;
        constexpr i8 MAX_X_WIDTH = 40;

        const auto path_height = g_touch_max.y - g_touch_min.y;
        const auto total_path_width = abs(g_touch_min.x - g_touch_max.x);
        if (total_path_width < MAX_X_WIDTH                                      // not to wide
            && (path_height > MIN_Y_HEIGHT && path_height < MAX_Y_HEIGHT)       // total path downwards
            && (g_touch_min.y <= MAX_Y_START)) {                                // path starts in top 10 percent
            
            // gesture detected to open the screen selection overlay
            ESP_LOGI(TAG, "Should open screen selection overlay");
            show_overlay();
        }
        ESP_LOGI(TAG, "Path height: %d, Path width: %d, Y-start: %d", path_height, total_path_width, g_touch_min.y);

        g_touch_min = {-1, -1};                             // reset buffers
        g_touch_max = {-1, -1};
    }


    static void touch_cb(lv_event_t* e) {

        lv_indev_t* indev = lv_indev_get_act();
        if (!indev)
            return;

        lv_point_t point;
        lv_indev_get_point(indev, &point);          // get raw coordinates

        const auto current = lv_point_to_percent_coordinates(point);

        if (current.x < g_touch_min.x)      g_touch_min.x = current.x;
        if (current.y < g_touch_min.y)      g_touch_min.y = current.y;
        if (current.x > g_touch_max.x)      g_touch_max.x = current.x;
        if (current.y > g_touch_max.y)      g_touch_max.y = current.y;
    }


    static vec_2d lv_point_to_percent_coordinates(const lv_point_t point) {

        // Get display size
        lv_disp_t* disp = lv_disp_get_default();
        if (!disp) 
            return {-1, -1};
        lv_coord_t hor_res = lv_disp_get_hor_res(disp);
        lv_coord_t ver_res = lv_disp_get_ver_res(disp);

        return vec_2d{
            static_cast<i8>((point.x * 100) / hor_res), 
            static_cast<i8>((point.y * 100) / ver_res) 
        };
    }


    static void create_overlay_if_needed() {

        if (overlay_cont) 
            return;

        // Full‑screen container with dimmed background
        overlay_cont = lv_obj_create(lv_layer_top());
        lv_obj_set_size(overlay_cont, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(overlay_cont, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(overlay_cont, LV_OPA_60, 0);
        lv_obj_clear_flag(overlay_cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(overlay_cont, LV_OBJ_FLAG_CLICKABLE);

        // Close when tapping outside the panel
        lv_obj_add_event_cb(overlay_cont, [](lv_event_t* e) {
            if (lv_event_get_target(e) == overlay_cont) {
                hide_overlay();
            }
        }, LV_EVENT_CLICKED, nullptr);

        // Panel – full width, top‑aligned, rounded bottom corners
        lv_obj_t* panel = lv_obj_create(overlay_cont);
        lv_obj_set_size(panel, LV_PCT(100), LV_PCT(55));   // 55% of screen height
        lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_radius(panel, 20, 0);             // rounded all corners
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(panel, 0, 0);
        lv_obj_set_style_shadow_width(panel, 12, 0);       // subtle shadow
        lv_obj_set_style_shadow_color(panel, lv_color_hex(0x000000), 0);
        lv_obj_set_style_shadow_opa(panel, LV_OPA_40, 0);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

        // Title – use highlight_color
        lv_obj_t* title = lv_label_create(panel);
        lv_label_set_text(title, "Select Screen");
        lv_obj_set_style_text_color(title, highlight_color, 0);
        lv_obj_set_style_text_font(title, &inconsolata_regular_26, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

        // List container – fills remaining space
        overlay_list = lv_list_create(panel);
        lv_obj_set_size(overlay_list, LV_PCT(90), LV_PCT(70)); // adjust to fit
        lv_obj_align(overlay_list, LV_ALIGN_CENTER, 0, 10);
        lv_obj_set_style_bg_color(overlay_list, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(overlay_list, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(overlay_list, 0, 0);
        lv_obj_set_style_pad_row(overlay_list, 4, 0);

        // Start hidden
        lv_obj_add_flag(overlay_cont, LV_OBJ_FLAG_HIDDEN);
    }


    static void show_overlay() {

        if (!overlay_cont)
            create_overlay_if_needed();

        // Clear and repopulate
        lv_obj_clean(overlay_list);

        for (const auto& pair : g_ordered_screens) {
            const std::string& name = pair.first;
            lv_obj_t* btn = lv_list_add_btn(overlay_list, nullptr, name.c_str());

            // ----- Modern button styling -----
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x000000), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
            lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(btn, &inconsolata_regular_26, 0);
            lv_obj_set_style_border_width(btn, 0, 0);
            lv_obj_set_style_pad_left(btn, 20, 0);
            lv_obj_set_style_pad_top(btn, 12, 0);
            lv_obj_set_style_pad_bottom(btn, 12, 0);

            // Bottom separator in support_color
            lv_obj_set_style_border_width(btn, 1, 0);
            lv_obj_set_style_border_color(btn, support_color, 0);
            lv_obj_set_style_border_opa(btn, LV_OPA_60, 0);
            lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_BOTTOM, 0);

            // Press feedback
            static lv_style_t press_style;
            lv_style_init(&press_style);
            lv_style_set_bg_color(&press_style, highlight_color);
            lv_style_set_bg_opa(&press_style, LV_OPA_30);
            lv_style_set_text_color(&press_style, highlight_color);
            lv_obj_add_style(btn, &press_style, LV_STATE_PRESSED);

            // Click event
            lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                lv_obj_t* btn = lv_event_get_target(e);
                lv_obj_t* list = lv_obj_get_parent(btn);
                const char* name = lv_list_get_btn_text(list, btn);
                if (name) {
                    APP::UI::screen_manager::switch_to(std::string(name));
                    hide_overlay();
                }
            }, LV_EVENT_CLICKED, nullptr);
        }

        // Show overlay with a slide‑down animation (optional)
        lv_obj_clear_flag(overlay_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(overlay_cont);

        // Optional animation: slide from top
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, overlay_cont);
        lv_anim_set_exec_cb(&a, [](void* var, int32_t val) {
            lv_obj_set_y((lv_obj_t*)var, val);
        });
        lv_anim_set_values(&a, -lv_obj_get_height(overlay_cont), 0);
        lv_anim_set_time(&a, 300);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_start(&a);

        overlay_shown = true;
    }

    
    static void hide_overlay() {

        if (overlay_cont) {
            lv_obj_add_flag(overlay_cont, LV_OBJ_FLAG_HIDDEN);
            overlay_shown = false;
        }
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

                lv_obj_remove_event_cb(old_root, touch_cb);
                lv_obj_remove_event_cb(old_root, touch_begin_cb);
                lv_obj_remove_event_cb(old_root, touch_end_cb);
            }
            g_current->hide();
        }

        g_current = g_ordered_screens[idx].second.get();
        g_current_name = name;
        g_current->show();

        lv_obj_t* root = g_current->get_root();
        if (root) {
            
            lv_scr_load(root);
            lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);       // Enable gesture detection on the root screen
            lv_obj_add_flag(root, LV_OBJ_FLAG_GESTURE_BUBBLE);
            lv_obj_add_event_cb(root, touch_cb,         LV_EVENT_PRESSING,  nullptr);
            lv_obj_add_event_cb(root, touch_begin_cb,   LV_EVENT_PRESSED,   nullptr);
            lv_obj_add_event_cb(root, touch_end_cb,     LV_EVENT_RELEASED,  nullptr);
        } else
            ESP_LOGE(TAG, "Screen '%s' has null root", name.c_str());

        ESP_LOGI(TAG, "Switched to screen '%s'", name.c_str());
    }


    screen* get_current() { return g_current; }


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
