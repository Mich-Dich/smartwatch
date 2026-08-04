
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


    swipe_direction get_swipe_direction(const touch_movement_data& data) {

        // ---------- Thresholds (tune to your liking) ----------
        constexpr i8 MIN_SWIPE_LENGTH               = 15;   // minimum net displacement in any direction (percent)
        constexpr i8 MAX_DEVIATION                  = 30;   // max perpendicular deviation for cardinal swipes
        constexpr i8 START_REGION                   = 35;   // percentage from edge to consider "near edge"
        constexpr i8 DIAGONAL_ANGLE_TOL             = 20;   // degrees tolerance around 45° for diagonals

        // Net displacement
        const i8 dx = data.touch_stop.x - data.touch_start.x;
        const i8 dy = data.touch_stop.y - data.touch_start.y;
        const i8 len = std::abs(dx) + std::abs(dy);   // Manhattan length (quick filter)
        if (len < MIN_SWIPE_LENGTH)
            return swipe_direction::none;

        double angle = std::atan2(dy, dx) * 180.0 / M_PI;       // Angle in degrees (0 = right, 90 = down, ±180 = left)
        if (angle < 0) angle += 360.0;                          // Normalize to [0, 360)

        // Determine cardinal direction based on angle
        swipe_direction dir = swipe_direction::none;
        if (angle > 45 && angle <= 135)             dir = swipe_direction::down;
        else if (angle > 135 && angle <= 225)       dir = swipe_direction::left;
        else if (angle > 225 && angle <= 315)       dir = swipe_direction::up;
        else                                        dir = swipe_direction::right;  // 315–360 or 0–45

        // Check for diagonal: if the angle is near 45°, 135°, 225°, 315° with tolerance
        bool is_diag = false;
        const double mod45 = std::fmod(angle + 45, 90); // distance to nearest 45° multiple
        if (mod45 < DIAGONAL_ANGLE_TOL || mod45 > 90 - DIAGONAL_ANGLE_TOL) {
            
            is_diag = true;
            // Refine diagonal direction
            if (angle > 0 && angle < 90)            dir = swipe_direction::down_right;
            else if (angle > 90 && angle < 180)     dir = swipe_direction::down_left;
            else if (angle > 180 && angle < 270)    dir = swipe_direction::up_left;
            else if (angle > 270 && angle < 360)    dir = swipe_direction::up_right;
        }

        // -------- Start‑region constraints (to avoid accidental swipes) --------
        // For cardinal directions, we often want the swipe to start near the opposite edge.
        // For diagonals, start near the corresponding corner.
        bool start_ok = true;
        switch (dir) {
            case swipe_direction::down:         start_ok = (data.touch_start.y <= START_REGION); break;
            case swipe_direction::up:           start_ok = (data.touch_start.y >= 100 - START_REGION); break;
            case swipe_direction::right:        start_ok = (data.touch_start.x <= START_REGION); break;
            case swipe_direction::left:         start_ok = (data.touch_start.x >= 100 - START_REGION); break;
            case swipe_direction::down_right:   start_ok = (data.touch_start.x <= START_REGION && data.touch_start.y <= START_REGION); break;
            case swipe_direction::down_left:    start_ok = (data.touch_start.x >= 100 - START_REGION && data.touch_start.y <= START_REGION); break;
            case swipe_direction::up_right:     start_ok = (data.touch_start.x <= START_REGION && data.touch_start.y >= 100 - START_REGION); break;
            case swipe_direction::up_left:      start_ok = (data.touch_start.x >= 100 - START_REGION && data.touch_start.y >= 100 - START_REGION); break;
            default:                            start_ok = false; break;
        }

        // Also require the path extent (min/max) not to be too large or small? 
        // We can optionally add checks similar to before, but keep it simple for now.

        // Log everything
        const char* dir_str[] = {"None","Up","Down","Left","Right","UpLeft","UpRight","DownLeft","DownRight"};
        ESP_LOGI(TAG,
            "Swipe detection:\n"
            "  dx=%2d, dy=%2d, len=%2d, angle=%.1f°, start=(%2d,%2d)\n"
            "  → direction = %s, start_ok=%s",
            dx, dy, len, angle,
            data.touch_start.x, data.touch_start.y,
            dir_str[static_cast<int>(dir)],
            start_ok ? "✅" : "❌"
        );

        return (start_ok && dir != swipe_direction::none) ? dir : swipe_direction::none;
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
