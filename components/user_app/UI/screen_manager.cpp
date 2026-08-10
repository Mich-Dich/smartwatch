#include "util/pch.hpp"
#include "screen_manager.hpp"

#include "user_app.hpp"
#include "util/system.hpp"
#include "UI/util.hpp"



#define USE_ESP_SLEEP_MODE                                                  1

// FORWARD DECLARATIONS ================================================================================================

extern "C" {

    extern void setBrightens(uint8_t brig);

    extern uint8_t getBrightens();

}

namespace APP::UI::screen_manager {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // @brief Time (µs) after last interaction before dimming the display.
    constexpr u32                                                           DIM_TIMER_DURATION = 9 * 1000 * 1000;

    #if USE_ESP_SLEEP_MODE

        // @brief Time (µs) after dimming before entering low‑power sleep.
        constexpr u32                                                       SLEEP_TIMER_DURATION = 3 * 1000 * 1000;     // 3 seconds after dim

    #endif

    // MACROS ==========================================================================================================

    #if USE_ESP_SLEEP_MODE

        #define TOUCH_INT_GPIO                                              GPIO_NUM_5

        #define TOUCH_INT_ACTIVE_LOW                                        1               // 1 if active low (0 = active high)

    #endif

    // STATIC VARIABLES ================================================================================================

    // @brief List of registered screens (name + unique_ptr).
    static std::vector<std::pair<std::string, std::unique_ptr<screen>>>     s_ordered_screens{};

    // @brief Pointer to the currently active screen.
    static screen*                                                          s_current = nullptr;

    // @brief Name of the currently active screen.
    static std::string                                                      s_current_name{};

    // @brief Overlay container (full‑screen background) and its list child.
    static lv_obj_t*                                                        s_overlay_cont = nullptr;

    static lv_obj_t*                                                        s_overlay_list = nullptr;

    static bool                                                             s_overlay_shown = false;

    // @brief Timers for dimming, sleep, and battery monitoring.
    static esp_timer_handle_t                                               s_dim_timer = nullptr;

    static esp_timer_handle_t                                               s_full_second_timer = nullptr;

    static bool                                                             s_is_dimmed = false;

    #if USE_ESP_SLEEP_MODE

        static esp_timer_handle_t                                           s_sleep_timer = nullptr;

        static bool                                                         s_is_sleeping = false;

    #endif

    // @brief Previous brightness value (to restore after dimming).
    static uint8_t                                                          s_previous_brightness = 255;

    // @brief Lists of callbacks for sleep and wake events.
    static std::vector<callback_func>                                       s_sleep_callbacks{};

    static std::vector<callback_func>                                       s_wake_callbacks{};

    // @brief LVGL timer for touch polling (100 ms) and associated touch state.
    static lv_timer_t*                                                      s_interaction_lv_timer = nullptr;

    static bool                                                             s_touch_was_pressed = false;

    static lv_point_t                                                       s_touch_start = {0, 0};

    static lv_point_t                                                       s_touch_last = {0, 0};

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    // @brief Find the index of a screen by name.
    // @param name  Screen name.
    // @return      Index in s_ordered_screens, or -1 if not found.
    static int find_index(const std::string& name);

    // @brief Wake the system: restore brightness, stop dim/sleep timers.
    static void wake_system_event();

    // @brief Re‑arm the dim timer (and optionally sleep timer) after user activity.
    static void rearm_sleep_system();

    // @brief Timer callback for battery voltage monitoring (every 1 second).
    static void full_second_cb(void* arg);

    // @brief LVGL timer callback for touch polling. Reads the touch state, wakes the system on press, and detects down‑swipe.
    static void interaction_timer_cb(lv_timer_t* timer);

    // @brief Create the overlay UI objects (if not already created).
    static void create_overlay_if_needed();

    // @brief Show the overlay with a slide‑down animation and populate the screen list.
    static void show_overlay();

    // @brief Hide the overlay.
    static void hide_overlay();

