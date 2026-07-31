
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
#include <atomic>
#include <time.h>
#include <unordered_map>

// // ESP-IDF headers
// #include "freertos/FreeRTOS.h"
// #include "freertos/event_groups.h"
// #include "freertos/task.h"
// #include "esp_system.h"
// #include "esp_log.h"
// #include "esp_dsp.h"
// #include "esp_event.h"
// #include "esp_netif.h"
// #include "esp_sntp.h"
// #include "esp_timer.h"
// #include "nvs_flash.h"

// // BSP and LVGL
// #include "lvgl.h"
// #include "bsp/esp-bsp.h"
// #include "bsp/display.h"
// #include "bsp_board_extra.h"

// // internal code
// #include "util/display.hpp"



// FORWARD DECLARATIONS =====================================================================================

// extern const lv_font_t                  inconsolata_regular_64;

// extern const lv_font_t                  inconsolata_regular_48;

namespace APP {

    // CONSTANTS ============================================================================================

    constexpr const char*               TAG = "application";

    // MACROS ===============================================================================================

    #define VALIDATE(cond, action, tag, success_msg, fail_fmt, ...)                                         \
    do {                                                                                                    \
        if ((cond)) {                                                                                       \
            ESP_LOGI(tag, "%s", success_msg);                                                               \
        } else {                                                                                            \
            ESP_LOGE(tag, fail_fmt __VA_OPT__(,) __VA_ARGS__);                                              \
            action;                                                                                         \
        }                                                                                                   \
    } while(0);


    #define VALIDATE_S(cond, action, tag, fail_fmt, ...)                                                    \
    do {                                                                                                    \
        if ( !(cond)) {                                                                                     \
            ESP_LOGE(tag, fail_fmt __VA_OPT__(,) __VA_ARGS__);                                              \
            action;                                                                                         \
        }                                                                                                   \
    } while(0);

    // Force code to be inline
    #define FORCE_INLINE                                    inline __attribute__((always_inline))

    #define FORCE_INLINE_R                                  [[nodiscard]] FORCE_INLINE
    
    // getters ----------------------------------------------------------------------------------------------

    #define DEFAULT_GETTER(type, name)				        FORCE_INLINE_R type get_##name() { return m_##name; }

    #define DEFAULT_GETTER_P(type, name)				    FORCE_INLINE_R type* get_##name() { return m_##name.get(); }

    #define DEFAULT_GETTER_REF(type, name)			        FORCE_INLINE_R type& get_##name##_ref() { return m_##name; }

    #define DEFAULT_GETTER_C(type, name)			        FORCE_INLINE_R type get_##name() const { return m_##name; }

    #define DEFAULT_GETTER_CC(type, name)			        FORCE_INLINE_R const type& get_##name() const { return m_##name; }

    #define DEFAULT_GETTER_POINTER(type, name)		        FORCE_INLINE_R type* get_##name##_pointer() { return &m_##name; }

    #define DEFAULT_GETTERS(type, name)				        DEFAULT_GETTER(type, name)					                \
                                                            DEFAULT_GETTER_REF(type, name)					            \
                                                            DEFAULT_GETTER_POINTER(type, name)

    #define DEFAULT_GETTERS_C(type, name)			        DEFAULT_GETTER_C(type, name)			                    \
                                                            DEFAULT_GETTER_POINTER(type, name)

    #define GETTER(type, func_name, var_name)		        FORCE_INLINE_R type get_##func_name() { return var_name; }

    #define GETTER_C(type, func_name, var_name)		        FORCE_INLINE_R type get_##func_name() const { return var_name; }

    #define GETTER_CC(type, func_name, var_name)		    FORCE_INLINE_R const type& get_##func_name() const { return var_name; }

    // setters ----------------------------------------------------------------------------------------------

    #define DEFAULT_SETTER(type, name)				        FORCE_INLINE void set_##name(type name) { m_##name = name; }

    #define SETTER(type, func_name, var_name)               FORCE_INLINE void set_##func_name(type value) { var_name = value; }

    // both together ----------------------------------------------------------------------------------------

    #define DEFAULT_GETTER_SETTER(type, name)				DEFAULT_GETTER(type, name)				                    \
                                                            DEFAULT_SETTER(type, name)

    #define DEFAULT_GETTER_SETTER_C(type, name)				DEFAULT_GETTER_C(type, name)			                    \
                                                            DEFAULT_SETTER(type, name)

    #define DEFAULT_GETTER_SETTER_ALL(type, name)			DEFAULT_SETTER(type, name)				                    \
                                                            DEFAULT_GETTER(type, name)				                    \
                                                            DEFAULT_GETTER_POINTER(type, name)

    #define GETTER_SETTER(type, func_name, var_name)		GETTER(type, func_name, var_name)		                    \
                                                            SETTER(type, func_name, var_name)

    #define GETTER_SETTER_C(type, func_name, var_name)		GETTER_C(type, func_name, var_name)	                        \
                                                            SETTER(type, func_name, var_name)

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
