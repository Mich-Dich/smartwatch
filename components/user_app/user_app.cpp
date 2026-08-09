
#include "util/pch.hpp"
#include "user_app.hpp"

#include "util/system.hpp"
#include "UI/screen_manager.hpp"
#include "UI/screens/main_screen.hpp"
#include "UI/screens/wifi_screen.hpp"
#include "UI/screens/bluetooth_screen.hpp"
#include "UI/screens/home_assist_screen.hpp"
#include "UI/screens/settings_screen.hpp"


// FORWARD DECLARATIONS ================================================================================================

EventGroupHandle_t      TaskEven;

// STATIC VARIABLES ====================================================================================================

lv_color_t                  highlight_color = lv_color_hex(0x1940ff);

lv_color_t                  support_color   = lv_color_hex(0x3f60ff);

namespace APP {

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

    extern "C" void application_begin(void) {

        TaskEven = xEventGroupCreate();
        xEventGroupSetBits( TaskEven,(0x01<<2) ); //wifi
        xEventGroupSetBits( TaskEven,(0x01<<1) ); //ble
        SD_card_Init();
        nvs_flash_Init();
        ble_scan_class_init();
        ble_scan_Init();

        APP::system::init();
        APP::UI::screen_manager::init();
        APP::UI::screen_manager::register_screen("Main", std::move(std::make_unique<APP::UI::main_screen>()));
        APP::UI::screen_manager::register_screen("Wifi", std::move(std::make_unique<APP::UI::wifi_screen>()));
        APP::UI::screen_manager::register_screen("Bluetooth", std::move(std::make_unique<APP::UI::bluetooth_screen>()));
        APP::UI::screen_manager::register_screen("Home Assist", std::move(std::make_unique<APP::UI::home_assist_screen>()));
        APP::UI::screen_manager::register_screen("Settings", std::move(std::make_unique<APP::UI::settings_screen>()));
        APP::UI::screen_manager::switch_to("Wifi");
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
