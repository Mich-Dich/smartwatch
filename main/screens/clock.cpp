
#include "util/pch.hpp"
#include "clock.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI {

    // TYPES ===========================================================================================================

    struct clock_labels {

        lv_obj_t* time_label;
        lv_obj_t* date_label;
    };

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    static void update_clock_cb(lv_timer_t* timer);

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

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

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    void clock_screen::create(lv_obj_t* screen) {

        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
        APP::display::draw_background_pattern_arcs_markers_hexagon(screen);

        lv_obj_t* text_container = lv_obj_create(screen);
        lv_obj_set_size(text_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(text_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(text_container, 0, 0);
        lv_obj_set_flex_flow(text_container, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(text_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_center(text_container);

        lv_obj_t* time_label = lv_label_create(text_container);
        lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(time_label, &inconsolata_regular_64, 0);
        lv_label_set_text(time_label, "--:--:--");

        lv_obj_t* date_label = lv_label_create(text_container);
        lv_obj_set_style_text_color(date_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(date_label, &inconsolata_regular_48, 0);
        lv_label_set_text(date_label, "----/--/--");

        static clock_labels labels = { time_label, date_label };
        lv_timer_create(update_clock_cb, 1000, &labels);
    }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
