
#include "util/pch.hpp"
#include "user_app.hpp"

#include "UI/screen_manager.hpp"
#include "UI/screens/main_screen.hpp"
#include "UI/screens/settings_screen.hpp"
#include "UI/screens/wifi_screen.hpp"


// FORWARD DECLARATIONS ================================================================================================

EventGroupHandle_t      TaskEven;



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
        adc_bsp_init();
        nvs_flash_Init();
        ble_scan_class_init();
        ble_scan_Init();

        APP::UI::screen_manager::register_screen("main", std::move(std::make_unique<APP::UI::main_screen>()));
        APP::UI::screen_manager::register_screen("settings", std::move(std::make_unique<APP::UI::settings_screen>()));
        APP::UI::screen_manager::register_screen("wifi", std::move(std::make_unique<APP::UI::wifi_screen>()));
        APP::UI::screen_manager::switch_to("main");
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
