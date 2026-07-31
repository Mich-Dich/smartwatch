
#include "util/pch.hpp"
#include "spectrum_analyzer.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    // At file scope (outside the class) – only initialise the codec once
    static bool             s_audio_initialised = false;

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    void spectrum_analyzer_screen::create(lv_obj_t* screen) {

        lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

        const i32 w = lv_display_get_horizontal_resolution(lv_display_get_default());
        const i32 h = lv_display_get_vertical_resolution(lv_display_get_default());
        const size_t buf_size = LV_CANVAS_BUF_SIZE(w, h, LV_COLOR_FORMAT_RGB565, LV_DRAW_BUF_STRIDE_ALIGN);
        void* buf = malloc(buf_size);
        assert(buf);

        m_canvas = lv_canvas_create(screen);
        lv_obj_set_size(m_canvas, w, h);
        lv_obj_center(m_canvas);
        lv_canvas_set_buffer(m_canvas, buf, w, h, LV_COLOR_FORMAT_RGB565);
    }


    void spectrum_analyzer_screen::on_enter() {

        m_stop_requested = false;

        if (!s_audio_initialised) {                                                     // Audio setup (one‑time initialisation)
            esp_err_t ret = bsp_extra_codec_init();
            if (ret != ESP_OK) {
                ESP_LOGE("Spectrum", "Audio codec init failed");
                return;
            }
            s_audio_initialised = true;
        }

        m_canvas_timer = lv_timer_create(canvas_timer_cb, 33, this);                    // Canvas timer
        xTaskCreate(audio_fft_task, "audio_fft", 6 * 1024, this, 5, &m_audio_task);     // Audio task
    }


    void spectrum_analyzer_screen::on_exit() {

        m_stop_requested = true;                                                        // Signal the audio task to stop

        if (m_audio_task) {                                                             // Wait for the task to finish (up to 500 ms)
            vTaskDelay(pdMS_TO_TICKS(500));
            if (eTaskGetState(m_audio_task) != eDeleted) {
                vTaskDelete(m_audio_task);
            }
            m_audio_task = nullptr;
        }

        if (m_canvas_timer) {                                                           // Stop the canvas timer
            lv_timer_delete(m_canvas_timer);
            m_canvas_timer = nullptr;
        }
    }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

    void spectrum_analyzer_screen::canvas_timer_cb(lv_timer_t* timer) {

        auto* self = static_cast<spectrum_analyzer_screen*>(lv_timer_get_user_data(timer));
        lv_obj_t* canvas = self->m_canvas;
        const int canvas_w = lv_obj_get_width(canvas);
        const int canvas_h = lv_obj_get_height(canvas);
        const f32 stripe_width = static_cast<f32>(canvas_w) / STRIPE_COUNT;
        const int center_y = canvas_h / 2;
        lv_layer_t layer;
        lv_canvas_init_layer(canvas, &layer);
        lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
        
        constexpr int BAR_GAP_PX = 2;
        constexpr f32 HUE_STEP = 360.0f / STRIPE_COUNT;
        constexpr f32 BAR_DROP_RATE = 1.5f;
        for (size_t i = 0; i < STRIPE_COUNT; ++i) {

            const f32 db = display_spectrum[i];
            f32 norm = (db + 90.0f) / 90.0f;  // db_min = -90, db_max = 0
            norm = std::clamp(norm, 0.0f, 1.0f);
            norm = std::sqrt(norm);
            const i32 bar_height = static_cast<int>(norm * (canvas_h / 2.0f));

            // Peak hold
            if (peak[i] < bar_height)
                peak[i] = static_cast<f32>(bar_height);
            else {
                peak[i] -= BAR_DROP_RATE;
                if (peak[i] < 0.0f) peak[i] = 0.0f;
            }

            const u16 hue = static_cast<u16>(i * HUE_STEP);
            const lv_color_t color = lv_color_hsv_to_rgb(hue, 100, 100);

            lv_draw_rect_dsc_t rect_dsc;
            lv_draw_rect_dsc_init(&rect_dsc);
            rect_dsc.bg_color = color;
            rect_dsc.bg_opa   = LV_OPA_COVER;

            const i32 x_start = static_cast<int>(i * stripe_width + BAR_GAP_PX / 2.0f);
            const i32 x_end   = static_cast<int>((i + 1) * stripe_width - BAR_GAP_PX / 2.0f - 1);

            const lv_area_t bar_area{x_start, center_y - bar_height, x_end, center_y + bar_height};
            lv_draw_rect(&layer, &rect_dsc, &bar_area);

            // Peak markers
            const i32 peak_y_top = center_y - static_cast<int>(peak[i]) - 2;
            const i32 peak_y_bot = center_y + static_cast<int>(peak[i]);
            const lv_area_t ptop{x_start, peak_y_top, x_end, peak_y_top + 2};
            const lv_area_t pbot{x_start, peak_y_bot, x_end, peak_y_bot + 2};
            lv_draw_rect(&layer, &rect_dsc, &ptop);
            lv_draw_rect(&layer, &rect_dsc, &pbot);
        }

        lv_canvas_finish_layer(canvas, &layer);
    }


    void spectrum_analyzer_screen::audio_fft_task(void* param) {

        auto* self = static_cast<spectrum_analyzer_screen*>(param);
        
        esp_err_t ret = dsps_fft2r_init_fc32(nullptr, CONFIG_DSP_MAX_FFT_SIZE);         // DSP initialisation (unchanged)
        if (ret != ESP_OK) {
            ESP_LOGE("Spectrum", "FFT init failed: %d", ret);
            vTaskDelete(nullptr);
            return;
        }
        dsps_wind_hann_f32(wind.data(), N_SAMPLES);
        ESP_LOGI("Spectrum", "FFT and window initialized");

        // Audio codec was already initialised in on_enter()
        TickType_t last_wake_time = xTaskGetTickCount();
        size_t bytes_read = 0;
        while (!self->m_stop_requested) {

            ret = bsp_extra_i2s_read(raw_data.data(), N_SAMPLES * CHANNELS * sizeof(i16), &bytes_read, pdMS_TO_TICKS(100));  // small timeout
            if (ret != ESP_OK || bytes_read != N_SAMPLES * CHANNELS * sizeof(i16)) {
                if (!self->m_stop_requested) {
                    ESP_LOGW("Spectrum", "I2S read error: %d, bytes: %zu", ret, bytes_read);
                }
                continue;
            }

            // Convert to mono, scale
            for (size_t i = 0; i < N_SAMPLES; ++i) {
                const i16 left  = raw_data[i * CHANNELS];
                const i16 right = raw_data[i * CHANNELS + 1];
                audio_buffer[i] = (left + right) / (2.0f * 32768.0f);
            }

            dsps_mul_f32(audio_buffer.data(), wind.data(), audio_buffer.data(), N_SAMPLES, 1, 1, 1);
            for (size_t i = 0; i < N_SAMPLES; ++i) {
                fft_buffer[2 * i]     = audio_buffer[i];
                fft_buffer[2 * i + 1] = 0.0f;
            }

            dsps_fft2r_fc32(fft_buffer.data(), N_SAMPLES);
            dsps_bit_rev_fc32(fft_buffer.data(), N_SAMPLES);
            for (size_t i = 0; i < N_SAMPLES / 2; ++i) {
                const f32 real = fft_buffer[2 * i];
                const f32 imag = fft_buffer[2 * i + 1];
                const f32 magnitude = sqrtf(real * real + imag * imag);
                spectrum[i] = 20.0f * log10f(magnitude / (N_SAMPLES / 2.0f) + 1e-9f);
            }

            for (size_t i = 0; i < STRIPE_COUNT; ++i) {
                const size_t fft_idx = i * (N_SAMPLES / 2) / STRIPE_COUNT;
                display_spectrum[i] = std::clamp(spectrum[fft_idx], -90.0f, 0.0f);
            }

            vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1));
        }

        vTaskDelete(nullptr);                                                           // Clean exit – task deletes itself
    }

}