    // @brief Timer callback for display dimming (after inactivity).
    static void dim_timer_cb(void* arg);

    // @brief Stop the dim timer.
    static void stop_dim_timer();

    // @brief Start the dim timer with the configured duration.
    static void start_dim_timer();

    #if USE_ESP_SLEEP_MODE

        // @brief Timer callback for entering low‑power sleep.
        static void sleep_timer_cb(void* arg);

        // @brief Stop the sleep timer.
        static void stop_sleep_timer();

        // @brief Start the sleep timer (after dimming).
        static void start_sleep_timer();

        // @brief Enter low‑power mode (turn off backlight, execute sleep callbacks).
        static void enter_low_power_mode();

        // @brief Exit low‑power mode (restore brightness, execute wake callbacks).
        static void exit_low_power_mode();

    #endif

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    static int find_index(const std::string& name) {

        for (size_t i = 0; i < s_ordered_screens.size(); ++i)
            if (s_ordered_screens[i].first == name)
                return static_cast<int>(i);

        return -1;
    }


    static void wake_system_event() {
#if USE_ESP_SLEEP_MODE
        if (s_is_sleeping)
            exit_low_power_mode();
#endif
        if (s_is_dimmed) {
            setBrightens(s_previous_brightness);
            s_is_dimmed = false;
            ESP_LOGI(TAG, "Brightness restored to %d", s_previous_brightness);
        }
        stop_dim_timer();
#if USE_ESP_SLEEP_MODE
        stop_sleep_timer();
#endif
    }


    static void rearm_sleep_system() {
#if USE_ESP_SLEEP_MODE
        if (!s_is_sleeping)
            start_dim_timer();
#else
        start_dim_timer();
#endif
    }


    static void full_second_cb(void* arg) {
        constexpr f32 CONNECTED_POWER_DIFFERENCE = 0.30f;
        static f32 previous_voltage = 0;

        f32 adjusted{}, raw_voltage{};
        APP::system::get_battery_voltage(adjusted, raw_voltage);

        if (raw_voltage > (previous_voltage + CONNECTED_POWER_DIFFERENCE)) {
            wake_system_event();
            rearm_sleep_system();
            APP::system::set_charger_connected(true);
        }
        if (raw_voltage < (previous_voltage - CONNECTED_POWER_DIFFERENCE)) {
            wake_system_event();
            rearm_sleep_system();
            APP::system::set_charger_connected(false);
        }
        previous_voltage = raw_voltage;
    }


    static void interaction_timer_cb(lv_timer_t* timer) {

        lv_indev_t* indev = lv_indev_get_next(NULL);
        if (!indev)
            return;

        lv_indev_data_t data;
        if (indev->driver && indev->driver->read_cb)
            indev->driver->read_cb(indev->driver, &data);
        else {
            ESP_LOGI(TAG, "Cant read");
            return;
        }

        if (data.state == LV_INDEV_STATE_PRESSED) {

            // ESP_LOGI(TAG, "STATE_PRESSED");
            wake_system_event();
            rearm_sleep_system();

            if (!s_touch_was_pressed) {
                s_touch_was_pressed = true;
                s_touch_start = data.point;
            }
            s_touch_last = data.point;   // store last known point

        } else {

            // ESP_LOGI(TAG, "TOUCH FINISHED");
            // Touch released – detect swipe using custom gesture logic
            if (s_touch_was_pressed) {
                // Convert raw coordinates to percent
                util::vec_2d start_percent = APP::UI::util::lv_point_to_percent(s_touch_start);
                util::vec_2d stop_percent  = APP::UI::util::lv_point_to_percent(s_touch_last);

                // Build touch_movement_data (min/max set from start/stop)
                util::touch_movement_data gesture_data(start_percent);
                gesture_data.touch_stop = stop_percent;
                gesture_data.touch_min = {
                    std::min(start_percent.x, stop_percent.x),
                    std::min(start_percent.y, stop_percent.y)
                };
                gesture_data.touch_max = {
                    std::max(start_percent.x, stop_percent.x),
                    std::max(start_percent.y, stop_percent.y)
                };
                gesture_data.touch_size = gesture_data.touch_max - gesture_data.touch_min;

                // Use your custom gesture detector
                if (util::get_swipe_direction(gesture_data) == util::swipe_direction::down)
                    show_overlay();   // Safe: called from LVGL context

                s_touch_was_pressed = false;
            }
        }
    }


