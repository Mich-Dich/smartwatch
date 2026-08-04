#include "util/pch.hpp"
#include "screen_manager.hpp"

#include "UI/util.hpp"



#define USE_ESP_SLEEP_MODE          1

// FORWARD DECLARATIONS ================================================================================================

extern "C" {
    extern void setBrightens(uint8_t brig);

    extern uint8_t getBrightens();
}

namespace APP::UI::screen_manager {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    constexpr u32                                                           DIM_TIMER_DURATION = 5 * 1000 * 1000;       // 5 seconds
    
    #if USE_ESP_SLEEP_MODE
        
        constexpr u32                                                       SLEEP_TIMER_DURATION = 3 * 1000 * 1000;     // 3 seconds after dim

    #endif

    // MACROS ==========================================================================================================

    #if USE_ESP_SLEEP_MODE

        #define TOUCH_INT_GPIO                                              GPIO_NUM_27       // e.g., GPIO 27

        #define TOUCH_INT_ACTIVE_LOW                                        1               // 1 if active low (0 = active high)

    #endif

    // STATIC VARIABLES ================================================================================================

    static std::vector<std::pair<std::string, std::unique_ptr<screen>>>     g_ordered_screens{};

    static screen*                                                          g_current = nullptr;

    static std::string                                                      g_current_name;

    static util::touch_movement_data                                        s_touch_movement{};

    static lv_obj_t*                                                        overlay_cont = nullptr;

    static lv_obj_t*                                                        overlay_list = nullptr;

    static bool                                                             overlay_shown = false;

    static esp_timer_handle_t                                               s_dim_timer = nullptr;

    static bool                                                             s_is_dimmed = false;

    #if USE_ESP_SLEEP_MODE

        static esp_timer_handle_t                                           s_sleep_timer = nullptr;
    
        static bool                                                         s_is_sleeping = false;

    #endif

    static uint8_t                                                          s_previous_brightness = 255;

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    static int find_index(const std::string& name);

    static void touch_begin_cb(lv_event_t* e);

    static void touch_end_cb(lv_event_t* e);

    static void touch_cb(lv_event_t* e);

    static util::vec_2d lv_point_to_percent_coordinates(const lv_point_t point);

    static void create_overlay_if_needed();
    
    static void show_overlay();

    static void hide_overlay();

    static void dim_timer_cb(void* arg);

    static void stop_dim_timer();

    static void start_dim_timer();

    #if USE_ESP_SLEEP_MODE
    
        static void sleep_timer_cb(void* arg);

        static void stop_sleep_timer();

        static void start_sleep_timer();

        static void enter_low_power_mode();

        static void exit_low_power_mode();

    #endif

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    static int find_index(const std::string& name) {

        for (size_t i = 0; i < g_ordered_screens.size(); ++i)
            if (g_ordered_screens[i].first == name)
                return static_cast<int>(i);
        
        return -1;
    }


    static void touch_begin_cb(lv_event_t* e) {

        #if USE_ESP_SLEEP_MODE

            if (s_is_sleeping)                                                      // If we were sleeping, wake up first
                exit_low_power_mode();                                              // The timer will be restarted later

        #endif

        if (s_is_dimmed) {                                                      // Restore brightness if dimmed

            setBrightens(s_previous_brightness);
            s_is_dimmed = false;
            ESP_LOGI(TAG, "Brightness restored to %d", s_previous_brightness);
        }

        stop_dim_timer();

        #if USE_ESP_SLEEP_MODE

            stop_sleep_timer();

        #endif
        
        lv_indev_t* indev = lv_indev_get_act();
        if (!indev) 
            return;

        lv_point_t point;
        lv_indev_get_point(indev, &point);                                      // get raw coordinates
        s_touch_movement = {lv_point_to_percent_coordinates(point)};
    }


    static void touch_end_cb(lv_event_t* e) {

        lv_indev_t* indev = lv_indev_get_act();
        if (!indev)
            return;

        lv_point_t point;
        lv_indev_get_point(indev, &point);
        s_touch_movement.touch_stop = lv_point_to_percent_coordinates(point);


        const util::swipe_direction dir = get_swipe_direction(s_touch_movement);
        switch (dir) {
            case util::swipe_direction::down:           ESP_LOGI(TAG, "Swipe Down → open overlay"); show_overlay(); break;
            case util::swipe_direction::up:             ESP_LOGI(TAG, "Swipe Up → maybe go back"); break;
            case util::swipe_direction::right:          ESP_LOGI(TAG, "Swipe Right → next item"); break;
            case util::swipe_direction::left:           ESP_LOGI(TAG, "Swipe Left → previous item"); break;
            case util::swipe_direction::down_right:     ESP_LOGI(TAG, "Diagonal Down-Right → something"); break;
            default:                                    break;
        }

        #if USE_ESP_SLEEP_MODE

            if (!s_is_sleeping) start_dim_timer();
        
        #else

            start_dim_timer();

        #endif

        s_touch_movement = {};
    }


    static void touch_cb(lv_event_t* e) {

        lv_indev_t* indev = lv_indev_get_act();
        if (!indev)
            return;

        lv_point_t point;
        lv_indev_get_point(indev, &point);          // get raw coordinates

        const auto current = lv_point_to_percent_coordinates(point);

        s_touch_movement.update_min(current);
        s_touch_movement.update_max(current);
        s_touch_movement.update_size();
    }


