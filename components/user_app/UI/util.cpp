
#include "util/pch.hpp"
#include "util.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI::util {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    #define ENABLE_LOC_DEBUGGING                0

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


    lv_obj_t* create_geometric_pattern_0(lv_obj_t* screen, lv_color_t color1, lv_color_t color2) {

        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

        const lv_coord_t scr_w = lv_obj_get_width(screen);
        const lv_coord_t scr_h = lv_obj_get_height(screen);
        const lv_coord_t margin = LV_MIN(scr_w, scr_h) / 12;

        lv_obj_t* canvas = lv_canvas_create(screen);
        lv_obj_set_size(canvas, scr_w, scr_h);
        lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);

        size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR(scr_w, scr_h);
        lv_color_t* buf = (lv_color_t*)lv_mem_alloc(buf_size * sizeof(lv_color_t));  // cast required
        if (!buf)
            return canvas;

        lv_canvas_set_buffer(canvas, buf, scr_w, scr_h, LV_IMG_CF_TRUE_COLOR);
        lv_obj_set_user_data(canvas, buf);                          // store buffer so we can free it later
        lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

        // ----- Create the geometric pattern -----
        const lv_coord_t cx = scr_w / 2;
        const lv_coord_t cy = scr_h / 2;
        const lv_coord_t radius = (LV_MIN(scr_w, scr_h) * 1.25f) - margin;

        // Central hexagon (flat‑top, rotated 30°)
        constexpr u8 HEX_VERTICES = 6;
        lv_point_t hex_points[HEX_VERTICES];
        for (u8 i = 0; i < HEX_VERTICES; i++) {

            const f32 angle = 2.0f * M_PI * i / HEX_VERTICES - (M_PI / 6.0f);
            hex_points[i].x = cx + (lv_coord_t)(radius * cosf(angle));
            hex_points[i].y = cy + (lv_coord_t)(radius * sinf(angle));
        }

        // Draw hexagon outline using a rectangle descriptor (polygon = border only here)
        lv_draw_rect_dsc_t poly_dsc;
        lv_draw_rect_dsc_init(&poly_dsc);
        poly_dsc.border_color = color2;
        poly_dsc.border_opa = LV_OPA_COVER;
        poly_dsc.border_width = 2;
        poly_dsc.bg_opa = LV_OPA_TRANSP;    // no fill
        lv_canvas_draw_polygon(canvas, hex_points, HEX_VERTICES, &poly_dsc);

        // Radial lines from centre to each vertex (alternating colours)
        lv_point_t center = {cx, cy};
        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.round_start = 1;
        line_dsc.round_end   = 1;
        line_dsc.opa = LV_OPA_COVER;
        for (u8 i = 0; i < HEX_VERTICES; i++) {

            line_dsc.color = (i % 2 == 0) ? color1 : color2;
            line_dsc.width = 2;
            lv_point_t line_points[] = {center, hex_points[i]};
            lv_canvas_draw_line(canvas, line_points, 2, &line_dsc);
        }

        // Decorative arc (bold semicircle, colour2)
        const lv_coord_t arc_x = cx + radius / 2;
        const lv_coord_t arc_y = cy - radius / 3;
        const u16 arc_radius = radius / 3;
        const i32 start_angle = 450;        // 90° (top) in LVGL arc convention (0° = 3 o'clock)
        const i32 end_angle   = 1350;       // 270°
        lv_draw_arc_dsc_t arc_dsc;
        lv_draw_arc_dsc_init(&arc_dsc);
        arc_dsc.color = color2;
        arc_dsc.width = 5;                  // bold arc
        arc_dsc.opa = LV_OPA_COVER;
        arc_dsc.rounded = 1;
        lv_canvas_draw_arc(canvas, arc_x, arc_y, arc_radius, start_angle, end_angle, &arc_dsc);

        // Dot accents at hexagon vertices (manual pixel fill, colour1)
        for (i32 i = 0; i < HEX_VERTICES; i++) {
            for (i32 dx = -2; dx <= 2; dx++) {
                for (i32 dy = -2; dy <= 2; dy++) {
                    if (dx*dx + dy*dy <= 5) {  // roughly circular dot
                        lv_coord_t px = hex_points[i].x + dx;
                        lv_coord_t py = hex_points[i].y + dy;
                        if (px >= 0 && px < scr_w && py >= 0 && py < scr_h)
                            lv_canvas_set_px_color(canvas, px, py, color1);
                    }
                }
            }
        }

        // Small diagonal crosses at edge midpoints (colour2, using line_dsc)
        line_dsc.width = 2;
        line_dsc.color = color2;
        for (u8 i = 0; i < HEX_VERTICES; i++) {

            const i32 next = (i + 1) % HEX_VERTICES;
            const lv_coord_t mx = (lv_coord_t)((hex_points[i].x + hex_points[next].x) / 2);
            const lv_coord_t my = (lv_coord_t)((hex_points[i].y + hex_points[next].y) / 2);

            // Cast the results to lv_coord_t to avoid narrowing errors.
            const lv_point_t cross1[] = {
                {(lv_coord_t)(mx - 3), (lv_coord_t)(my - 3)},
                {(lv_coord_t)(mx + 3), (lv_coord_t)(my + 3)}
            };
            const lv_point_t cross2[] = {
                {(lv_coord_t)(mx + 3), (lv_coord_t)(my - 3)},
                {(lv_coord_t)(mx - 3), (lv_coord_t)(my + 3)}
            };
            lv_canvas_draw_line(canvas, cross1, 2, &line_dsc);
            lv_canvas_draw_line(canvas, cross2, 2, &line_dsc);
        }
        return canvas;
    }


    lv_obj_t* create_geometric_pattern_1(lv_obj_t* screen, lv_color_t color1, lv_color_t color2) {

        // Black background
        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

        const lv_coord_t scr_w = lv_obj_get_width(screen);
        const lv_coord_t scr_h = lv_obj_get_height(screen);
        const lv_coord_t margin = LV_MIN(scr_w, scr_h) / 12;

        // Canvas covering the whole screen
        lv_obj_t* canvas = lv_canvas_create(screen);
        lv_obj_set_size(canvas, scr_w, scr_h);
        lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);

        const size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR(scr_w, scr_h);
        lv_color_t* buf = (lv_color_t*)lv_mem_alloc(buf_size * sizeof(lv_color_t));
        if (!buf)
            return canvas;
        lv_canvas_set_buffer(canvas, buf, scr_w, scr_h, LV_IMG_CF_TRUE_COLOR);
        lv_obj_set_user_data(canvas, buf);                          // store buffer so we can free it later
        lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

        /*----- Radar / tech pattern -----*/

        const lv_coord_t cx = scr_w / 2;
        const lv_coord_t cy = scr_h / 2;
        const lv_coord_t max_r = (LV_MIN(scr_w, scr_h) / 2) - margin;   // outer radius

        // Descriptors
        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.opa = LV_OPA_COVER;
        line_dsc.round_start = 1;
        line_dsc.round_end = 1;

        lv_draw_arc_dsc_t arc_dsc;
        lv_draw_arc_dsc_init(&arc_dsc);
        arc_dsc.opa = LV_OPA_COVER;
        arc_dsc.rounded = 0;

        lv_draw_rect_dsc_t poly_dsc;                    // for filled polygons (wedge)
        lv_draw_rect_dsc_init(&poly_dsc);
        poly_dsc.border_opa = LV_OPA_TRANSP;            // no border for the wedge

        arc_dsc.color = color2;                         // Outer circle (color2, thin)
        arc_dsc.width = 2;
        lv_canvas_draw_arc(canvas, cx, cy, max_r, 0, 3600, &arc_dsc);

        // Radial spokes every 30° (12 lines) alternating colors
        for (u8 i = 0; i < 12; i++) {

            f32 angle_deg = i * 30.0f;                  // 0°, 30°, 60°, ...
            f32 rad = angle_deg * M_PI / 180.0f;
            lv_coord_t ex = cx + (lv_coord_t)(max_r * cosf(rad));
            lv_coord_t ey = cy + (lv_coord_t)(max_r * sinf(rad));

            line_dsc.color = (i % 2 == 0) ? color1 : color2;
            line_dsc.width = (i % 4 == 0) ? 3 : 1;      // thicker main axes every 90°

            lv_point_t points[] = {{cx, cy}, {ex, ey}};
            lv_canvas_draw_line(canvas, points, 2, &line_dsc);
        }

        // Two dashed concentric rings (at 1/3 and 2/3 of max radius, color1)
        // Dashed effect: 15° arc, 15° gap, repeat.
        for (u8 ring = 1; ring <= 2; ring++) {
            lv_coord_t r = max_r * ring / 3;
            arc_dsc.color = color1;
            arc_dsc.width = 2;
            for (u32 a = 0; a < 360; a += 30) {
                int32_t start = a * 10;                 // 0.1° per unit => a*10
                int32_t end   = (a + 15) * 10;          // 15° arc
                lv_canvas_draw_arc(canvas, cx, cy, r, start, end, &arc_dsc);
            }
        }

        // Central hub (filled circle, color1)
        // Using arc trick: full circle with width = 2 * hub_radius
        lv_coord_t hub_r = max_r / 10;
        arc_dsc.color = color1;
        arc_dsc.width = hub_r * 2;    // creates a solid disc
        arc_dsc.rounded = 1;          // smooth edge
        lv_canvas_draw_arc(canvas, cx, cy, hub_r, 0, 3600, &arc_dsc);

        // Semi‑transparent sweep wedge (45° wide, color1 at 50% opacity)
        // Triangle: center + two points on the outer circle at 0° and 45°
        f32 start_rad = 0.0f;
        f32 end_rad   = 45.0f * M_PI / 180.0f;
        lv_point_t wedge[3];
        wedge[0].x = cx;
        wedge[0].y = cy;
        wedge[1].x = cx + (lv_coord_t)(max_r * cosf(start_rad));
        wedge[1].y = cy + (lv_coord_t)(max_r * sinf(start_rad));
        wedge[2].x = cx + (lv_coord_t)(max_r * cosf(end_rad));
        wedge[2].y = cy + (lv_coord_t)(max_r * sinf(end_rad));

        poly_dsc.bg_color = color1;
        poly_dsc.bg_opa = LV_OPA_50;   // 50% translucent sweep
        lv_canvas_draw_polygon(canvas, wedge, 3, &poly_dsc);

        // Cardinal tick marks (small crosses at N/E/S/W on outer ring)
        line_dsc.color = color2;
        line_dsc.width = 2;
        lv_coord_t tick_len = max_r / 20;
        for (u8 i = 0; i < 4; i++) {
            f32 rad = i * M_PI / 2.0f;  // 0°, 90°, 180°, 270°
            lv_coord_t px = cx + (lv_coord_t)(max_r * cosf(rad));
            lv_coord_t py = cy + (lv_coord_t)(max_r * sinf(rad));
            lv_coord_t dx = (lv_coord_t)(tick_len * cosf(rad + M_PI/2)); // perpendicular
            lv_coord_t dy = (lv_coord_t)(tick_len * sinf(rad + M_PI/2));
            lv_point_t tick[] = {
                {(lv_coord_t)(px - dx), (lv_coord_t)(py - dy)},
                {(lv_coord_t)(px + dx), (lv_coord_t)(py + dy)}
            };
            lv_canvas_draw_line(canvas, tick, 2, &line_dsc);
        }
        return canvas;
    }


    lv_obj_t* create_geometric_pattern_2(lv_obj_t* screen, lv_color_t color1, lv_color_t color2) {

        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);     // Black background
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
        lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);          // Prevent unwanted scrolling
        lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

        const lv_coord_t scr_w = lv_obj_get_width(screen);
        const lv_coord_t scr_h = lv_obj_get_height(screen);
        const lv_coord_t margin = LV_MIN(scr_w, scr_h) / 12;

        lv_obj_t* canvas = lv_canvas_create(screen);                // Full‑screen canvas
        lv_obj_set_size(canvas, scr_w, scr_h);
        lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_clear_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);          // (optional) disable scrolling on the canvas too

        size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR(scr_w, scr_h);
        lv_color_t* buf = (lv_color_t*)lv_mem_alloc(buf_size * sizeof(lv_color_t));
        if (!buf)
            return canvas;

        lv_canvas_set_buffer(canvas, buf, scr_w, scr_h, LV_IMG_CF_TRUE_COLOR);
        lv_obj_set_user_data(canvas, buf);                          // store buffer so we can free it later
        lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

        // Relative sizes based on the smaller screen dimension
        const lv_coord_t small = LV_MIN(scr_w, scr_h);
        const lv_coord_t tri1_size = small * 2 / 5;                 // big top‑left triangle (≈180 px on 480p)
        const lv_coord_t tri2_size = tri1_size * 8 / 10;            // smaller bottom‑right triangle
        const lv_coord_t line_spacing = small / 8;                  // ≈100 px between diagonal lines
        const lv_coord_t line_width = 3;

        // Descriptors
        lv_draw_rect_dsc_t poly_dsc;
        lv_draw_rect_dsc_init(&poly_dsc);
        poly_dsc.border_opa = LV_OPA_TRANSP;                        // no outline

        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.round_start = 1;
        line_dsc.round_end   = 1;

        lv_draw_arc_dsc_t arc_dsc;
        lv_draw_arc_dsc_init(&arc_dsc);
        arc_dsc.rounded = 1;

        {   // Top‑left filled triangle (color1, 70% opacity) Equilateral triangle centered at the top‑left corner (0,0)
            lv_point_t pts[3];
            for (int i = 0; i < 3; i++) {

                const f32 angle = i * 2.0f * M_PI / 3.0f;                 // 0°, 120°, 240°
                pts[i].x = (lv_coord_t)(tri1_size * cosf(angle));
                pts[i].y = (lv_coord_t)(tri1_size * sinf(angle));
            }
            poly_dsc.bg_color = color1;
            poly_dsc.bg_opa = LV_OPA_70;                            // semi‑transparent
            lv_canvas_draw_polygon(canvas, pts, 3, &poly_dsc);
        }

        {   // Bottom‑right filled triangle (color1, 40% opacity)
            const lv_coord_t cx = scr_w;
            const lv_coord_t cy = scr_h;
            lv_point_t pts[3];
            for (u8 i = 0; i < 3; i++) {

                const f32 angle = i * 2.0f * M_PI / 3.0f;                 // 0°, 120°, 240°
                pts[i].x = cx + (lv_coord_t)(tri2_size * cosf(angle));
                pts[i].y = cy + (lv_coord_t)(tri2_size * sinf(angle));
            }
            poly_dsc.bg_color = color1;
            poly_dsc.bg_opa = LV_OPA_40;
            lv_canvas_draw_polygon(canvas, pts, 3, &poly_dsc);
        }

        // Diagonal lines (color2, 50% opacity)
        line_dsc.color = color2;
        line_dsc.opa = LV_OPA_50;
        line_dsc.width = line_width;
        for (u8 i = 0; i < 5; i++) {

            const lv_coord_t offset = i * line_spacing;
            const lv_point_t tl_line[] = {                                // Top‑left: line from (offset, 0) to (0, offset)
                {offset, 0},
                {0, offset}
            };
            lv_canvas_draw_line(canvas, tl_line, 2, &line_dsc);

            const lv_point_t br_line[] = {                                // Bottom‑right: line from (scr_w - offset, scr_h) to (scr_w, scr_h - offset)
                {(lv_coord_t)(scr_w - offset), scr_h},
                {scr_w, (lv_coord_t)(scr_h - offset)}
            };
            lv_canvas_draw_line(canvas, br_line, 2, &line_dsc);
        }

        // Three floating filled circles (color2, 35% opacity) Placed near the bottom‑left area (similar to the ImGui cluster)
        arc_dsc.color = color2;
        arc_dsc.opa = LV_OPA_30;
        for (u8 i = 0; i < 3; i++) {

            const lv_coord_t cx = margin + (i + 1) * (small / 8);
            const lv_coord_t cy = scr_h - margin - (i + 1) * (small / 10);
            const lv_coord_t radius = 5 + i * 3;

            // Draw a filled circle by using an arc with width = 2 * radius
            arc_dsc.width = radius * 2;
            lv_canvas_draw_arc(canvas, cx, cy, radius, 0, 3600, &arc_dsc);
        }
        return canvas;
    }


    vec_2d lv_point_to_percent(const lv_point_t& p) {

        lv_disp_t* disp = lv_disp_get_default();
        if (!disp)
            return {0, 0};

        lv_coord_t w = lv_disp_get_hor_res(disp);
        lv_coord_t h = lv_disp_get_ver_res(disp);
        return {
            static_cast<i8>((p.x * 100) / w),
            static_cast<i8>((p.y * 100) / h)
        };
    }


    swipe_direction get_swipe_direction(const touch_movement_data& data) {

        //------- Thresholds (tune to your liking)-------
        constexpr i8 MIN_SWIPE_LENGTH               = 15;   // minimum net displacement in any direction (percent)
        constexpr i8 START_REGION                   = 20;   // percentage from edge to consider "near edge"
        constexpr i8 DIAGONAL_ANGLE_TOL             = 20;   // degrees tolerance around 45° for diagonals

        // Net displacement
        const i8 dx = data.touch_stop.x - data.touch_start.x;
        const i8 dy = data.touch_stop.y - data.touch_start.y;
        const i8 len = std::abs(dx) + std::abs(dy);         // Manhattan length (quick filter)
        if (len < MIN_SWIPE_LENGTH)
            return swipe_direction::none;

        double angle = std::atan2(dy, dx) * 180.0 / M_PI;   // Angle in degrees (0 = right, 90 = down, ±180 = left)
        if (angle < 0)
            angle += 360.0;                                 // Normalize to [0, 360)

        // Determine cardinal direction based on angle
        swipe_direction dir = swipe_direction::none;
        if (angle > 45 && angle <= 135)             dir = swipe_direction::down;
        else if (angle > 135 && angle <= 225)       dir = swipe_direction::left;
        else if (angle > 225 && angle <= 315)       dir = swipe_direction::up;
        else                                        dir = swipe_direction::right;  // 315–360 or 0–45

        // Check for diagonal: if the angle is near 45°, 135°, 225°, 315° with tolerance
        const double mod45 = std::fmod(angle + 45, 90);     // distance to nearest 45° multiple
        if (mod45 < DIAGONAL_ANGLE_TOL || mod45 > 90 - DIAGONAL_ANGLE_TOL) {

            // Refine diagonal direction
            if (angle > 0 && angle < 90)            dir = swipe_direction::down_right;
            else if (angle > 90 && angle < 180)     dir = swipe_direction::down_left;
            else if (angle > 180 && angle < 270)    dir = swipe_direction::up_left;
            else if (angle > 270 && angle < 360)    dir = swipe_direction::up_right;
        }

        //----- Start‑region constraints (to avoid accidental swipes)-----
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

        #if ENABLE_LOC_DEBUGGING
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
                dir_str[static_cast<i32>(dir)],
                start_ok ? "✅" : "❌"
            );
        #endif

        return (start_ok && dir != swipe_direction::none) ? dir : swipe_direction::none;
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
