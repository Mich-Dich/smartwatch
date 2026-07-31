
#include "util/pch.hpp"

#include "util/wifi.hpp"
#include "util/RTC.hpp"
#include "UI/manager.hpp"
#include "screens/clock.hpp"
#include "screens/music.hpp"
#include "screens/menu.hpp"
#include "screens/stopwatch.hpp"
#include "screens/spectrum_analyzer.hpp"
#include "screens/settings.hpp"



// FORWARD DECLARATIONS ================================================================================================

namespace APP {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================
    
    esp_lcd_panel_handle_t       lcd_panel = nullptr;

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    static void on_time_synced(time_t current_time);

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================
    
    static void on_time_synced(time_t current_time) {

        struct tm timeinfo;
        localtime_r(&current_time, &timeinfo);
        VALIDATE(RTC::set_time(&timeinfo), , "RTC", "RTC updated from SNTP", "Failed to write RTC");
    }


    extern "C" void app_main() {

        ESP_LOGI(TAG, "Starting interactive smartwatch");

        wifi::init();                                       // Wi‑Fi / time sync
        setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
        tzset();
        wifi::set_credentials({
            {"Happy",                   "Kerstin321!"},
            {"FRITZ!Repeater 3000",     "frosh#5"}
        });

        // Display
        lv_display_t* disp = bsp_display_start();           // Get the underlying LCD panel handle
        if (disp) {

            bsp_display_backlight_on();
            APP::UI::screen_manager::init_dim_timer();      // start auto‑dim
        }
        lcd_panel = static_cast<esp_lcd_panel_handle_t>(lv_display_get_driver_data(disp));

        // RTC
        if (RTC::init()) {
            struct tm rtc_time;
            if (RTC::get_time(&rtc_time)) {
                time_t t = mktime(&rtc_time);
                struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
                settimeofday(&tv, NULL);
                ESP_LOGI("RTC", "System clock set from RTC");
            }
        } else
            ESP_LOGE("RTC", "RTC init failed");

        wifi::set_time_synced_callback(on_time_synced);     // Register callback to update RTC after every successful sync

        // Mount SD-Card
        esp_err_t ret = bsp_sdcard_mount();
        VALIDATE(ret == ESP_OK, , "SD", "SD card mounted via BSP", "BSP SD mount failed: %s", esp_err_to_name(ret));

        bsp_display_lock(-1);                               // Lock display and create the screens
        UI::screen_manager::add_screen(new UI::clock_screen, UI::screen::type::home);
        UI::screen_manager::add_screen(new UI::music_screen);
        UI::screen_manager::add_screen(new UI::spectrum_analyzer_screen);
        UI::screen_manager::add_screen(new UI::stopwatch_screen);
        UI::screen_manager::add_screen(new UI::settings_screen);
        UI::screen_manager::add_screen(new UI::menu_screen, UI::screen::type::menu);
        bsp_display_unlock();

        ESP_LOGI(TAG, "running");
    }

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