    static util::vec_2d lv_point_to_percent_coordinates(const lv_point_t point) {

        // Get display size
        lv_disp_t* disp = lv_disp_get_default();
        if (!disp) 
            return {-1, -1};
        lv_coord_t hor_res = lv_disp_get_hor_res(disp);
        lv_coord_t ver_res = lv_disp_get_ver_res(disp);

        return util::vec_2d{
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

            // Modern button styling
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
    

    static void dim_timer_cb(void* arg) {

        s_previous_brightness = getBrightens();                                 // Store current brightness before dimming
        setBrightens(51);                                                       // 20% of 255
        s_is_dimmed = true;
        hide_overlay();
        ESP_LOGI(TAG, "Display dimmed to 20%% brightness");

        #if USE_ESP_SLEEP_MODE

            start_sleep_timer();                                                    // Now start the sleep timer (3 seconds later)
        
        #endif
    }

    
    static void stop_dim_timer() {

        if (s_dim_timer)
            esp_timer_stop(s_dim_timer);
    }


    static void start_dim_timer() {

        #if USE_ESP_SLEEP_MODE

            if (s_is_sleeping) 
                return;                                                             // If we are sleeping, don't start the dim timer (we'll start it after wake)

        #endif
        
        if (s_dim_timer == nullptr) {
            esp_timer_create_args_t args = {
                .callback = dim_timer_cb,
                .arg = nullptr,
                .dispatch_method = ESP_TIMER_TASK,
                .name = "dim_timer",
                .skip_unhandled_events = false
            };
            ESP_ERROR_CHECK(esp_timer_create(&args, &s_dim_timer));
        }
        ESP_ERROR_CHECK(esp_timer_start_once(s_dim_timer, DIM_TIMER_DURATION));
    }


    #if USE_ESP_SLEEP_MODE
            
        static void sleep_timer_cb(void* arg) {

            ESP_LOGI(TAG, "Entering low-power mode");
            enter_low_power_mode();
        }


        static void stop_sleep_timer() {

            if (s_sleep_timer)
                esp_timer_stop(s_sleep_timer);
        }

        static void start_sleep_timer() {

            if (s_sleep_timer == nullptr) {
                esp_timer_create_args_t args = {
                    .callback = sleep_timer_cb,
                    .arg = nullptr,
                    .dispatch_method = ESP_TIMER_TASK,
                    .name = "sleep_timer",
                    .skip_unhandled_events = false
                };
                ESP_ERROR_CHECK(esp_timer_create(&args, &s_sleep_timer));
            }
            ESP_ERROR_CHECK(esp_timer_start_once(s_sleep_timer, SLEEP_TIMER_DURATION));
        }


        static void enter_low_power_mode() {

            // Turn off the display (optional – may be handled by setBrightens(0))
            setBrightens(0);

            // // Configure the touch interrupt GPIO for wake-up
            // gpio_config_t io_conf = {
            //     .pin_bit_mask = (1ULL << TOUCH_INT_GPIO),
            //     .mode = GPIO_MODE_INPUT,
            //     .pull_up_en = GPIO_PULLUP_ENABLE,    // Most touch interrupts are open-drain, so pull-up is needed
            //     .pull_down_en = GPIO_PULLDOWN_DISABLE,
            //     .intr_type = GPIO_INTR_DISABLE       // We only use it for wake, not for normal interrupts
            // };
            // gpio_config(&io_conf);

            // // Enable wake-up on the GPIO level (choose the level that triggers the interrupt)
            // // If active low, wake when pin is LOW (0)
            // // If active high, wake when pin is HIGH (1)
            // #if TOUCH_INT_ACTIVE_LOW
            //     esp_sleep_enable_ext0_wakeup(TOUCH_INT_GPIO, 0);   // wake on LOW
            // #else
            //     esp_sleep_enable_ext0_wakeup(TOUCH_INT_GPIO, 1);   // wake on HIGH
            // #endif

            // // Optionally, keep RTC peripherals powered if needed
            // // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);

            // ESP_LOGI(TAG, "Entering light sleep, wake on GPIO %d", TOUCH_INT_GPIO);
            // s_is_sleeping = true;

            // // Enter light sleep
            // esp_light_sleep_start();

            // // ---- Execution resumes here after wake-up ----
            // // The touch interrupt has woken us, but the interrupt line might still be asserted.
            // // We must clear it by reading the touch data (done in exit_low_power_mode).
        }

        static void exit_low_power_mode() {

            // Re-enable the display and restore previous brightness
            setBrightens(s_previous_brightness);
            s_is_dimmed = false;
            s_is_sleeping = false;

            // // Clear the touch interrupt by reading the touch status.
            // // This is critical: the touch controller keeps the INT pin low until we read the touch data.
            // // If you are using LVGL, call its touch driver read function.
            // // Example (if you have a function like touch_driver_read()):
            // // touch_driver_read();
            // // Or call the LVGL input device's read_cb:
            // // lv_indev_data_t data;
            // // lv_indev_get_drv(lv_indev_get_next(NULL))->read_cb(NULL, &data);
            // // Since we don't have the exact driver, we'll just log a warning.
            // ESP_LOGW(TAG, "Please clear the touch interrupt by reading touch data");

            // // If the touch driver uses I2C/SPI, you may need to reinitialize it because
            // // the bus may have been powered down. Check your driver's reinit function.

            // ESP_LOGI(TAG, "Exited low-power mode, brightness restored to %d", s_previous_brightness);
        }

    #endif

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
            
            stop_dim_timer();
            
            #if USE_ESP_SLEEP_MODE
                
                stop_sleep_timer();

            #endif
            
            start_dim_timer();
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
