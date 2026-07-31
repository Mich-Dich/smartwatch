
#pragma once

#include "UI/screen.hpp"



// FORWARD DECLARATIONS =====================================================================================

namespace APP::UI {

    // CONSTANTS ============================================================================================

    // MACROS ===============================================================================================

    // TYPES ================================================================================================

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================

    // @brief   A full‑screen real‑time audio spectrum analyzer.
    // 
    // Uses the ESP‑DSP library to perform an FFT on incoming I2S audio
    // and draws a mirrored bar spectrum on an LVGL canvas.
    // 
    // Audio hardware is initialised once and stays alive for the whole
    // runtime – the screen only starts/stops the processing task and the
    // LVGL refresh timer.
    class spectrum_analyzer_screen : public screen {
    public:

        SCREEN_NAME("Spectrum")


        // @brief Called once when the screen is first added to the manager.
        // Allocates a full‑screen canvas and attaches a draw buffer to it.
        // @param screen LVGL screen object that will contain the canvas.
        void create(lv_obj_t* screen) override;


        // @brief Called every time the screen becomes active.
        // Starts the canvas refresh timer and the audio processing task.
        // If this is the very first visit, initialises the audio codec / I2S.
        void on_enter() override;
        

        // @brief Called when the screen is about to be hidden.
        // Signals the audio task to stop, deletes the timer, and waits for
        // the task to exit cleanly. Audio hardware remains initialised.
        void on_exit() override;

    private:

        //
        static void audio_task_func(void* param);


        // @brief LVGL timer callback – redraws the spectrum bars on the canvas.
        // Reads the current frequency data from the global display_spectrum array
        // and draws vertical bars with peak hold.
        // @param timer The LVGL timer that fired.
        static void canvas_timer_cb(lv_timer_t* timer);


        // @brief FreeRTOS task – continuously reads audio, runs FFT, and updates
        // the display_spectrum array.
        // Runs as long as m_stop_requested is false.
        // @param param Pointer to the spectrum_analyzer_screen instance.
        static void audio_fft_task(void* param);


        static constexpr size_t                                             N_SAMPLES = 1024;           // Number of audio samples per FFT block.
        static constexpr size_t                                             SAMPLE_RATE = 16000;        // Sample rate of the audio input (16 kHz).
        static constexpr size_t                                             CHANNELS = 2;               // Number of audio channels (2 = stereo).
        static constexpr size_t                                             STRIPE_COUNT = 64;          // Number of frequency bins (bars) displayed.

        // Static buffers – shared between the timer and the audio task
        alignas(16) static inline std::array<i16, N_SAMPLES * CHANNELS> raw_data{};                 // Raw stereo samples from I2S.
        alignas(16) static inline std::array<float, N_SAMPLES>              audio_buffer{};             // Mono, scaled audio for FFT.
        alignas(16) static inline std::array<float, N_SAMPLES>              wind{};                     // Hann window coefficients.
        alignas(16) static inline std::array<float, N_SAMPLES * 2>          fft_buffer{};               // Complex FFT buffer (real/imag interleaved).
        alignas(16) static inline std::array<float, N_SAMPLES / 2>          spectrum{};                 // Magnitude spectrum (in dB).
        static inline std::array<float, STRIPE_COUNT>                       display_spectrum{};         // Averaged spectrum for drawing.
        static inline std::array<float, STRIPE_COUNT>                       peak{};                     // Peak hold values for each bar.

        // Per‑instance members
        TaskHandle_t                                                        m_audio_task = nullptr;     // Handle of the audio processing task.
        lv_timer_t*                                                         m_canvas_timer = nullptr;   // LVGL timer that drives the canvas updates.
        lv_obj_t*                                                           m_canvas = nullptr;         // The full‑screen LVGL canvas object.
        std::atomic<bool>                                                   m_stop_requested{false};    // Flag to signal the audio task to exit.
    };

}
