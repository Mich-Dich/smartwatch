#include "util/pch.hpp"
#include "home_assist_screen.hpp"

#include <esp_http_client.h>

#include "util/io/serializer_yaml.hpp"
#include "user_app.hpp"
#include "UI/util.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    static const char*                      TAG = "home_assist_screen";

    static constexpr const char*            SERVER_URL = "http://192.168.178.32:5000/data";
    
    static constexpr const char*            USERNAME = "admin";
    
    static constexpr const char*            PASSWORD = "J=e>LDG=?wKMXAGk$}.QenhIU6=mR-w?n8Mj=%[v8aZ[Hr5RvZ0kS*CZhqxh_Kg:7_";

    // MACROS ==========================================================================================================

    #define DEBUG_FETCH_DATA                0

    // STATIC VARIABLES ================================================================================================

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

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

        APP::UI::util::create_geometric_pattern_0(
            m_screen,
            lv_color_mix(highlight_color, lv_color_hex(0x000000), 80),
            lv_color_mix(support_color, lv_color_hex(0x000000), 80)
        );

        m_status_label = lv_label_create(m_screen);
        lv_label_set_text(m_status_label, "Remote Data");
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(m_status_label, &inconsolata_regular_26, 0);
        lv_obj_align(m_status_label, LV_ALIGN_TOP_MID, 0, 20);

        m_number_label = lv_label_create(m_screen);
        lv_obj_set_style_text_color(m_number_label, highlight_color, 0);
        lv_obj_set_style_text_font(m_number_label, &wildgrin_152, 0);
        lv_label_set_text(m_number_label, "--");
        lv_obj_align(m_number_label, LV_ALIGN_CENTER, 0, -20);

        lv_obj_t* btn_container = lv_obj_create(m_screen);
        lv_obj_set_size(btn_container, LV_PCT(90), LV_SIZE_CONTENT);
        lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_container, 0, 0);
        lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        m_refresh_btn = lv_btn_create(btn_container);
        lv_obj_set_size(m_refresh_btn, 120, 40);
        lv_obj_set_style_bg_color(m_refresh_btn, lv_color_hex(0x0066FF), 0);
        lv_obj_add_event_cb(m_refresh_btn, refresh_btn_cb, LV_EVENT_CLICKED, this);
        lv_obj_t* btn_label = lv_label_create(m_refresh_btn);
        lv_label_set_text(btn_label, LV_SYMBOL_REFRESH " Refresh");
        lv_obj_center(btn_label);

        m_fetch_timer = lv_timer_create(timer_cb, 5000, this);
        lv_timer_pause(m_fetch_timer);

        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "Remote screen initialised");
    }


    void home_assist_screen::show() {
        m_is_visible = true;
        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        lv_timer_resume(m_fetch_timer);
        fetch_data();
    }


    void home_assist_screen::hide() {
        m_is_visible = false;
        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        lv_timer_pause(m_fetch_timer);
    }


    void home_assist_screen::destroy() {
        if (m_fetch_timer) {
            lv_timer_del(m_fetch_timer);
            m_fetch_timer = nullptr;
        }
        if (m_screen) {
            lv_obj_del(m_screen);
            m_screen = nullptr;
        }
    }


    bool home_assist_screen::handle_event(lv_event_t* e) {
        return false;
    }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

    void home_assist_screen::fetch_data() {

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

        esp_http_client_set_username(client, USERNAME);
        esp_http_client_set_password(client, PASSWORD);
        esp_http_client_set_authtype(client, HTTP_AUTH_TYPE_BASIC);

        // Open connection and send request (write_len = 0 for GET)
        esp_err_t err = esp_http_client_open(client, 0);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Open failed: %s", esp_err_to_name(err));
            update_ui_error("Open error");
            esp_http_client_cleanup(client);
            return;
        }

        // Fetch headers and get content length
        int content_length = esp_http_client_fetch_headers(client);
        int status = esp_http_client_get_status_code(client);
        
        #if DEBUG_FETCH_DATA
            ESP_LOGI(TAG, "HTTP status: %d, content_length: %d", status, content_length);
        #endif

        if (status != 200) {
            // Read error body if any
            std::string error_body;
            char buf[64];
            int len;
            while ((len = esp_http_client_read(client, buf, sizeof(buf) - 1)) > 0) {
                buf[len] = '\0';
                error_body += buf;
            }
            ESP_LOGE(TAG, "Error response: %s", error_body.c_str());
            update_ui_error("Server error");
            esp_http_client_cleanup(client);
            return;
        }

        // Read the response body
        std::string response_body{};
        char buffer[128];
        int data_read;
        #if DEBUG_FETCH_DATA
            ESP_LOGI(TAG, "Starting to read response body...");
        #endif

        while ((data_read = esp_http_client_read(client, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[data_read] = '\0';
            response_body += buffer;
            #if DEBUG_FETCH_DATA
                ESP_LOGI(TAG, "Read chunk: %d bytes", data_read);
            #endif
        }

        if (data_read < 0) {

            ESP_LOGE(TAG, "Read error: %s", esp_err_to_name(data_read));
            update_ui_error("Read error");
            esp_http_client_cleanup(client);
            return;
        }

        #if DEBUG_FETCH_DATA
            ESP_LOGI(TAG, "Total bytes read: %d", (int)response_body.size());
        #endif

        esp_http_client_cleanup(client);
        if (response_body.empty()) {

            ESP_LOGE(TAG, "Response body is empty!");
            update_ui_error("Empty response");
            return;
        }

        #if DEBUG_FETCH_DATA
            ESP_LOGI(TAG, "Server message: [%s]", response_body.c_str());
        #endif

        // Parse YAML
        bool yaml_success = false;
        u32 number{};
        f32 timestamp{};
        APP::serializer::yaml(&response_body, "data", APP::serializer::option::load, &yaml_success)
            .entry(KEY_VALUE(number))
            .entry(KEY_VALUE(timestamp));

        if (!yaml_success) {

            ESP_LOGE(TAG, "YAML parse error");
            update_ui_error("Parse error");
            return;
        }

        update_ui(static_cast<int>(number), "OK");
    }


    void home_assist_screen::update_ui(int number, const char* status) {
        if (!m_is_visible) return;
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", number);
        lv_label_set_text(m_number_label, buf);
        lv_label_set_text(m_status_label, status);
    }


    void home_assist_screen::update_ui_error(const char* error) {
        if (!m_is_visible) return;
        lv_label_set_text(m_number_label, "?");
        lv_label_set_text(m_status_label, error);
    }


    void home_assist_screen::timer_cb(lv_timer_t* timer) {
        home_assist_screen* self = static_cast<home_assist_screen*>(timer->user_data);
        if (self && self->m_is_visible) {
            self->fetch_data();
        }
    }


    void home_assist_screen::refresh_btn_cb(lv_event_t* e) {
        home_assist_screen* self = static_cast<home_assist_screen*>(lv_event_get_user_data(e));
        if (self) {
            self->fetch_data();
        }
    }

}
