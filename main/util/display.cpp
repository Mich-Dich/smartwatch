
#include "util/pch.hpp"
#include "display.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::display {

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // TYPES ===========================================================================================================

    // STATIC VARIABLES ================================================================================================

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    void fullscreen_canvas(lv_timer_cb_t timer_cb) {

        lv_display_t* disp = lv_display_get_default();
        const i32 w = lv_display_get_horizontal_resolution(disp);
        const i32 h = lv_display_get_vertical_resolution(disp);

        const u32 stride = LV_DRAW_BUF_STRIDE(w, LV_COLOR_FORMAT_RGB565);       // Compute stride (row length in bytes, aligned)
        const size_t buf_size = LV_CANVAS_BUF_SIZE(w, h, LV_COLOR_FORMAT_RGB565, stride);
        void *buf = malloc(buf_size);
        if (!buf) {

            ESP_LOGE("display", "Failed to allocate canvas buffer");
            return;
        }

        lv_obj_t* canvas = lv_canvas_create(lv_screen_active());
        lv_obj_set_size(canvas, w, h);
        lv_obj_center(canvas);
        lv_canvas_set_buffer(canvas, buf, w, h, LV_COLOR_FORMAT_RGB565);    // Use lv_canvas_set_buffer() which accepts the stride
        lv_timer_create(timer_cb, 33, canvas);
    }
    
    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
