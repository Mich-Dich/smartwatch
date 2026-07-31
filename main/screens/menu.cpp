
#include "util/pch.hpp"
#include "menu.hpp"

#include "UI/manager.hpp"   // need get_home_screen() and get_normal_screens()
#include "util/wifi.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI {

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
    
    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    void menu_screen::create(lv_obj_t* screen) {

        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

        // Tileview (fills most of the screen)
        lv_obj_t* tileview = lv_tileview_create(screen);
        lv_obj_set_size(tileview, LV_PCT(50), LV_PCT(50));
        lv_obj_align(tileview, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(tileview, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(tileview, LV_OPA_TRANSP, 0);

        // Collect screen pointers in tile order: home screen first, then normal screens
        m_tile_screens.clear();
        for (APP::UI::screen const * s : screen_manager::get_normal_screens())
            m_tile_screens.push_back(s);

        for (size_t i = 0; i < m_tile_screens.size(); ++i) {                        // For each screen, create a tile with a label showing its name
            APP::UI::screen const* s = m_tile_screens[i];

            lv_obj_t* tile = lv_tileview_add_tile(tileview, i, 0, LV_DIR_HOR);
            lv_obj_set_style_bg_color(tile, lv_color_hex(0x1A1A2E), 0);
            lv_obj_set_style_border_width(tile, 2, 0);
            lv_obj_set_style_border_color(tile, lv_color_hex(0x336699), 0);
            lv_obj_set_style_radius(tile, 5, 0);

            lv_obj_t* label = lv_label_create(tile);
            lv_label_set_text(label, s->name());
            lv_obj_center(label);
            lv_obj_set_style_text_color(label, lv_color_white(), 0);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);

            lv_obj_add_event_cb(tile, tile_click_cb, LV_EVENT_CLICKED, this);       // Make the tile respond to taps
        }
    }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

    void menu_screen::tile_click_cb(lv_event_t* e) {
        
        menu_screen* self = static_cast<menu_screen*>(lv_event_get_user_data(e));
        lv_obj_t* tile = lv_event_get_target_obj(e);
        lv_obj_t* tileview = lv_obj_get_parent(tile);   // the tile’s parent is the tileview
        if (!tileview) return;

        // Find index of the clicked tile
        u32 child_count = lv_obj_get_child_cnt(tileview);
        u32 idx = 0;
        for (u32 i = 0; i < child_count; i++) {
            if (lv_obj_get_child(tileview, i) == tile) {
                idx = i;
                break;
            }
        }

        if (idx < self->m_tile_screens.size()) {
            screen const* target = self->m_tile_screens[idx];
            if (target) {
                screen_manager::navigate_to(target->name());
            }
        }
    }

}
