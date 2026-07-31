
#include "util/pch.hpp"
#include "music.hpp"

#include <dirent.h>



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

    void music_screen::create(lv_obj_t* screen) {

        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

        // Title
        lv_obj_t* title = lv_label_create(screen);
        lv_label_set_text(title, "Music Player");
        lv_obj_set_style_text_color(title, lv_color_hex(0x6699CC), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

        // Scrollable list container
        lv_obj_t* list = lv_obj_create(screen);
        lv_obj_set_size(list, LV_PCT(90), LV_PCT(55));
        lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);
        lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(list, 0, 0);
        lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_scroll_dir(list, LV_DIR_VER);
        lv_obj_set_style_pad_all(list, 10, 0);

        m_list = list;

        // Read the /sdcard/music directory
        DIR* dir = opendir("/sdcard/music");
        if (!dir) {
            lv_obj_t* err = lv_label_create(list);
            lv_label_set_text(err, "No /sdcard/music found");
            lv_obj_set_style_text_color(err, lv_color_white(), 0);
            return;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            // Only show .mp3 files
            if (name.size() > 4 && name.substr(name.size() - 4) == ".mp3") {
                m_files.push_back("/sdcard/music/" + name);   // full path

                // Create a label for the filename
                lv_obj_t* item = lv_label_create(list);
                lv_label_set_text(item, name.c_str());
                lv_obj_set_style_text_color(item, lv_color_white(), 0);
                lv_obj_set_style_text_font(item, &lv_font_montserrat_20, 0);
                lv_obj_set_style_pad_ver(item, 6, 0);
            }
        }
        closedir(dir);

        if (m_files.empty()) {
            lv_obj_t* empty = lv_label_create(list);
            lv_label_set_text(empty, "No MP3 files found");
            lv_obj_set_style_text_color(empty, lv_color_white(), 0);
        }
    }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
