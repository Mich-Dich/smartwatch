


# Commands

## Check for available ports
```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

## Export needed commands
```bash
source ~/esp/esp-idf/export.sh
```

## Set Target
```bash
idf.py set-target esp32s3
```

## Build & Flash
```bash
clear; idf.py build && idf.py flash monitor
```










## Font creation
- URL: https://lvgl.io/tools/fontconverter
  - Name format: <font-name>-<font-type (regular, ...)>-<height>    
    - example: inconsolata_regular_64
  - Bpp: 2 bit-per-pixel
  - Output format: C file
  - Range: 0x20-0x7F, 0xB0, 0x2022
    (For Icons: 0xF000-0xF8FF)







# TODO: 
- save brightness to non-volatile memory (for rebooting/powerless/...)
- add Bluetooth/Wifi power-saving functions (Need to figure out how to ensure bluetooth remains active if music screen running)
- detect Powering by cable (reenable screen)















I use the home_assist_screen in my screen_manager. That screen_manager contains some logic to check if ANY touch event was triggered. I use that to keep the display active when any touch happens but for some reason It never triggers when im interacting with some LV elements.
When Im scrolling or when im pressing some buttons it bypasses the callback in the screen_manager that keep the display on!!
I NEED to fix that!

I want to combine the 3 callbacks and create a timer that checks every 100ms for any touch!
    static void touch_begin_cb(lv_event_t* e);
    static void touch_end_cb(lv_event_t* e);
    static void touch_cb(lv_event_t* e);









I want to create a new timer:
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

    // timer to check for touch activity (every 50 ms)
    esp_timer_create_args_t activity_args = {
        .callback = interaction_timer_cb,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "interaction_timer",
        .skip_unhandled_events = false
    };
    ESP_ERROR_CHECK(esp_timer_create(&activity_args, &s_interaction_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_interaction_timer, 100 * 1000));        // 100 ms
}





Let's combine them into this:
static void interaction_timer_cb() {

    wake_system_event();
    rearm_sleep_system();

    switch (lv_event_get_code(e)) {
        
        case LV_EVENT_PRESSED: {

            // TODO: implement
            break;
        }

        case LV_EVENT_PRESSING: {

            // TODO: implement
            break;
        }

        case LV_EVENT_RELEASED: {

            // TODO: implement
            break;
        }
    }
}






I also already cleared theswitch_to functionm:
void switch_to(const std::string& name) {

    int idx = find_index(name);
    if (idx < 0) {
        ESP_LOGE(TAG, "Screen '%s' not found", name.c_str());
        return;
    }

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




