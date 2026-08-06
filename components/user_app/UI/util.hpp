
#pragma once



// FORWARD DECLARATIONS =====================================================================================

namespace APP::UI::util {

    // CONSTANTS ============================================================================================

    // MACROS ===============================================================================================

    // TYPES ================================================================================================

    enum class swipe_direction {
        none = 0,
        up,
        down,
        left,
        right,
        up_left,
        up_right,
        down_left,
        down_right
    };


    struct vec_2d {

        i8          x{};
        i8          y{};

    };
    

    // Equality / inequality
    inline bool operator==(const vec_2d& a, const vec_2d& b)   { return a.x == b.x && a.y == b.y; }
    inline bool operator!=(const vec_2d& a, const vec_2d& b)   { return !(a == b); }

    // Component‑wise ordering (partial order)
    inline bool operator< (const vec_2d& a, const vec_2d& b)   { return a.x < b.x && a.y < b.y; }
    inline bool operator> (const vec_2d& a, const vec_2d& b)   { return b < a; }                     // or a.x > b.x && a.y > b.y
    inline bool operator<=(const vec_2d& a, const vec_2d& b)   { return a.x <= b.x && a.y <= b.y; }
    inline bool operator>=(const vec_2d& a, const vec_2d& b)   { return b <= a; }

    inline vec_2d operator+(const vec_2d& a, const vec_2d& b)  { return { static_cast<i8>(a.x + b.x), static_cast<i8>(a.y + b.y) }; }
    inline vec_2d operator-(const vec_2d& a, const vec_2d& b)  { return { static_cast<i8>(a.x - b.x), static_cast<i8>(a.y - b.y) }; }
    inline vec_2d operator-(const vec_2d& v)                   { return { static_cast<i8>(-v.x), static_cast<i8>(-v.y) }; }

    inline vec_2d& operator+=(vec_2d& a, const vec_2d& b) {        // (Optional) Compound assignment +=
        a.x += b.x;
        a.y += b.y;
        return a;
    }

    inline vec_2d& operator-=(vec_2d& a, const vec_2d& b) {        // (Optional) Compound assignment -=
        a.x -= b.x;
        a.y -= b.y;
        return a;
    }


    struct touch_movement_data {

        touch_movement_data(vec_2d touch_start, vec_2d touch_size, vec_2d touch_stop, vec_2d touch_max, vec_2d touch_min)
            : touch_start(touch_start), touch_size(touch_size), touch_stop(touch_stop), touch_max(touch_max), touch_min(touch_min) {}
        
        touch_movement_data(vec_2d point)
            : touch_start(point), touch_size({}), touch_stop(point), touch_max(point), touch_min(point) {}
        
        touch_movement_data()
            : touch_start({}), touch_size({}), touch_stop({}), touch_max({}), touch_min({}) {}
        

        void update_size()  { touch_size = touch_max - touch_min; }

        void update_min(const vec_2d point)   { 
            
            if (point.x < touch_min.x)      touch_min.x = point.x;
            if (point.y < touch_min.y)      touch_min.y = point.y;
        }

        void update_max(const vec_2d point)   { 
            
            if (point.x > touch_max.x)      touch_max.x = point.x;
            if (point.y > touch_max.y)      touch_max.y = point.y;
        }

        vec_2d      touch_start = {};  // -1: not set; 0-100 the coordinate in percent
        vec_2d      touch_size = {};   // -1: not set; 0-100 the coordinate in percent
        vec_2d      touch_stop = {};   // -1: not set; 0-100 the coordinate in percent
        vec_2d      touch_max = {};    // -1: not set; 0-100 the coordinate in percent
        vec_2d      touch_min = {};    // -1: not set; 0-100 the coordinate in percent
    };

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // @brief Set the background color of a screen object.
    //
    // @param screen   The LVGL screen object (e.g., m_screen or m_ui.screen).
    // @param color    The desired background color (default: black).
    void set_background(lv_obj_t* screen, lv_color_t color = lv_color_hex(0x000000));


    // @brief Create a black background with a geometric pattern drawn in two colors.
    // @param screen  The LVGL screen object to style.
    // @param color1  Primary color (used for fills, e.g. dark grey/blue).
    // @param color2  Secondary color (used for strokes, e.g. white/cyan).
    void create_geometric_pattern_0(lv_obj_t* screen, lv_color_t color1, lv_color_t color2);
    
    
    // @brief Create a black background with a geometric pattern drawn in two colors.
    // @param screen  The LVGL screen object to style.
    // @param color1  Primary color (used for fills, e.g. dark grey/blue).
    // @param color2  Secondary color (used for strokes, e.g. white/cyan).
    void create_geometric_pattern_1(lv_obj_t* screen, lv_color_t color1, lv_color_t color2);


    // @brief Create a black background with a geometric pattern drawn in two colors.
    // @param screen  The LVGL screen object to style.
    // @param color1  Primary color (used for fills, e.g. dark grey/blue).
    // @param color2  Secondary color (used for strokes, e.g. white/cyan).
    void create_geometric_pattern_2(lv_obj_t* screen, lv_color_t color1, lv_color_t color2);


    swipe_direction get_swipe_direction(const touch_movement_data& data);
    
    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================

}
