
#pragma once

// C++ standard library
#include <cstddef>   // std::byte
#include <cstdint>   // std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include <time.h>

// ESP-IDF headers
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_dsp.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "nvs_flash.h"

// BSP and LVGL
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"

// internal code
#include "util/display.hpp"


// FORWARD DECLARATIONS =====================================================================================

extern const lv_font_t                  inconsolata_regular_64;

extern const lv_font_t                  inconsolata_regular_48;

namespace APP {

    // CONSTANTS ============================================================================================

    constexpr const char*               TAG = "audio_fft";

    // MACROS ===============================================================================================

    // TYPES ================================================================================================

    typedef std::uint8_t                u8;		// 8-bit unsigned integer
    typedef std::uint16_t               u16;	// 16-bit unsigned integer
    typedef std::uint32_t               u32;	// 32-bit unsigned integer
    typedef std::uint64_t               u64;	// 64-bit unsigned integer

    typedef std::int8_t                 i8;	 	// 8-bit signed integer
    typedef std::int16_t                i16; 	// 16-bit signed integer
    typedef std::int32_t                i32; 	// 32-bit signed integer
    typedef std::int64_t                i64; 	// 64-bit signed integer

    typedef float 					    f32;	// 32-bit floating point
    typedef double 					    f64;	// 64-bit floating point
    typedef long double 			    f128;	// 128-bit floating point (platform dependent)

    // Platform-specific types
    typedef unsigned long long 		    handle; // Generic handle type for OS resources

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================

}