    static void create_overlay_if_needed() {

        if (s_overlay_cont)
            return;

        // Full‑screen dimmed backdrop – tap outside closes overlay
        s_overlay_cont = lv_obj_create(lv_layer_top());
        lv_obj_set_size(s_overlay_cont, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(s_overlay_cont, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(s_overlay_cont, LV_OPA_60, 0);
        lv_obj_clear_flag(s_overlay_cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(s_overlay_cont, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_border_width(s_overlay_cont, 0, 0);
        lv_obj_set_style_shadow_width(s_overlay_cont, 0, 0);            // no shadow for full‑width look

        // Close when tapping outside the panel
        lv_obj_add_event_cb(s_overlay_cont, [](lv_event_t* e) {
            if (lv_event_get_target(e) == s_overlay_cont) {
                hide_overlay();
            }
        }, LV_EVENT_CLICKED, nullptr);

        // ---- Panel – full width, top‑aligned, 60% height ----
        lv_obj_t* panel = lv_obj_create(s_overlay_cont);
        lv_obj_set_size(panel, LV_PCT(100), LV_PCT(60));
        lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_radius(panel, 0, 0);                           // no rounded corners
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x0A0A0A), 0);    // dark base
        lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(panel, 0, 0);
        lv_obj_set_style_shadow_width(panel, 0, 0);                     // no shadow for full‑width look
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

        // ---- List container (fills the panel) ----
        s_overlay_list = lv_obj_create(panel);
        lv_obj_set_size(s_overlay_list, LV_PCT(100), LV_PCT(100));
        lv_obj_align(s_overlay_list, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_style_bg_color(s_overlay_list, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(s_overlay_list, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(s_overlay_list, 0, 0);
        lv_obj_set_style_pad_all(s_overlay_list, 0, 0);
        lv_obj_set_flex_flow(s_overlay_list, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(s_overlay_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_scrollbar_mode(s_overlay_list, LV_SCROLLBAR_MODE_OFF);
        lv_obj_add_flag(s_overlay_list, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_add_flag(s_overlay_cont, LV_OBJ_FLAG_HIDDEN);
    }


    static void show_overlay() {

        if (!s_overlay_cont)
            create_overlay_if_needed();

        // Clear previous items
        lv_obj_clean(s_overlay_list);

        for (const auto& pair : s_ordered_screens) {
            const std::string& name = pair.first;
            screen* scr = pair.second.get();

            // ---- Row container ----
            lv_obj_t* row = lv_obj_create(s_overlay_list);
            lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_color(row, lv_color_hex(0x000000), 0);
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(row, 0, 0);
            lv_obj_set_style_pad_top(row, 12, 0);
            lv_obj_set_style_pad_bottom(row, 12, 0);
            lv_obj_set_style_pad_left(row, 20, 0);
            lv_obj_set_style_pad_right(row, 20, 0);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            // Bottom separator using support_color
            lv_obj_set_style_border_width(row, 1, 0);
            lv_obj_set_style_border_color(row, support_color, 0);
            lv_obj_set_style_border_opa(row, LV_OPA_50, 0);
            lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);

            // ---- Screen name (label) ----
            lv_obj_t* label = lv_label_create(row);
            lv_label_set_text(label, name.c_str());
            lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(label, &inconsolata_regular_26, 0);

            // ---- Optional toggle ----
            if (scr->display_toggle_in_manager()) {

                lv_obj_t* sw = lv_switch_create(row);
                lv_obj_set_size(sw, 50, 28);
                lv_obj_set_style_bg_color(sw, highlight_color, LV_STATE_CHECKED);

                if (scr->get_toggle_state())
                    lv_obj_add_state(sw, LV_STATE_CHECKED);
                else
                    lv_obj_clear_state(sw, LV_STATE_CHECKED);

                lv_obj_add_event_cb(sw, [](lv_event_t* e) {

                    lv_obj_t* sw = lv_event_get_target(e);
                    screen* scr = static_cast<screen*>(lv_event_get_user_data(e));
                    if (scr) {
                        bool new_state = (lv_obj_get_state(sw) & LV_STATE_CHECKED) != 0;
                        scr->toggle_change_from_manager(new_state);
                    }
                }, LV_EVENT_VALUE_CHANGED, scr);
            }

            // ---- Click on row to switch screen ----
            // Highlight on press using highlight_color
            lv_obj_add_event_cb(row, [](lv_event_t* e) {
                if (lv_event_get_code(e) == LV_EVENT_PRESSED) {

                    lv_obj_set_style_bg_color(lv_event_get_target(e), highlight_color, 0);
                    lv_obj_set_style_bg_opa(lv_event_get_target(e), LV_OPA_30, 0);

                } else if (lv_event_get_code(e) == LV_EVENT_RELEASED || lv_event_get_code(e) == LV_EVENT_CLICKED) {

                    lv_obj_set_style_bg_color(lv_event_get_target(e), lv_color_hex(0x000000), 0);
                    lv_obj_set_style_bg_opa(lv_event_get_target(e), LV_OPA_TRANSP, 0);
                }
            }, LV_EVENT_ALL, nullptr);

            // Switch to screen on click
            lv_obj_add_event_cb(row, [](lv_event_t* e) {
                lv_obj_t* row = lv_event_get_target(e);
                lv_obj_t* label = lv_obj_get_child(row, 0);
                if (label && lv_obj_check_type(label, &lv_label_class)) {

                    const char* name = lv_label_get_text(label);
                    if (name) {
                        APP::UI::screen_manager::switch_to(std::string(name));
                        hide_overlay();
                    }
                }
            }, LV_EVENT_CLICKED, nullptr);
        }

        // Show overlay – slide down from top
        lv_obj_clear_flag(s_overlay_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_overlay_cont);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, s_overlay_cont);
        lv_anim_set_exec_cb(&a, [](void* var, int32_t val) {
            lv_obj_set_y((lv_obj_t*)var, val);
        });
        lv_anim_set_values(&a, -lv_obj_get_height(s_overlay_cont), 0);
        lv_anim_set_time(&a, 300);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_start(&a);

        s_overlay_shown = true;
    }


    static void hide_overlay() {
        if (s_overlay_cont) {
            lv_obj_add_flag(s_overlay_cont, LV_OBJ_FLAG_HIDDEN);
            s_overlay_shown = false;
        }
    }


    static void dim_timer_cb(void* arg) {
        s_previous_brightness = getBrightens();
        setBrightens(51);
        s_is_dimmed = true;
        hide_overlay();
        ESP_LOGI(TAG, "Display dimmed to 20%% brightness");

#if USE_ESP_SLEEP_MODE
        start_sleep_timer();
#endif
    }


    static void stop_dim_timer() {
        if (s_dim_timer)
            esp_timer_stop(s_dim_timer);
    }


    static void start_dim_timer() {
#if USE_ESP_SLEEP_MODE
        if (s_is_sleeping)
            return;
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

            setBrightens(0);
            ESP_LOGI(TAG, "Executing [%zu] registered sleep callbacks", s_sleep_callbacks.size());
            for (auto callback : s_sleep_callbacks)
                if (callback) callback();
            // Light sleep code commented out – enable as needed.
        }


        static void exit_low_power_mode() {

            setBrightens(s_previous_brightness);
            s_is_dimmed = false;
            s_is_sleeping = false;

            ESP_LOGI(TAG, "Executing [%zu] registered wake callbacks", s_wake_callbacks.size());
            for (auto callback : s_wake_callbacks)
                if (callback) callback();

            ESP_LOGI(TAG, "Exited low-power mode, brightness restored to %d", s_previous_brightness);
        }

    #endif

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    void init() {

        const esp_timer_create_args_t args = {
            .callback = full_second_cb,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "full_second",
            .skip_unhandled_events = false
        };
        ESP_ERROR_CHECK(esp_timer_create(&args, &s_full_second_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(s_full_second_timer, 1000 * 1000));    // 1s

        // LVGL timer for touch polling (100 ms)
        s_interaction_lv_timer = lv_timer_create(interaction_timer_cb, 100, nullptr);
        if (s_interaction_lv_timer)
            lv_timer_set_repeat_count(s_interaction_lv_timer, -1); // repeat indefinitely
        else
            ESP_LOGE(TAG, "Failed to create LVGL interaction timer");
    }


    void register_screen(const std::string& name, std::unique_ptr<screen> screen) {

        if (!screen) {
            ESP_LOGE(TAG, "Attempt to register null screen under '%s'", name.c_str());
            return;
        }

        // Check for duplicate names
        if (find_index(name) >= 0) {
            ESP_LOGW(TAG, "Screen '%s' already registered - replacing", name.c_str());
            // Remove the old one
            auto it = std::find_if(s_ordered_screens.begin(), s_ordered_screens.end(),
                [&](const auto& pair) { return pair.first == name; });
            if (it != s_ordered_screens.end()) {
                s_ordered_screens.erase(it);
            }
        }

        screen->init();   // let the screen create its UI
        s_ordered_screens.emplace_back(name, std::move(screen));
        ESP_LOGI(TAG, "Registered screen '%s' (total: %zu)", name.c_str(), s_ordered_screens.size());
    }


    void switch_to(const std::string& name) {

        int idx = find_index(name);
        if (idx < 0) {
            ESP_LOGE(TAG, "Screen '%s' not found", name.c_str());
            return;
        }

        // Remove gesture callback from the current screen (if any)
        if (s_current)
            s_current->hide();

        s_current = s_ordered_screens[idx].second.get();
        s_current_name = name;
        s_current->show();

        lv_obj_t* root = s_current->get_root();
        if (root) {

            lv_scr_load(root);
            lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);       // Enable gesture detection on the root screen
            lv_obj_add_flag(root, LV_OBJ_FLAG_GESTURE_BUBBLE);

            stop_dim_timer();

            #if USE_ESP_SLEEP_MODE

                stop_sleep_timer();

            #endif

            start_dim_timer();
        } else
            ESP_LOGE(TAG, "Screen '%s' has null root", name.c_str());

        ESP_LOGI(TAG, "Switched to screen '%s'", name.c_str());
    }


    screen* get_current() { return s_current; }


    void handle_event(lv_event_t* e) {

        if (s_current)
            s_current->handle_event(e);

        // If no current screen, the event is silently ignored.
    }


    u8 add_sleep_callback(const callback_func callback) {

        s_sleep_callbacks.push_back(callback);
        return static_cast<u8>(s_sleep_callbacks.size() - 1);       // return index
    }


    void remove_sleep_callback(const u8 index) {

		if (index < s_sleep_callbacks.size())
			s_sleep_callbacks[index] = nullptr;
    }


    u8 add_wake_callback(const callback_func callback) {

        s_wake_callbacks.push_back(callback);
        return static_cast<u8>(s_wake_callbacks.size() - 1);       // return  index
    }


    void remove_wake_callback(const u8 index) {

		if (index < s_wake_callbacks.size())
			s_wake_callbacks[index] = nullptr;
    }


    void recreate_screens() {

        for (const auto& [name, screen] : s_ordered_screens)
            screen->recreate_ui();
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
