
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

    class home_assist_screen : public screen {
    public:

        home_assist_screen();
        ~home_assist_screen() override;

        void init() override;

        void show() override;

        void hide() override;

        void destroy() override;

        lv_obj_t* get_root() override { return m_screen; }

        bool handle_event(lv_event_t* e) override;

    private:

        struct cpu {
            f32                     temperature_celsius{};
            f32                     usage_percent{};
        };

        struct gpu {
            f32                     temperature_celsius{};
        };

        struct memory {
            f32                     percent{};
            f32                     total_gb{};
            f32                     used_gb{};
        };

        struct server_pc_data {
            cpu                     cpu_data{};
            gpu                     gpu_data{};
            memory                  memory_data{};
            f32                     uptime_seconds{};
        };

        // @brief Number of data points kept in the chart history.
        static constexpr int HISTORY_SIZE = 10;


        // @brief Parse YAML response from the server into m_server_pc_data.
        // @param yaml_data  Pointer to the YAML string.
        // @return           True if parsing succeeded.
        bool read_yaml_data(std::string* yaml_data);


        // @brief Perform the HTTP GET request to the server and update UI.
        void fetch_data();


        // @brief Update UI with new data and a status message.
        // @param data    The parsed server data.
        // @param status  Status string (e.g., "OK").
        void update_ui(const server_pc_data& data, const char* status);


        // @brief Show an error message in the UI.
        // @param error  Error description.
        void update_ui_error(const char* error);


        // @brief Format uptime seconds into a human‑readable string.
        // @param seconds  Uptime in seconds.
        // @return         Formatted string (e.g., "1d 2h 3m 45s").
        static std::string format_uptime(f32 seconds);


        // @brief LVGL timer callback for periodic data fetching.
        static void timer_cb(lv_timer_t* timer);


        // @brief LVGL event callback for the refresh button.
        static void refresh_btn_cb(lv_event_t* e);


        // UI elements
        lv_obj_t*                   m_screen = nullptr;
        lv_obj_t*                   m_title_label = nullptr;
        lv_obj_t*                   m_status_label = nullptr;
        lv_obj_t*                   m_scroll_container = nullptr;   // holds all charts
        lv_timer_t*                 m_fetch_timer = nullptr;
        bool                        m_is_visible = false;
        server_pc_data              m_server_pc_data{};

        // Chart objects
        lv_obj_t*                   m_chart_cpu_temp = nullptr;
        lv_chart_series_t*          m_series_cpu_temp = nullptr;
        lv_obj_t*                   m_chart_cpu_usage = nullptr;
        lv_chart_series_t*          m_series_cpu_usage = nullptr;
        lv_obj_t*                   m_chart_memory = nullptr;
        lv_chart_series_t*          m_series_memory = nullptr;
    };

}
