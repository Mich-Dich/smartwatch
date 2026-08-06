#include "util/pch.hpp"
#include "wifi_screen.hpp"

#include "user_app.hpp"
#include "UI/util.hpp"
#include "UI/screens/main_screen.hpp" 

#include <esp_sntp.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <lwip/inet.h>



// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    static const char*                              TAG = "wifi_screen";

    static bool                                     s_wifi_initialized = false;

    // Pre‑defined known networks (SSID + password)
    std::vector<wifi_screen::known_network>         wifi_screen::m_known_networks = {
        {"Happy",                   "Kerstin321!"},
        {"FRITZ!Repeater 3000",     "frosch#5"}
    };

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    // CLASS IMPLEMENTATION ============================================================================================

    wifi_screen::wifi_screen() = default;


    wifi_screen::~wifi_screen() { destroy(); }


    // CLASS PUBLIC ====================================================================================================

    void wifi_screen::init() {

        ESP_LOGI(TAG, "Initialising WiFi screen");

        m_screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(m_screen, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(m_screen, LV_OPA_COVER, 0);
        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);

        // Geometric background (same as bluetooth)
        APP::UI::util::create_geometric_pattern_0(
            m_screen,
            lv_color_mix(highlight_color, lv_color_hex(0x000000), 80),
            lv_color_mix(support_color, lv_color_hex(0x000000), 80)
        );

        // Status label
        m_status_label = lv_label_create(m_screen);
        lv_label_set_text(m_status_label, "WiFi networks");
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(m_status_label, &inconsolata_regular_26, 0);
        lv_obj_align(m_status_label, LV_ALIGN_TOP_MID, 0, 20);

        // List container – transparent
        m_list = lv_list_create(m_screen);
        lv_obj_set_size(m_list, LV_PCT(90), LV_PCT(70));
        lv_obj_align(m_list, LV_ALIGN_TOP_MID, 0, 70);
        lv_obj_set_style_bg_opa(m_list, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(m_list, 0, 0);
        lv_obj_set_style_pad_row(m_list, 4, 0);
        lv_obj_add_flag(m_list, LV_OBJ_FLAG_EVENT_BUBBLE);

        // Bottom toolbar with Scan button
        lv_obj_t* btn_container = lv_obj_create(m_screen);
        lv_obj_set_size(btn_container, LV_PCT(90), LV_SIZE_CONTENT);
        lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn_container, 0, 0);
        lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Scan button
        m_scan_btn = lv_btn_create(btn_container);
        lv_obj_set_size(m_scan_btn, 100, 40);
        lv_obj_set_style_bg_color(m_scan_btn, lv_color_hex(0x0066FF), 0);
        lv_obj_add_event_cb(m_scan_btn, scan_btn_event_cb, LV_EVENT_CLICKED, this);
        lv_obj_add_flag(m_scan_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_t* scan_label = lv_label_create(m_scan_btn);
        lv_label_set_text(scan_label, "Scan");
        lv_obj_center(scan_label);

        // Floating Connect button (hidden initially)
        m_connect_btn = lv_btn_create(m_screen);
        lv_obj_set_size(m_connect_btn, 40, 36);
        lv_obj_set_style_bg_color(m_connect_btn, lv_color_hex(0x0066FF), 0);
        lv_obj_set_style_radius(m_connect_btn, 4, 0);
        lv_obj_add_event_cb(m_connect_btn, connect_btn_event_cb, LV_EVENT_CLICKED, this);
        lv_obj_add_flag(m_connect_btn, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t* connect_label = lv_label_create(m_connect_btn);
        lv_label_set_text(connect_label, LV_SYMBOL_WIFI);
        lv_obj_set_style_text_color(connect_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(connect_label);

        // Register WiFi event handlers
        esp_event_handler_instance_t instance_any;
        esp_event_handler_instance_t instance_ip;
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, this, &instance_any);
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_event_handler, this, &instance_ip);

        // Queue for UI updates
        m_ui_queue = xQueueCreate(5, sizeof(ui_update_msg));

        // Timer to process UI queue every 100 ms
        lv_timer_create([](lv_timer_t* timer) {
            wifi_screen* self = static_cast<wifi_screen*>(timer->user_data);
            self->process_ui_queue();
            self->check_ntp_sync();
        }, 100, this);

        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "WiFi screen initialised");
    }


    void wifi_screen::show() {

        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        start_scan();   // auto‑scan when shown
    }


    void wifi_screen::hide() {

        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        stop_wifi();
    }


    void wifi_screen::destroy() {

        stop_wifi();
        if (m_screen) {
            lv_obj_del(m_screen);
            m_screen = nullptr;
        }
        m_networks.clear();
    }


    bool wifi_screen::handle_event(lv_event_t* e) {

        // Not used – handled via static callbacks
        return false;
    }


    // CLASS PRIVATE ===================================================================================================

    void wifi_screen::start_scan() {

        if (m_is_scanning)
            return;

        if (!s_wifi_initialized) {
            // Deinit BLE to free resources (if needed)
            ble_scan_Deinit();
            espwifi_Init();
            esp_wifi_set_mode(WIFI_MODE_STA);
            s_wifi_initialized = true;
        }

        m_is_scanning = true;
        update_status("Scanning...");
        lv_obj_clean(m_list);
        m_networks.clear();
        clear_selection();

        esp_wifi_scan_start(nullptr, true);   // blocking scan

        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);
        ESP_LOGI(TAG, "Found %d APs", ap_count);

        if (ap_count == 0) {
            update_status("No networks found");
            m_is_scanning = false;
            return;
        }

        std::vector<wifi_ap_record_t> ap_records(ap_count);
        esp_wifi_scan_get_ap_records(&ap_count, ap_records.data());

        for (const auto& rec : ap_records) {
            std::string ssid_str = reinterpret_cast<const char*>(rec.ssid);
            // Trim whitespace
            ssid_str.erase(0, ssid_str.find_first_not_of(" \t\n\r\f\v"));
            ssid_str.erase(ssid_str.find_last_not_of(" \t\n\r\f\v") + 1);

            if (ssid_str.empty()) {
                ESP_LOGD(TAG, "Skipping empty SSID (hidden network)");
                continue;
            }

            m_networks.push_back(ssid_str);
            ESP_LOGI(TAG, "    Added network: %s", ssid_str.c_str());
        }

        populate_list();
        update_status("Scan complete");
        m_is_scanning = false;
    }


    void wifi_screen::populate_list() {

        lv_obj_clean(m_list);

        for (size_t i = 0; i < m_networks.size(); ++i) {
            const std::string& ssid = m_networks[i];

            lv_obj_t* btn = lv_list_add_btn(m_list, nullptr, ssid.c_str());
            lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x000000), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(btn, 1, 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x333333), 0);
            lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_BOTTOM, 0);
            lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);

            // Store index as user data
            lv_obj_set_user_data(btn, (void*)i);

            // Click event to select this network
            lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                lv_obj_t* btn = lv_event_get_target(e);
                wifi_screen* self = (wifi_screen*)lv_event_get_user_data(e);
                size_t idx = (size_t)lv_obj_get_user_data(btn);
                self->clear_selection();

                // Highlight selected item
                lv_obj_set_style_bg_opa(btn, LV_OPA_50, 0);
                lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), 0);
                self->m_selected_item = btn;
                self->m_selected_index = idx;
                self->m_device_selected = true;

                // Reparent connect button to the list item
                lv_obj_set_parent(self->m_connect_btn, btn);
                lv_obj_align(self->m_connect_btn, LV_ALIGN_RIGHT_MID, 10, 0);
                lv_obj_clear_flag(self->m_connect_btn, LV_OBJ_FLAG_HIDDEN);
                lv_obj_move_foreground(self->m_connect_btn);

                ESP_LOGI(TAG, "Selected network: %s", self->m_networks[idx].c_str());
            }, LV_EVENT_CLICKED, this);
        }
    }


    void wifi_screen::clear_selection() {

        if (m_selected_item) {
            lv_obj_set_style_bg_opa(m_selected_item, LV_OPA_TRANSP, 0);
            lv_obj_set_style_bg_color(m_selected_item, lv_color_hex(0x000000), 0);
            m_selected_item = nullptr;
        }
        m_device_selected = false;
        m_selected_index = 0;

        // Hide and reparent connect button to screen
        if (m_connect_btn) {
            lv_obj_set_parent(m_connect_btn, m_screen);
            lv_obj_add_flag(m_connect_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }


    void wifi_screen::on_connect_clicked() {

        if (!m_device_selected || m_selected_index >= m_networks.size())
            return;

        const std::string& ssid = m_networks[m_selected_index];

        // Find password in static list
        std::string password;
        bool found = false;
        for (const auto& known : m_known_networks) {
            if (known.ssid == ssid) {
                password = known.password;
                found = true;
                break;
            }
        }

        if (!found) {
            update_status("Unknown passwd:" + ssid);
            return;
        }

        // Store current SSID and start connection
        m_connected_ssid = ssid;
        m_wifi_state = wifi_state::connecting;
        update_status("Connecting to " + ssid);

        // Configure WiFi
        wifi_config_t wifi_config = {};
        strcpy((char*)wifi_config.sta.ssid, ssid.c_str());
        strcpy((char*)wifi_config.sta.password, password.c_str());
        wifi_config.sta.scan_method = WIFI_FAST_SCAN;
        wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

        esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        if (err != ESP_OK) {
            m_wifi_state = wifi_state::failed;
            update_status("WiFi config failed");
            return;
        }

        err = esp_wifi_connect();
        if (err != ESP_OK) {
            m_wifi_state = wifi_state::failed;
            update_status("Connect failed");
        }
    }


    void wifi_screen::update_status(const char* text)   { update_status(std::string(text)); }


    void wifi_screen::update_status(const std::string& text) {
        if (m_status_label) {
            std::string truncated = text;
            if (truncated.length() > 21) {
                truncated.resize(18);
                truncated += "...";
            }
            lv_label_set_text(m_status_label, truncated.c_str());
        }
    }


    void wifi_screen::process_ui_queue() {

        ui_update_msg msg;
        while (xQueueReceive(m_ui_queue, &msg, 0) == pdTRUE)
            update_status(msg.text);
    }



    void wifi_screen::check_ntp_sync() {

        if (!m_ntp_started)
            return;

        // Check if NTP sync has completed
        if (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)
            return;

        // NTP sync completed – get current time and update clock
        time_t now = 0;
        struct tm timeinfo = {};
        time(&now);
        localtime_r(&now, &timeinfo);      // assuming local timezone set via TZ

        APP::clock new_time;
        new_time.hours   = timeinfo.tm_hour;
        new_time.minutes = timeinfo.tm_min;
        new_time.seconds = timeinfo.tm_sec;

        APP::UI::main_screen::set_clock(new_time);
        ESP_LOGI(TAG, "Clock updated via NTP: %02d:%02d:%02d", new_time.hours, new_time.minutes, new_time.seconds);

        // We only need to do this once per connection
        m_ntp_started = false;
        // Optionally show a one‑time status
        update_status("Time synchronized");
    }


    void wifi_screen::stop_wifi() {

        if (m_wifi_state == wifi_state::connected || m_wifi_state == wifi_state::connecting)
            esp_wifi_disconnect();

        // Stop NTP if running
        if (m_ntp_started) {
            esp_sntp_stop();
            m_ntp_started = false;
        }

        esp_wifi_stop();
        esp_wifi_deinit();
        s_wifi_initialized = false;
        m_wifi_state = wifi_state::idle;
        ESP_LOGI(TAG, "Wi-Fi disabled");
    }


    // Static callbacks ------------------------------------------------------------------------------------------------

    void wifi_screen::scan_btn_event_cb(lv_event_t* e) {

        wifi_screen* self = (wifi_screen*)lv_event_get_user_data(e);
        self->start_scan();
    }


    void wifi_screen::connect_btn_event_cb(lv_event_t* e) {

        wifi_screen* self = (wifi_screen*)lv_event_get_user_data(e);
        self->on_connect_clicked();
    }


    void wifi_screen::wifi_event_handler(void* arg, esp_event_base_t event_base,
                                         int32_t event_id, void* event_data) {

        wifi_screen* self = static_cast<wifi_screen*>(arg);

        if (event_base == WIFI_EVENT) {
            switch (event_id) {

                case WIFI_EVENT_STA_CONNECTED: {
                    self->m_wifi_state = wifi_state::connected;
                    ui_update_msg msg;
                    snprintf(msg.text, sizeof(msg.text), "WiFi: %.15s", self->m_connected_ssid.c_str());
                    xQueueSend(self->m_ui_queue, &msg, 0);
                    break;
                }

                case WIFI_EVENT_STA_DISCONNECTED: {
                    wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*)event_data;
                    self->m_wifi_state = wifi_state::disconnected;
                    esp_wifi_disconnect();

                    // Provide a human‑readable reason
                    const char* reason_str = "Disconnected";
                    switch (disconnected->reason) {
                        case WIFI_REASON_AUTH_EXPIRE:                    reason_str = "Auth expired"; break;
                        case WIFI_REASON_AUTH_LEAVE:                     reason_str = "Auth leave"; break;
                        case WIFI_REASON_DISASSOC_DUE_TO_INACTIVITY:     reason_str = "Inactive"; break;
                        case WIFI_REASON_ASSOC_TOOMANY:                  reason_str = "Too many stations"; break;
                        case WIFI_REASON_CLASS2_FRAME_FROM_NONAUTH_STA:  reason_str = "Class2 non‑auth"; break;
                        case WIFI_REASON_CLASS3_FRAME_FROM_NONASSOC_STA: reason_str = "Class3 non‑assoc"; break;
                        case WIFI_REASON_ASSOC_LEAVE:                    reason_str = "Assoc leave"; break;
                        case WIFI_REASON_ASSOC_NOT_AUTHED:               reason_str = "Not authenticated"; break;
                        case WIFI_REASON_DISASSOC_PWRCAP_BAD:            reason_str = "Bad power cap"; break;
                        case WIFI_REASON_DISASSOC_SUPCHAN_BAD:           reason_str = "Bad channel"; break;
                        case WIFI_REASON_BSS_TRANSITION_DISASSOC:        reason_str = "BSS transition"; break;
                        case WIFI_REASON_IE_INVALID:                     reason_str = "Invalid IE"; break;
                        case WIFI_REASON_MIC_FAILURE:                    reason_str = "MIC failure"; break;
                        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:         reason_str = "4‑way timeout"; break;
                        case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:       reason_str = "Group key timeout"; break;
                        case WIFI_REASON_IE_IN_4WAY_DIFFERS:             reason_str = "IE mismatch"; break;
                        case WIFI_REASON_GROUP_CIPHER_INVALID:           reason_str = "Bad group cipher"; break;
                        case WIFI_REASON_PAIRWISE_CIPHER_INVALID:        reason_str = "Bad pair cipher"; break;
                        case WIFI_REASON_AKMP_INVALID:                   reason_str = "Bad AKMP"; break;
                        case WIFI_REASON_UNSUPP_RSN_IE_VERSION:          reason_str = "Bad RSN version"; break;
                        case WIFI_REASON_INVALID_RSN_IE_CAP:             reason_str = "Bad RSN caps"; break;
                        case WIFI_REASON_802_1X_AUTH_FAILED:             reason_str = "802.1X auth fail"; break;
                        case WIFI_REASON_CIPHER_SUITE_REJECTED:          reason_str = "Cipher rejected"; break;
                        case WIFI_REASON_TDLS_PEER_UNREACHABLE:          reason_str = "TDLS no peer"; break;
                        case WIFI_REASON_TDLS_UNSPECIFIED:               reason_str = "TDLS error"; break;
                        case WIFI_REASON_SSP_REQUESTED_DISASSOC:         reason_str = "SSP disassoc"; break;
                        case WIFI_REASON_NO_SSP_ROAMING_AGREEMENT:       reason_str = "No SSP roaming"; break;
                        case WIFI_REASON_BAD_CIPHER_OR_AKM:              reason_str = "Bad cipher/AKM"; break;
                        case WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION:   reason_str = "Not authorized"; break;
                        case WIFI_REASON_SERVICE_CHANGE_PERCLUDES_TS:    reason_str = "Service changed"; break;
                        case WIFI_REASON_UNSPECIFIED_QOS:                reason_str = "QoS error"; break;
                        case WIFI_REASON_NOT_ENOUGH_BANDWIDTH:           reason_str = "Low bandwidth"; break;
                        case WIFI_REASON_MISSING_ACKS:                   reason_str = "Missing ACKs"; break;
                        case WIFI_REASON_EXCEEDED_TXOP:                  reason_str = "TXOP exceeded"; break;
                        case WIFI_REASON_STA_LEAVING:                    reason_str = "STA leaving"; break;
                        case WIFI_REASON_END_BA:                         reason_str = "End BA"; break;
                        case WIFI_REASON_UNKNOWN_BA:                     reason_str = "Unknown BA"; break;
                        case WIFI_REASON_TIMEOUT:                        reason_str = "Timeout"; break;
                        case WIFI_REASON_PEER_INITIATED:                 reason_str = "Peer disassoc"; break;
                        case WIFI_REASON_AP_INITIATED:                   reason_str = "AP disassoc"; break;
                        case WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT:  reason_str = "Bad FT count"; break;
                        case WIFI_REASON_INVALID_PMKID:                  reason_str = "Bad PMKID"; break;
                        case WIFI_REASON_INVALID_MDE:                    reason_str = "Bad MDE"; break;
                        case WIFI_REASON_INVALID_FTE:                    reason_str = "Bad FTE"; break;
                        case WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED: reason_str = "Link est. failed"; break;
                        case WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED:    reason_str = "Alt chan busy"; break;
                        case WIFI_REASON_BEACON_TIMEOUT:                 reason_str = "Beacon timeout"; break;
                        case WIFI_REASON_NO_AP_FOUND:                    reason_str = "No AP found"; break;
                        case WIFI_REASON_AUTH_FAIL:                      reason_str = "Auth failed"; break;
                        case WIFI_REASON_ASSOC_FAIL:                     reason_str = "Assoc failed"; break;
                        case WIFI_REASON_HANDSHAKE_TIMEOUT:              reason_str = "Handshake timeout"; break;
                        case WIFI_REASON_CONNECTION_FAIL:                reason_str = "Conn failed"; break;
                        case WIFI_REASON_AP_TSF_RESET:                   reason_str = "AP TSF reset"; break;
                        case WIFI_REASON_ROAMING:                        reason_str = "Roaming"; break;
                        case WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG:   reason_str = "Comeback too long"; break;
                        case WIFI_REASON_SA_QUERY_TIMEOUT:               reason_str = "SAQ timeout"; break;
                        case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY: reason_str = "No AP: security"; break;
                        case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD: reason_str = "No AP: auth mode"; break;
                        case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:  reason_str = "No AP: RSSI"; break;
                        default: break;
                    }
                    ui_update_msg msg;
                    snprintf(msg.text, sizeof(msg.text), "Failed: %.12s", reason_str);
                    xQueueSend(self->m_ui_queue, &msg, 0);
                    break;
                }

                default:
                    break;
            }
        }
    }


    void wifi_screen::ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {

        wifi_screen* self = static_cast<wifi_screen*>(arg);

        if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* got_ip = (ip_event_got_ip_t*)event_data;
            char ip_str[16];
            esp_ip4addr_ntoa(&got_ip->ip_info.ip, ip_str, sizeof(ip_str));

            self->m_wifi_state = wifi_state::connected;
            ui_update_msg msg;
            snprintf(msg.text, sizeof(msg.text), "WiFi: %s", self->m_connected_ssid.c_str());
            xQueueSend(self->m_ui_queue, &msg, 0);

            // ---- Start NTP synchronisation (only if not already started) ----
            if (!self->m_ntp_started) {
                // Set timezone to UTC (or your local TZ)
                setenv("TZ", "UTC-0", 1);
                tzset();

                sntp_setoperatingmode(SNTP_OPMODE_POLL);
                sntp_setservername(0, "pool.ntp.org");
                sntp_init();
                self->m_ntp_started = true;
                ESP_LOGI(TAG, "NTP client started");
            }
        }
    }

}
