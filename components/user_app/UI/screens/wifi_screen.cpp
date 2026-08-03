
#include "util/pch.hpp"
#include "wifi_screen.hpp"



// FORWARD DECLARATIONS ================================================================================================

namespace APP::UI {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    static const char* TAG = "wifi_screen";

    static bool s_wifi_initialized = false;

    std::vector<wifi_screen::KnownNetwork>       wifi_screen::s_known_networks = {
        {"Happy",                   "Kerstin321!"},
        {"FRITZ!Repeater 3000",     "frosh#5"}
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

        // Title
        lv_obj_t* title = lv_label_create(m_screen);
        lv_label_set_text(title, "WiFi Settings");
        lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(title, &inconsolata_regular_26, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

        // Scan button – now below title, top-right but with y=45
        m_scan_btn = lv_btn_create(m_screen);
        lv_obj_set_size(m_scan_btn, 80, 30);
        lv_obj_align(m_scan_btn, LV_ALIGN_TOP_RIGHT, -10, 45);
        lv_obj_set_style_bg_color(m_scan_btn, lv_color_hex(0x0066FF), 0);
        lv_obj_add_event_cb(m_scan_btn, scan_btn_event_cb, LV_EVENT_CLICKED, this);
        lv_obj_t* scan_label = lv_label_create(m_scan_btn);
        lv_label_set_text(scan_label, "Scan");
        lv_obj_center(scan_label);

        // Status label – now at bottom
        m_status_label = lv_label_create(m_screen);
        lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_text_font(m_status_label, &inconsolata_regular_26, 0);
        lv_label_set_text(m_status_label, "Press Scan to find networks");
        lv_obj_align(m_status_label, LV_ALIGN_BOTTOM_MID, 0, -10);

        // List – adjust vertical position to fit between scan button and status label
        // We'll give it percentage height, but we can also align to center with top and bottom margins
        m_list = lv_list_create(m_screen);
        lv_obj_set_size(m_list, LV_PCT(95), LV_PCT(70));   // reduced height to leave room for bottom status
        lv_obj_align(m_list, LV_ALIGN_CENTER, 0, 10);      // slight center offset

        lv_obj_set_style_bg_color(m_list, lv_color_hex(0x111111), 0);
        lv_obj_set_style_border_width(m_list, 0, 0);
        lv_obj_set_style_pad_all(m_list, 5, 0);
        lv_obj_set_style_pad_row(m_list, 2, 0);

        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "WiFi screen initialised");
    }


    void wifi_screen::show() {

        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
        start_scan();   // auto‑scan when shown
    }


    void wifi_screen::hide() {

        lv_obj_add_flag(m_screen, LV_OBJ_FLAG_HIDDEN);
    }


    void wifi_screen::destroy() {

        if (m_screen) {
            lv_obj_del(m_screen);
            m_screen = nullptr;
        }
        m_networks.clear();
    }


    bool wifi_screen::handle_event(lv_event_t* e) {

        // Not used – events are handled via static callbacks
        return false;
    }

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

    void wifi_screen::start_scan() {
        
        if (m_is_scanning) 
            return;

        static bool s_wifi_initialized = false;
        if (!s_wifi_initialized) {
            ble_scan_Deinit();
            espwifi_Init();
            esp_wifi_set_mode(WIFI_MODE_STA);
            s_wifi_initialized = true;
        }

        m_is_scanning = true;
        update_status("Scanning...");
        lv_obj_clean(m_list);
        m_networks.clear();

        esp_wifi_scan_start(nullptr, true);
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
            // Convert SSID to string and trim whitespace
            std::string ssid_str = reinterpret_cast<const char*>(rec.ssid);
            // Trim leading/trailing spaces
            ssid_str.erase(0, ssid_str.find_first_not_of(" \t\n\r\f\v"));
            ssid_str.erase(ssid_str.find_last_not_of(" \t\n\r\f\v") + 1);

            // Skip empty or hidden networks
            if (ssid_str.empty()) {
                ESP_LOGD(TAG, "Skipping empty SSID (hidden network)");
                continue;
            }

            NetworkItem item;
            item.ssid = ssid_str;
            m_networks.push_back(std::move(item));
        }

