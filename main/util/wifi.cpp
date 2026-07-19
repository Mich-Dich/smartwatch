
#include "util/pch.hpp"
#include "wifi.hpp"

#include "esp_wifi.h"



// FORWARD DECLARATIONS ================================================================================================

namespace APP::wifi {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    constexpr int                           WIFI_CONNECTED_BIT = BIT0;

    constexpr int                           SNTP_SYNC_TIMEOUT_SEC = 10;

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================
    
    static std::vector<Credential>          s_credentials;
    
    static EventGroupHandle_t               s_wifi_event_group = nullptr;
    
    static esp_timer_handle_t               s_periodic_timer = nullptr;
    
    static bool                             s_initialized = false;
    
    static esp_timer_handle_t               s_retry_timer = nullptr;
    
    static u32                              s_long_term_interval = 0;  // store for later

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================
    
    static void wifi_event_handler(void* arg, esp_event_base_t event_base, i32 event_id, void* event_data);
    
    static bool try_connect_to_saved_networks();
    
    static bool perform_sntp_sync();

    static void retry_sync_timer_cb(void* arg);

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    // Event handler 
    static void wifi_event_handler(void* arg, esp_event_base_t event_base, i32 event_id, void* event_data) {

        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }


    // Try to connect to one of the stored networks 
    static bool try_connect_to_saved_networks() {

        wifi_config_t wifi_config = {};
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

        for (const auto& cred : s_credentials) {
            esp_wifi_stop();   // ensure driver is idle

            memset(&wifi_config, 0, sizeof(wifi_config));
            strncpy((char*)wifi_config.sta.ssid, cred.ssid, sizeof(wifi_config.sta.ssid) - 1);
            strncpy((char*)wifi_config.sta.password, cred.password, sizeof(wifi_config.sta.password) - 1);

            ESP_LOGI("WiFiSync", "Trying %s ...", cred.ssid);
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

            ESP_ERROR_CHECK(esp_wifi_start());
            esp_err_t ret = esp_wifi_connect();
            if (ret != ESP_OK) {
                ESP_LOGW("WiFiSync", "esp_wifi_connect() failed: %s", esp_err_to_name(ret));
                continue;
            }

            // Wait for an IP (up to 15 seconds)
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                                                   pdFALSE, pdTRUE,
                                                   pdMS_TO_TICKS(15000));
            if (bits & WIFI_CONNECTED_BIT) {
                ESP_LOGI("WiFiSync", "Connected to %s", cred.ssid);
                return true;
            }

            ESP_LOGW("WiFiSync", "Failed to connect to %s", cred.ssid);
            esp_wifi_disconnect();
        }
        return false;
    }


    // Perform SNTP sync 
    static bool perform_sntp_sync() {
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_init();

        int retries = 0;
        while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED
               && retries < SNTP_SYNC_TIMEOUT_SEC * 10) {
            vTaskDelay(pdMS_TO_TICKS(100));
            retries++;
        }

        esp_sntp_stop();
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            ESP_LOGI("SNTP", "Time synchronised successfully");
            return true;
        }
        ESP_LOGW("SNTP", "Sync timeout - time may be inaccurate");
        return false;
    }


    static void retry_sync_timer_cb(void* arg) {

        if (sync_time()) {      // Try to sync; if successful, switch to long‑term interval

            stop_retry_sync();                              // Stop this retry timer
            start_periodic_sync(s_long_term_interval);      // Start the normal periodic sync with the long‑term interval
        }
    }

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    void init() {
        // NVS
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);

        // TCP/IP stack and event loop
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();

        // Wi‑Fi driver init (once)
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

        // Register event handlers
        esp_event_handler_instance_t instance_any_id, instance_got_ip;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr, &instance_got_ip));

        // Event group for connection management
        s_wifi_event_group = xEventGroupCreate();

        s_initialized = true;
    }


    void set_credentials(const std::vector<Credential>& creds) {
        s_credentials = creds;
    }


    void set_credentials(std::initializer_list<Credential> creds) {
        set_credentials(std::vector<Credential>(creds));
    }


    bool sync_time() {
        if (!s_initialized) {
            ESP_LOGE("WiFiSync", "Module not initialised - call init() first");
            return false;
        }
        if (!try_connect_to_saved_networks()) {
            ESP_LOGW("WiFiSync", "No known network found - will retry later");
            esp_wifi_stop();
            return false;
        }

        bool ok = perform_sntp_sync();

        esp_wifi_disconnect();
        esp_wifi_stop();
        ESP_LOGI("WiFiSync", "Wi‑Fi stopped (power save)");
        return ok;
    }


    void disconnect() {
        esp_wifi_disconnect();
        esp_wifi_stop();
    }


    static void periodic_sync_timer_cb(void* arg) {
        sync_time();
    }


    void start_periodic_sync(u32 interval_ms) {
        if (!s_initialized) {
            ESP_LOGE("WiFiSync", "Must call init() before starting periodic sync");
            return;
        }
        if (s_periodic_timer) {
            stop_periodic_sync();   // avoid duplicate timer
        }

        esp_timer_create_args_t args = {};
        args.callback = &periodic_sync_timer_cb;
        args.name = "wifi_time_sync";
        ESP_ERROR_CHECK(esp_timer_create(&args, &s_periodic_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(s_periodic_timer, interval_ms * 1000ULL));
    }


    void stop_periodic_sync() {
        if (s_periodic_timer) {
            esp_timer_stop(s_periodic_timer);
            esp_timer_delete(s_periodic_timer);
            s_periodic_timer = nullptr;
        }
    }


    void stop_retry_sync() {

        if (s_retry_timer) {

            esp_timer_stop(s_retry_timer);
            esp_timer_delete(s_retry_timer);
            s_retry_timer = nullptr;
        }
    }


    void start_retry_sync_until_success(u32 retry_interval_ms, u32 long_term_interval_ms) {

        if (!s_initialized) {
            ESP_LOGE("WiFiSync", "Must call init() first");
            return;
        }
        stop_retry_sync();                                      // Stop any existing retry timer and normal periodic sync
        stop_periodic_sync();

        s_long_term_interval = long_term_interval_ms;           // Store the long‑term interval for later

        esp_timer_create_args_t args = {};
        args.callback = &retry_sync_timer_cb;
        args.name = "wifi_retry_sync";
        ESP_ERROR_CHECK(esp_timer_create(&args, &s_retry_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(s_retry_timer, retry_interval_ms * 1000ULL));
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
