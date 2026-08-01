
#include "util/pch.hpp"
#include "util.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI::util {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    void set_background(lv_obj_t* screen, lv_color_t color) {

        if (!screen) 
            return;

        lv_obj_set_style_bg_color(screen, color, 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
        // Optionally remove borders/padding to make it clean
        lv_obj_set_style_border_width(screen, 0, 0);
        lv_obj_set_style_pad_all(screen, 0, 0);
        lv_obj_set_style_radius(screen, 0, 0);
    }


    void create_geometric_pattern(lv_obj_t* screen, lv_color_t color1, lv_color_t color2) {

        if (!screen)
            return;

        // Get screen dimensions
        lv_coord_t w = lv_obj_get_width(screen);
        lv_coord_t h = lv_obj_get_height(screen);
        if (w == 0 || h == 0) {
            lv_disp_t* disp = lv_disp_get_default();
            if (disp) {
                w = lv_disp_get_hor_res(disp);
                h = lv_disp_get_ver_res(disp);
            }
            if (w == 0 || h == 0) {
                ESP_LOGE(TAG, "Cannot determine screen size");
                return;
            }
        }

        // Allocate canvas buffer (16‑bit true color)
        size_t buf_size = w * h * sizeof(lv_color_t);
        lv_color_t* buf = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!buf) {
            ESP_LOGE(TAG, "Failed to allocate canvas buffer");
            return;
        }

        // Create canvas object
        lv_obj_t* canvas = lv_canvas_create(screen);
        lv_canvas_set_buffer(canvas, buf, w, h, LV_IMG_CF_TRUE_COLOR);
        lv_obj_clear_flag(canvas, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_opa(canvas, LV_OPA_TRANSP, 0);

        // Fill background with color1
        lv_canvas_fill_bg(canvas, color1, LV_OPA_COVER);

        // -------- Draw with color2 --------

        // 1. Concentric circles (arcs)
        lv_draw_arc_dsc_t arc_dsc;
        lv_draw_arc_dsc_init(&arc_dsc);
        arc_dsc.color = color2;
        arc_dsc.width = 2;
        arc_dsc.opa = LV_OPA_COVER;

        lv_coord_t cx = w / 2;
        lv_coord_t cy = h / 2;
        int max_r = (w < h) ? w / 2 : h / 2;
        for (int r = 30; r < max_r; r += 30) {
            lv_canvas_draw_arc(canvas, cx, cy, r, 0, 3600, &arc_dsc);
        }

        // 2. Radiating lines (sunburst)
        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.color = color2;
        line_dsc.width = 2;
        line_dsc.opa = LV_OPA_COVER;

        for (int angle = 0; angle < 360; angle += 15) {
            float rad = angle * 3.14159f / 180.0f;
            lv_point_t points[2];
            points[0].x = cx;
            points[0].y = cy;
            points[1].x = (lv_coord_t)(cx + max_r * cosf(rad));
            points[1].y = (lv_coord_t)(cy + max_r * sinf(rad));
            lv_canvas_draw_line(canvas, points, 2, &line_dsc);
        }

        // 3. Scattered diamonds (filled rounded rectangles)
        lv_draw_rect_dsc_t rect_dsc;
        lv_draw_rect_dsc_init(&rect_dsc);
        rect_dsc.bg_color = color2;
        rect_dsc.bg_opa = LV_OPA_30;
        rect_dsc.border_width = 0;
        rect_dsc.radius = 8;   // will make squares look like diamonds

        const int num_diamonds = 20;
        for (int i = 0; i < num_diamonds; ++i) {
            int x = rand() % w;
            int y = rand() % h;
            int size = 12 + (rand() % 20);
            // Draw a rounded square (looks like a diamond with rounded corners)
            lv_canvas_draw_rect(canvas, x - size/2, y - size/2, size, size, &rect_dsc);
        }

        // Apply canvas as screen background
        lv_obj_set_style_bg_img_src(screen, lv_canvas_get_img(canvas), 0);
        lv_obj_set_style_bg_img_opa(screen, LV_OPA_COVER, 0);
        lv_obj_move_to_index(canvas, 0);
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
