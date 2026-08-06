
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



// FORWARD DECLARATIONS =====================================================================================

extern lv_color_t highlight_color;

extern lv_color_t support_color;

// CONSTANTS ================================================================================================

// MACROS ===================================================================================================

// TYPES ====================================================================================================

// STATIC VARIABLES =========================================================================================

// FUNCTION DECLARATION =====================================================================================

#ifdef __cplusplus
    extern "C" {
#endif

    inline lv_color_t get_highlight_color()             { return highlight_color; }

    inline void set_highlight_color(lv_color_t c)       { highlight_color = c; }

    inline lv_color_t get_support_color()               { return support_color; }

    inline void set_support_color(lv_color_t c)         { support_color = c; }

    void application_begin(void);

#ifdef __cplusplus
    }
#endif

// TEMPLATE DECLARATION =====================================================================================

// CLASS DECLARATION ========================================================================================
