
#include "util/pch.hpp"

#include "util/wifi.hpp"



// FORWARD DECLARATIONS ================================================================================================

// TYPES ===============================================================================================================

// Data passed to the timer callback
struct clock_labels {

    lv_obj_t*       time_label;
    lv_obj_t*       date_label;
};

// CONSTANTS ===========================================================================================================

// MACROS ==============================================================================================================

// STATIC VARIABLES ====================================================================================================

// INTERNAL TEMPLATE DECLARATION =======================================================================================

// INTERNAL FUNCTION DECLARATION =======================================================================================

// INTERNAL TEMPLATE IMPLEMENTATION ====================================================================================

// INTERNAL FUNCTION IMPLEMENTATION ====================================================================================

// LVGL timer: update both time and date
static void update_clock_cb(lv_timer_t* timer) {

    auto* labels = static_cast<clock_labels*>(lv_timer_get_user_data(timer));
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
    lv_label_set_text(labels->time_label, buf);

    strftime(buf, sizeof(buf), "%Y-%m-%d", &timeinfo);
    lv_label_set_text(labels->date_label, buf);
}


extern "C" void app_main() {
    ESP_LOGI(APP::TAG, "Starting digital clock with background");

    // Initialise Wi‑Fi / time sync
    APP::wifi::init();
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();

    APP::wifi::set_credentials({
        {"Happy",               "Kerstin321!"},
        {"FRITZ!Repeater 3000", "frosh#5"},
        {"Armor 34 Pro",        "hab ich vergessen"}
    });

    // Start display
    lv_display_t* disp = bsp_display_start();
    if (disp) bsp_display_backlight_on();

    bsp_display_lock(-1);
    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

    // Draw the background pattern (behind everything)
    APP::display::draw_background_pattern_arcs_markers_hexagon(screen);

    // Create a container for the two text labels (flex column, centered)
    lv_obj_t* text_container = lv_obj_create(screen);
    lv_obj_set_size(text_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(text_container, LV_OPA_TRANSP, 0);   // transparent background
    lv_obj_set_style_border_width(text_container, 0, 0);
    lv_obj_set_flex_flow(text_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(text_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(text_container);

    // Time label (large font)
    lv_obj_t* time_label = lv_label_create(text_container);
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(time_label, &inconsolata_regular_64, 0);
    lv_label_set_text(time_label, "--:--:--");

    // Date label (smaller font, below time)
    lv_obj_t* date_label = lv_label_create(text_container);
    lv_obj_set_style_text_color(date_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(date_label, &inconsolata_regular_48, 0);
    lv_label_set_text(date_label, "----/--/--");

    // Start the update timer
    static clock_labels labels = { time_label, date_label };
    lv_timer_create(update_clock_cb, 1000, &labels);

    bsp_display_unlock();

    // Time synchronisation
    ESP_LOGI(APP::TAG, "First time sync attempt");
    APP::wifi::sync_time();
    APP::wifi::start_periodic_sync(60 * 60 * 1000);   // every 60 minutes
    ESP_LOGI(APP::TAG, "Clock running");
}

// TEMPLATE IMPLEMENTATION =============================================================================================

// FUNCTION IMPLEMENTATION =============================================================================================

// CLASS IMPLEMENTATION ================================================================================================

// CLASS PUBLIC ========================================================================================================

// CLASS PROTECTED =====================================================================================================

// CLASS PRIVATE =======================================================================================================
