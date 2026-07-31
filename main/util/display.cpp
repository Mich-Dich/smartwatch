
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
    

    void draw_background_pattern_arcs_markers_hexagon(lv_obj_t* parent) {

        i32 w = lv_display_get_horizontal_resolution(lv_display_get_default());
        i32 h = lv_display_get_vertical_resolution(lv_display_get_default());

        size_t buf_size = LV_CANVAS_BUF_SIZE(w, h, LV_COLOR_FORMAT_RGB565, LV_DRAW_BUF_STRIDE_ALIGN);
        void* buf = malloc(buf_size);
        assert(buf);

        lv_obj_t* canvas = lv_canvas_create(parent);
        lv_obj_set_size(canvas, w, h);
        lv_obj_center(canvas);
        lv_canvas_set_buffer(canvas, buf, w, h, LV_COLOR_FORMAT_RGB565);
        lv_obj_move_background(canvas);

        lv_layer_t layer;
        lv_canvas_init_layer(canvas, &layer);
        lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

        // ---- Arcs ----
        lv_draw_arc_dsc_t arc_dsc;
        lv_draw_arc_dsc_init(&arc_dsc);
        arc_dsc.color = lv_color_hex(0x336699);
        arc_dsc.width = 2;
        arc_dsc.rounded = 1;

        int cx = w / 2;
        int cy = h / 2;

        arc_dsc.radius = 180;
        arc_dsc.start_angle = 20;
        arc_dsc.end_angle = 340;
        lv_draw_arc(&layer, &arc_dsc);

        arc_dsc.color = lv_color_hex(0x225577);
        arc_dsc.width = 1;
        arc_dsc.radius = 140;
        arc_dsc.start_angle = 0;
        arc_dsc.end_angle = 360;
        lv_draw_arc(&layer, &arc_dsc);

        // ---- Hour markers ----
        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.color = lv_color_hex(0x333333);
        line_dsc.width = 1;

        lv_draw_rect_dsc_t dot_dsc;
        lv_draw_rect_dsc_init(&dot_dsc);
        dot_dsc.bg_color = lv_color_hex(0x6699CC);
        dot_dsc.bg_opa = LV_OPA_30;

        for (int i = 0; i < 12; i++) {
            int angle = i * 30;
            float rad = (angle - 90) * 3.14159f / 180.0f;

            line_dsc.p1.x = cx + 150 * cosf(rad);
            line_dsc.p1.y = cy + 150 * sinf(rad);
            line_dsc.p2.x = cx + 165 * cosf(rad);
            line_dsc.p2.y = cy + 165 * sinf(rad);
            lv_draw_line(&layer, &line_dsc);

            int x = cx + 170 * cosf(rad) - 3;
            int y = cy + 170 * sinf(rad) - 3;
            lv_area_t dot = {x, y, x + 6, y + 6};
            lv_draw_rect(&layer, &dot_dsc, &dot);
        }

        // ---- Central hexagon ----
        line_dsc.color = lv_color_hex(0x335577);
        line_dsc.width = 2;
        for (int i = 0; i < 6; i++) {
            float rad1 = (i * 60 - 30) * 3.14159f / 180.0f;
            float rad2 = ((i + 1) % 6 * 60 - 30) * 3.14159f / 180.0f;
            line_dsc.p1.x = cx + 60 * cosf(rad1);
            line_dsc.p1.y = cy + 60 * sinf(rad1);
            line_dsc.p2.x = cx + 60 * cosf(rad2);
            line_dsc.p2.y = cy + 60 * sinf(rad2);
            lv_draw_line(&layer, &line_dsc);
        }

        lv_canvas_finish_layer(canvas, &layer);
    }


    void draw_background_pattern_002(lv_obj_t* parent) {

        i32 w = lv_display_get_horizontal_resolution(lv_display_get_default());
        i32 h = lv_display_get_vertical_resolution(lv_display_get_default());

        size_t buf_size = LV_CANVAS_BUF_SIZE(w, h, LV_COLOR_FORMAT_RGB565, LV_DRAW_BUF_STRIDE_ALIGN);
        void* buf = malloc(buf_size);
        assert(buf);

        lv_obj_t* canvas = lv_canvas_create(parent);
        lv_obj_set_size(canvas, w, h);
        lv_obj_center(canvas);
        lv_canvas_set_buffer(canvas, buf, w, h, LV_COLOR_FORMAT_RGB565);
        lv_obj_move_background(canvas);

        lv_layer_t layer;
        lv_canvas_init_layer(canvas, &layer);
        lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

        int cx = w / 2;
        int cy = h / 2;

        // Outer dashed arcs
        lv_draw_arc_dsc_t arc_dsc;
        lv_draw_arc_dsc_init(&arc_dsc);
        arc_dsc.color = lv_color_hex(0x336699);
        arc_dsc.width = 2;
        arc_dsc.rounded = 1;

        for (int r = 170; r >= 110; r -= 20) {
            arc_dsc.radius = r;
            arc_dsc.start_angle = (r % 40 == 0) ? 0 : 15;
            arc_dsc.end_angle   = 360;
            lv_draw_arc(&layer, &arc_dsc);
        }

        // Concentric triangles
        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.color = lv_color_hex(0x225577);
        line_dsc.width = 1;

        for (int i = 0; i < 6; i++) {
            float rad = (i * 60 - 90) * 3.14159f / 180.0f;
            int x1 = cx + 40 * cosf(rad);
            int y1 = cy + 40 * sinf(rad);
            int x2 = cx + 100 * cosf(rad + 0.5f);
            int y2 = cy + 100 * sinf(rad + 0.5f);
            line_dsc.p1.x = x1; line_dsc.p1.y = y1;
            line_dsc.p2.x = x2; line_dsc.p2.y = y2;
            lv_draw_line(&layer, &line_dsc);

            rad = (i * 60 + 30 - 90) * 3.14159f / 180.0f;
            x1 = cx + 60 * cosf(rad);
            y1 = cy + 60 * sinf(rad);
            x2 = cx + 120 * cosf(rad);
            y2 = cy + 120 * sinf(rad);
            line_dsc.p1.x = x1; line_dsc.p1.y = y1;
            line_dsc.p2.x = x2; line_dsc.p2.y = y2;
            lv_draw_line(&layer, &line_dsc);
        }

        // Small dots on outer ring
        lv_draw_rect_dsc_t dot_dsc;
        lv_draw_rect_dsc_init(&dot_dsc);
        dot_dsc.bg_color = lv_color_hex(0x6699CC);
        dot_dsc.bg_opa = LV_OPA_30;

        for (int i = 0; i < 24; i++) {
            float rad = (i * 15 - 90) * 3.14159f / 180.0f;
            int x = cx + 175 * cosf(rad) - 2;
            int y = cy + 175 * sinf(rad) - 2;
            lv_area_t dot = {x, y, x + 5, y + 5};
            lv_draw_rect(&layer, &dot_dsc, &dot);
        }

        lv_canvas_finish_layer(canvas, &layer);
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
