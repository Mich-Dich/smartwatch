
#include "util/pch.hpp"
#include "spec_analyser.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::SA {

    // CONSTANTS =======================================================================================================

    constexpr size_t                                        N_SAMPLES    = 1024;
    constexpr size_t                                        SAMPLE_RATE  = 16000;
    constexpr size_t                                        CHANNELS     = 2;
    constexpr size_t                                        STRIPE_COUNT = 64;
    constexpr int                                           BAR_GAP_PX = 2;
    constexpr f32                                           HUE_STEP = 360.0f / STRIPE_COUNT;
    constexpr f32                                           BAR_DROP_RATE = 0.8f;

    // MACROS ==========================================================================================================

    // TYPES ===========================================================================================================

    // STATIC VARIABLES ================================================================================================
    
    // ---- Static buffers (aligned for DSP) ----
    alignas(16) std::array<i16, N_SAMPLES * CHANNELS>       raw_data{};
    alignas(16) std::array<f32, N_SAMPLES>                  audio_buffer{};
    alignas(16) std::array<f32, N_SAMPLES>                  wind{};
    alignas(16) std::array<f32, N_SAMPLES * 2>              fft_buffer{};
    alignas(16) std::array<f32, N_SAMPLES / 2>              spectrum{};
    std::array<f32, STRIPE_COUNT>                           display_spectrum{};
    std::array<f32, STRIPE_COUNT>                           peak{};

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    // ---- LVGL canvas update callback ----
    void timer_cb(lv_timer_t* timer) {

        auto* canvas = static_cast<lv_obj_t*>(lv_timer_get_user_data(timer));

        // Use the actual canvas size
        static const u16 canvas_w = lv_obj_get_width(canvas);
        static const u16 canvas_h = lv_obj_get_height(canvas);
        static const f32 stripe_width = static_cast<f32>(canvas_w) / static_cast<f32>(STRIPE_COUNT);
        static const u16 center_y = canvas_h / 2;

        lv_layer_t layer;
        lv_canvas_init_layer(canvas, &layer);
        lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

        for (size_t i = 0; i < STRIPE_COUNT; ++i) {
            
            const f32 db = display_spectrum[i];
            constexpr f32 db_min = -90.0f;
            constexpr f32 db_max =   0.0f;

            f32 norm = (db - db_min) / (db_max - db_min);
            norm = std::clamp(norm, 0.0f, 1.0f);
            norm = std::sqrt(norm);

            int bar_height = static_cast<int>(norm * (canvas_h / 2.0f));

            // Update peak hold
            if (peak[i] < bar_height) {
                peak[i] = static_cast<f32>(bar_height);
            
            } else {

                peak[i] -= BAR_DROP_RATE;
                if (peak[i] < 0.0f) 
                    peak[i] = 0.0f;
            }

            // Color: hue from 0 to 70 (warm colours)
            const u16 hue = static_cast<u16>(i * HUE_STEP);
            const lv_color_t color = lv_color_hsv_to_rgb(hue, 100, 100);

            lv_draw_rect_dsc_t rect_dsc;
            lv_draw_rect_dsc_init(&rect_dsc);
            rect_dsc.bg_color = color;
            rect_dsc.bg_opa   = LV_OPA_COVER;

            const i32 x_start = static_cast<i32>(static_cast<u32>(static_cast<f32>(i) * stripe_width) + BAR_GAP_PX / 2.0f);
            const i32 x_end   = static_cast<i32>(static_cast<u32>(static_cast<f32>(i + 1) * stripe_width) - BAR_GAP_PX / 2.0f - 1);

            // Main bar
            lv_area_t bar_area{
                .x1 = x_start,
                .y1 = center_y - bar_height,
                .x2 = x_end,
                .y2 = center_y + bar_height
            };
            lv_draw_rect(&layer, &rect_dsc, &bar_area);

            // Peak markers (top and bottom)
            const i32 peak_y_top = center_y - static_cast<i32>(peak[i]) - 2;
            const i32 peak_y_bot = center_y + static_cast<i32>(peak[i]);

            lv_area_t particle_top{
                .x1 = x_start,
                .y1 = peak_y_top,
                .x2 = x_end,
                .y2 = peak_y_top + 2
            };
            lv_draw_rect(&layer, &rect_dsc, &particle_top);

            lv_area_t particle_bot{
                .x1 = x_start,
                .y1 = peak_y_bot,
                .x2 = x_end,
                .y2 = peak_y_bot + 2
            };
            lv_draw_rect(&layer, &rect_dsc, &particle_bot);
        }

        lv_canvas_finish_layer(canvas, &layer);
    }


    // ---- Audio processing task (must have C linkage) ----
    extern "C" void audio_fft_task(void* pv_parameters) {

        esp_err_t ret = dsps_fft2r_init_fc32(nullptr, CONFIG_DSP_MAX_FFT_SIZE);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "FFT init failed: %d", ret);
            vTaskDelete(nullptr);
            return;
        }

        dsps_wind_hann_f32(wind.data(), N_SAMPLES);
        ESP_LOGI(TAG, "FFT and window initialized");

        if (bsp_extra_codec_init() != ESP_OK) {
            ESP_LOGE(TAG, "Audio codec init failed");
            vTaskDelete(nullptr);
            return;
        }

        TickType_t last_wake_time = xTaskGetTickCount();
        size_t bytes_read;

        while (true) {

            ret = bsp_extra_i2s_read(raw_data.data(), N_SAMPLES * CHANNELS * sizeof(i16), &bytes_read, portMAX_DELAY);
            if (ret != ESP_OK || bytes_read != N_SAMPLES * CHANNELS * sizeof(i16)) {
                ESP_LOGW(TAG, "I2S read error: %d, bytes: %zu", ret, bytes_read);
                continue;
            }

            // Convert stereo to mono, scale to [-1,1]
            for (size_t i = 0; i < N_SAMPLES; ++i) {
                const i16 left  = raw_data[i * CHANNELS];
                const i16 right = raw_data[i * CHANNELS + 1];
                audio_buffer[i] = (left + right) / (2.0f * 32768.0f);
            }

            // Apply Hann window
            dsps_mul_f32(audio_buffer.data(), wind.data(), audio_buffer.data(), N_SAMPLES, 1, 1, 1);

            // Prepare complex FFT buffer
            for (size_t i = 0; i < N_SAMPLES; ++i) {
                fft_buffer[2 * i]     = audio_buffer[i];
                fft_buffer[2 * i + 1] = 0.0f;
            }

            // FFT
            dsps_fft2r_fc32(fft_buffer.data(), N_SAMPLES);
            dsps_bit_rev_fc32(fft_buffer.data(), N_SAMPLES);

            // Compute magnitude spectrum in dB
            for (size_t i = 0; i < N_SAMPLES / 2; ++i) {
                const f32 real = fft_buffer[2 * i];
                const f32 imag = fft_buffer[2 * i + 1];
                const f32 magnitude = std::sqrt(real * real + imag * imag);
                spectrum[i] = 20.0f * std::log10(magnitude / (N_SAMPLES / 2.0f) + 1e-9f);
            }

            // Average into stripes
            for (size_t i = 0; i < STRIPE_COUNT; ++i) {
                const size_t fft_idx = i * (N_SAMPLES / 2) / STRIPE_COUNT;
                display_spectrum[i] = std::clamp(spectrum[fft_idx], -90.0f, 0.0f);
            }

            vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1));
        }
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
