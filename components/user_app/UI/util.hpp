
#pragma once



// FORWARD DECLARATIONS =====================================================================================

namespace APP::UI::util {

    // CONSTANTS ============================================================================================

    // MACROS ===============================================================================================

    // TYPES ================================================================================================

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
    void create_geometric_pattern(lv_obj_t* screen, lv_color_t color1, lv_color_t color2);

    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================

}
