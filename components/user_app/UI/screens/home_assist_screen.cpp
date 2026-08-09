#include "util/pch.hpp"
#include "home_assist_screen.hpp"

#include <esp_http_client.h>

#include "util/io/serializer_yaml.hpp"
#include "util/home_assist_credentials.hpp"
#include "UI/util.hpp"
#include "user_app.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    static const char*                              TAG = "home_assist_screen";

    static constexpr const char*                    SERVER_URL = "http://192.168.178.32:5000/data";

    // MACROS ==========================================================================================================

    #define DEBUG_FETCH_DATA_FUNCTION               0

    // STATIC VARIABLES ================================================================================================

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    static bool is_wifi_connected();

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    static bool is_wifi_connected() {

        wifi_ap_record_t ap_info;
        return esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
    }

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    // CLASS IMPLEMENTATION ============================================================================================

    home_assist_screen::home_assist_screen() = default;


    home_assist_screen::~home_assist_screen() { destroy(); }

    // CLASS PUBLIC ====================================================================================================

    void home_assist_screen::init() {

        ESP_LOGI(TAG, "Initialising remote screen");

        m_screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(m_screen, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(m_screen, LV_OPA_COVER, 0);
        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);

        // Background pattern
        APP::UI::util::create_geometric_pattern_0(
            m_screen,
            lv_color_mix(highlight_color, lv_color_hex(0x000000), 80),
            lv_color_mix(support_color, lv_color_hex(0x000000), 80)
        );

        // ---- Title ----
        m_title_label = lv_label_create(m_screen);
        lv_label_set_text(m_title_label, "System Monitor");
        lv_obj_set_style_text_color(m_title_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(m_title_label, &inconsolata_regular_26, 0);
        lv_obj_align(m_title_label, LV_ALIGN_TOP_MID, 0, 10);

        // ---- Status ----
        m_status_label = lv_label_create(m_screen);
        lv_label_set_text(m_status_label, "Waiting...");
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_text_font(m_status_label, &inconsolata_regular_26, 0);
        lv_obj_align(m_status_label, LV_ALIGN_TOP_MID, 0, 45);

        // ---- Scrollable container for charts ----
        m_scroll_container = lv_obj_create(m_screen);
        lv_obj_set_size(m_scroll_container, LV_PCT(95), LV_PCT(80));
        lv_obj_align(m_scroll_container, LV_ALIGN_TOP_MID, 0, 80);
        lv_obj_set_style_bg_opa(m_scroll_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(m_scroll_container, 0, 0);
        lv_obj_set_style_pad_all(m_scroll_container, 5, 0);
        lv_obj_set_flex_flow(m_scroll_container, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(m_scroll_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(m_scroll_container, 8, 0);
        lv_obj_add_flag(m_scroll_container, LV_OBJ_FLAG_SCROLLABLE);

        // Helper to create a chart with a label
        auto create_chart_row = [this](const char* label_text, lv_color_t color) -> lv_obj_t* {

            // Container for label + chart
            lv_obj_t* row = lv_obj_create(m_scroll_container);
            lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(row, 0, 0);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_row(row, 10, 0);

            // Label
            lv_obj_t* label = lv_label_create(row);
            lv_label_set_text(label, label_text);
            lv_obj_set_style_text_color(label, lv_color_hex(0xCCCCCC), 0);
            lv_obj_set_style_text_font(label, &inconsolata_regular_26, 0);

            // Chart
            lv_obj_t* chart = lv_chart_create(row);
            lv_obj_set_size(chart, LV_PCT(100), 100);
            lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(chart, HISTORY_SIZE);
            lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100); // will be adjusted per metric later
            lv_obj_set_style_bg_color(chart, lv_color_hex(0x1A1A1A), 0);
            lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_color(chart, lv_color_hex(0x333333), 0);
            lv_obj_set_style_border_width(chart, 0, 0);
            lv_chart_set_div_line_count(chart, 3, 4);

            // Series
            lv_chart_series_t* series = lv_chart_add_series(chart, color, LV_CHART_AXIS_PRIMARY_Y);
            // Initialize with zeros
            lv_chart_set_all_value(chart, series, 0);

            // Store pointer to series in user_data if needed, but we'll use class members.
            // Return chart object for later reference.
            return chart;
        };

        // Create charts – store chart objects and series pointers
        m_chart_cpu_temp = create_chart_row("CPU Temperature", lv_color_hex(0xFF6B6B));
        m_series_cpu_temp = lv_chart_get_series_next(m_chart_cpu_temp, nullptr);

        m_chart_cpu_usage = create_chart_row("CPU Usage", lv_color_hex(0x4ECDC4));
        m_series_cpu_usage = lv_chart_get_series_next(m_chart_cpu_usage, nullptr);

        m_chart_memory = create_chart_row("Memory Usage", lv_color_hex(0x5F27CD));
        m_series_memory = lv_chart_get_series_next(m_chart_memory, nullptr);

        // ---- Refresh button ----
        lv_obj_t* btn_container = lv_obj_create(m_screen);
        lv_obj_set_size(btn_container, LV_PCT(90), LV_SIZE_CONTENT);
        lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_container, 0, 0);
        lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // ---- Timer ----
        m_fetch_timer = lv_timer_create(timer_cb, 1000, this);
        lv_timer_pause(m_fetch_timer);

        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "Remote screen initialised");
    }


    void home_assist_screen::show() {

        m_is_visible = true;
        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        lv_timer_resume(m_fetch_timer);
        // fetch_data();
    }


    void home_assist_screen::hide() {

        m_is_visible = false;
        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        lv_timer_pause(m_fetch_timer);
    }


    void home_assist_screen::destroy() {

        if (m_fetch_timer)
            lv_timer_del(m_fetch_timer);

        if (m_screen)
            lv_obj_del(m_screen);


        m_fetch_timer = nullptr;
        m_screen = nullptr;
    }


    bool home_assist_screen::handle_event(lv_event_t* e)        { return false; }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

    bool home_assist_screen::read_yaml_data(std::string* yaml_data) {
        
        bool yaml_success = false;
        APP::serializer::yaml(yaml_data, "system", APP::serializer::option::load, &yaml_success)
            .sub_section("cpu", [&](APP::serializer::yaml& cpu_section) {
                cpu_section.entry(KEY_VALUE(m_server_pc_data.cpu_data.temperature_celsius))
                    .entry(KEY_VALUE(m_server_pc_data.cpu_data.usage_percent));
            })
            
            .sub_section("gpu", [&](APP::serializer::yaml& cpu_section) {
                cpu_section.entry(KEY_VALUE(m_server_pc_data.gpu_data.temperature_celsius));
            })
            
            .sub_section("memory", [&](APP::serializer::yaml& cpu_section) {
                cpu_section.entry(KEY_VALUE(m_server_pc_data.memory_data.percent))
                    .entry(KEY_VALUE(m_server_pc_data.memory_data.total_gb))
                    .entry(KEY_VALUE(m_server_pc_data.memory_data.used_gb));
            })
        .entry(KEY_VALUE(m_server_pc_data.uptime_seconds));

        return yaml_success;
    }


    void home_assist_screen::fetch_data() {

        if (!is_wifi_connected()) {
            // Update UI only once every 5 seconds to avoid flicker
            static int64_t last_wifi_error_time = 0;
            int64_t now = esp_timer_get_time();
            if (now - last_wifi_error_time > 5 * 1000 * 1000) { // 5 seconds
                update_ui_error("No Wi-Fi");
                last_wifi_error_time = now;
            }
            return;
        }

        esp_http_client_config_t config = {};
        config.url = SERVER_URL;
        config.method = HTTP_METHOD_GET;
        config.timeout_ms = 5000;
        config.keep_alive_enable = true;
        config.buffer_size = 2048;
        config.buffer_size_tx = 512;

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (!client) {
            ESP_LOGE(TAG, "Failed to init HTTP client");
            update_ui_error("Init error");
            return;
        }

        esp_http_client_set_username(client, credentials::SERVER_USERNAME);
        esp_http_client_set_password(client, credentials::SERVER_PASSWORD);
        esp_http_client_set_authtype(client, HTTP_AUTH_TYPE_BASIC);

        esp_err_t err = esp_http_client_open(client, 0);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Open failed: %s", esp_err_to_name(err));
            update_ui_error("Open error");
            esp_http_client_cleanup(client);
            return;
        }

        // -------- FIX: always fetch headers first --------
        int content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0) {
            ESP_LOGE(TAG, "Failed to fetch headers: %d", content_length);
            update_ui_error("Header error");
            esp_http_client_cleanup(client);
            return;
        }

        int status = esp_http_client_get_status_code(client);

        #if DEBUG_FETCH_DATA_FUNCTION
            ESP_LOGI(TAG, "HTTP status: %d, content_length: %d", status, content_length);
        #endif

        if (status != 200) {
            std::string error_body;
            char buf[64];
            int len;
            while ((len = esp_http_client_read(client, buf, sizeof(buf) - 1)) > 0) {
                buf[len] = '\0';
                error_body += buf;
            }
            ESP_LOGE(TAG, "Error response: [%d] [%s]", status, error_body.c_str());
            update_ui_error("Server error");
            esp_http_client_cleanup(client);
            return;
        }

        // Read the response body
        std::string response_body{};
        char buffer[128];
        int data_read;
        #if DEBUG_FETCH_DATA_FUNCTION
            ESP_LOGI(TAG, "Starting to read response body...");
        #endif

        while ((data_read = esp_http_client_read(client, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[data_read] = '\0';
            response_body += buffer;
            #if DEBUG_FETCH_DATA_FUNCTION
                ESP_LOGI(TAG, "Read chunk: %d bytes", data_read);
            #endif
        }

        if (data_read < 0) {
            ESP_LOGE(TAG, "Read error: %s", esp_err_to_name(data_read));
            update_ui_error("Read error");
            esp_http_client_cleanup(client);
            return;
        }

        #if DEBUG_FETCH_DATA_FUNCTION
            ESP_LOGI(TAG, "Total bytes read: %d", (int)response_body.size());
        #endif

        esp_http_client_cleanup(client);

        if (response_body.empty()) {
            ESP_LOGE(TAG, "Response body is empty!");
            update_ui_error("Empty response");
            return;
        }

        #if DEBUG_FETCH_DATA_FUNCTION
            ESP_LOGI(TAG, "Server message: [%s]", response_body.c_str());
        #endif

        if (!read_yaml_data(&response_body)) {
            ESP_LOGE(TAG, "YAML parse error");
            update_ui_error("Parse error");
            return;
        }

        update_ui(m_server_pc_data, "OK");
    }


    void home_assist_screen::update_ui(const server_pc_data& data, const char* status) {

        if (!m_is_visible)
            return;

        // Update charts with new values (they will shift history automatically)
        lv_chart_set_next_value(m_chart_cpu_temp, m_series_cpu_temp, (int)(data.cpu_data.temperature_celsius));
        lv_chart_set_next_value(m_chart_cpu_usage, m_series_cpu_usage, (int)(data.cpu_data.usage_percent));
        lv_chart_set_next_value(m_chart_memory, m_series_memory, (int)(data.memory_data.percent));

        // Update status label with timestamp
        time_t now;
        time(&now);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);
        char buf[64];
        snprintf(buf, sizeof(buf), "Last update: %s", time_str);
        lv_label_set_text(m_status_label, buf);
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0x88FF88), 0);
    }


    void home_assist_screen::update_ui_error(const char* error) {
        if (!m_is_visible)
            return;
        lv_label_set_text(m_status_label, error);
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xFF6666), 0);
        // Optionally clear charts or leave as is
    }


    std::string home_assist_screen::format_uptime(f32 seconds) {

        int total_sec = static_cast<int>(seconds);
        int days = total_sec / 86400;
        int hours = (total_sec % 86400) / 3600;
        int mins = (total_sec % 3600) / 60;
        int secs = total_sec % 60;

        std::ostringstream oss;
        if (days > 0)      oss << days << "d ";
        if (hours > 0)     oss << hours << "h ";
        if (mins > 0)      oss << mins << "m ";
        oss << secs << "s";
        return oss.str();
    }


    void home_assist_screen::timer_cb(lv_timer_t* timer) {

        home_assist_screen* self = static_cast<home_assist_screen*>(timer->user_data);
        if (self && self->m_is_visible)
            self->fetch_data();
    }

}