        populate_list();
        update_status("Scan complete");
        m_is_scanning = false;
    }


    void wifi_screen::populate_list() {
        
        lv_obj_clean(m_list);
        for (size_t i = 0; i < m_networks.size(); ++i) {
            auto& item = m_networks[i];

            // Container – now uses flex column layout
            lv_obj_t* cont = lv_obj_create(m_list);
            lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_color(cont, lv_color_hex(0x222222), 0);
            lv_obj_set_style_border_width(cont, 0, 0);
            lv_obj_set_style_pad_all(cont, 5, 0);
            lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
            // Enable flex layout, column direction, align children to start
            lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            item.cont = cont;

            // Clickable button (SSID) – now a flex child
            lv_obj_t* btn = lv_btn_create(cont);
            lv_obj_set_size(btn, LV_PCT(100), 40);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x444444), LV_STATE_PRESSED);
            lv_obj_set_style_radius(btn, 6, 0);
            lv_obj_add_event_cb(btn, list_item_event_cb, LV_EVENT_CLICKED, this);
            lv_obj_set_user_data(btn, (void*)i);

            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_text(label, item.ssid.c_str());
            lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
            lv_obj_center(label);

            // Expandable container – now below the button due to flex column
            lv_obj_t* expand_cont = lv_obj_create(cont);
            lv_obj_set_size(expand_cont, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_color(expand_cont, lv_color_hex(0x1A1A1A), 0);
            lv_obj_set_style_border_width(expand_cont, 0, 0);
            lv_obj_set_style_pad_all(expand_cont, 8, 0);
            lv_obj_add_flag(expand_cont, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(expand_cont, LV_OBJ_FLAG_SCROLLABLE);
            // Use flex row for the buttons inside expand_cont
            lv_obj_set_flex_flow(expand_cont, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(expand_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            item.expand_cont = expand_cont;

            // "Connect" button
            lv_obj_t* connect_btn = lv_btn_create(expand_cont);
            lv_obj_set_size(connect_btn, 100, 30);
            lv_obj_set_style_bg_color(connect_btn, lv_color_hex(0x00AA00), 0);
            lv_obj_add_event_cb(connect_btn, connect_btn_event_cb, LV_EVENT_CLICKED, this);
            lv_obj_set_user_data(connect_btn, (void*)i);
            lv_obj_t* c_label = lv_label_create(connect_btn);
            lv_label_set_text(c_label, "Connect");
            lv_obj_center(c_label);
            item.connect_btn = connect_btn;

            // "Forget password" button
            lv_obj_t* forget_btn = lv_btn_create(expand_cont);
            lv_obj_set_size(forget_btn, 130, 30);
            lv_obj_set_style_bg_color(forget_btn, lv_color_hex(0xAA0000), 0);
            lv_obj_add_event_cb(forget_btn, forget_btn_event_cb, LV_EVENT_CLICKED, this);
            lv_obj_set_user_data(forget_btn, (void*)i);
            lv_obj_t* f_label = lv_label_create(forget_btn);
            lv_label_set_text(f_label, "Forget");
            lv_obj_center(f_label);
            item.forget_btn = forget_btn;

            m_networks[i] = std::move(item);
        }
    }


    void wifi_screen::on_item_clicked(size_t index) {

        if (index >= m_networks.size())
            return;
        toggle_expand(index);
    }


    void wifi_screen::toggle_expand(size_t index) {

        auto& item = m_networks[index];
        item.expanded = !item.expanded;
        if (item.expanded)
            lv_obj_clear_flag(item.expand_cont, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(item.expand_cont, LV_OBJ_FLAG_HIDDEN);

        lv_obj_update_layout(m_list);                       // Refresh list layout (optional)
    }


    void wifi_screen::on_connect_clicked(size_t index) {

        if (index >= m_networks.size())
            return;
        
        const std::string& ssid = m_networks[index].ssid;

        // Search for password in static list
        std::string password;
        bool found = false;
        ESP_LOGI(TAG, "Searching for network: %s", ssid.c_str());

        for (const auto& known : s_known_networks) {

            ESP_LOGI(TAG, "Known network: %s", known.ssid.c_str());
            if (known.ssid == ssid) {
                password = known.password;
                found = true;
                break;
            }
        }

        if (!found) {
            update_status("passwd not found for " + ssid);
            return;
        }

        // Configure WiFi
        wifi_config_t wifi_config = {};
        strcpy((char*)wifi_config.sta.ssid, ssid.c_str());
        strcpy((char*)wifi_config.sta.password, password.c_str());
        wifi_config.sta.scan_method = WIFI_FAST_SCAN;
        wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

        esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        if (err != ESP_OK) {
            update_status("WiFi config failed");
            return;
        }

        err = esp_wifi_connect();
        if (err == ESP_OK)
            update_status("Connecting to " + ssid + "...");
        else
            update_status("Connect failed");
    }


    void wifi_screen::on_forget_clicked(size_t index) {

        if (index >= m_networks.size())
            return;

        const std::string& ssid = m_networks[index].ssid;

        // Remove password from known list
        auto it = std::find_if(s_known_networks.begin(), s_known_networks.end(),
            [&](const KnownNetwork& kn) { return kn.ssid == ssid; });
        if (it != s_known_networks.end()) {
            s_known_networks.erase(it);
            update_status("Forgot password for " + ssid);
        } else
            update_status("No stored password for " + ssid);
    }


    void wifi_screen::update_status(const char* text) {

        if (m_status_label)
            lv_label_set_text(m_status_label, text);
    }


    void wifi_screen::update_status(const std::string& text) {

        update_status(text.c_str());
    }

    // -----------------------------------------------------------------------------
    // Static callbacks

    void wifi_screen::list_item_event_cb(lv_event_t* e) {

        lv_obj_t* btn = lv_event_get_target(e);
        wifi_screen* self = (wifi_screen*)lv_event_get_user_data(e);
        size_t index = (size_t)lv_obj_get_user_data(btn);
        self->on_item_clicked(index);
    }


    void wifi_screen::connect_btn_event_cb(lv_event_t* e) {

        lv_obj_t* btn = lv_event_get_target(e);
        wifi_screen* self = (wifi_screen*)lv_event_get_user_data(e);
        size_t index = (size_t)lv_obj_get_user_data(btn);
        self->on_connect_clicked(index);
    }


    void wifi_screen::forget_btn_event_cb(lv_event_t* e) {

        lv_obj_t* btn = lv_event_get_target(e);
        wifi_screen* self = (wifi_screen*)lv_event_get_user_data(e);
        size_t index = (size_t)lv_obj_get_user_data(btn);
        self->on_forget_clicked(index);
    }


    void wifi_screen::scan_btn_event_cb(lv_event_t* e) {

        ESP_LOGI(TAG, "Scan button clicked");
        wifi_screen* self = (wifi_screen*)lv_event_get_user_data(e);
        self->start_scan();
    }

}
