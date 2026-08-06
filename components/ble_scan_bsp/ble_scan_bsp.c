#include <stdio.h>
#include "ble_scan_bsp.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

// FORWARD DECLARATIONS ================================================================================================

static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

// TYPES ===============================================================================================================

// CONSTANTS ===========================================================================================================

// MACROS ==============================================================================================================

#define GATTC_TAG                       "GATTC_DEMO"

#define REMOTE_SERVICE_UUID             0x00FF

#define REMOTE_NOTIFY_CHAR_UUID         0xFF01

#define PROFILE_NUM                     1

#define PROFILE_A_APP_ID                0

#define INVALID_HANDLE                  0

// VARIABLES ===========================================================================================================

EventGroupHandle_t                      ble_Even;

QueueHandle_t                           ble_Queue;

// STATIC VARIABLES ====================================================================================================

static esp_ble_scan_params_t            ble_scan_params = {
    .scan_type                          = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type                      = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy                 = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval                      = 0x50,
    .scan_window                        = 0x30,
    .scan_duplicate                     = BLE_SCAN_DUPLICATE_ENABLE
};

struct gattc_profile_inst {
    esp_gattc_cb_t                      gattc_cb;
    uint16_t                            gattc_if;
    uint16_t                            app_id;
    uint16_t                            conn_id;
    uint16_t                            service_start_handle;
    uint16_t                            service_end_handle;
    uint16_t                            char_handle;
    esp_bd_addr_t                       remote_bda;
};

static struct gattc_profile_inst        gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,
    },
};

static bool                             s_scanning = false;           // scanning state
static bool                             s_advertising = false;        // advertising state
static bool                             s_adv_config_pending = false; // waiting for config complete
static esp_ble_adv_params_t             s_adv_params;                 // stored params
static uint16_t                         s_service_uuid;   // persistent storage for advertising service UUID
static uint16_t                         s_conn_id = 0;
static uint16_t                         s_gattc_if = ESP_GATT_IF_NONE;
static esp_bd_addr_t                    s_connected_bda = {0};
static bool                             s_connected = false;

// Pending advertising request (if scanning needs to be stopped first)
static struct {
    bool                                pending;
    char                                name[BLE_NAME_MAX_LEN];
    uint16_t                            service_uuid;
} s_adv_request = {0};

static const char*                      ADVERTISING_TAG = "BLE_ADV";

// INTERNAL TEMPLATE DECLARATION =======================================================================================

// INTERNAL FUNCTION DECLARATION =======================================================================================

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

// INTERNAL TEMPLATE IMPLEMENTATION ====================================================================================

// INTERNAL FUNCTION IMPLEMENTATION ====================================================================================

static void start_advertising_internal(const char* name, uint16_t service_uuid) {

    // If a config is already pending, don't start another one
    if (s_adv_config_pending) {
        ESP_LOGW(ADVERTISING_TAG, "Adv config already pending, ignoring new request");
        return;
    }

    // Store the UUID in a persistent location
    s_service_uuid = service_uuid;

    // Set device name
    esp_ble_gap_set_device_name(name);

    // Build advertising data – use persistent pointer for p_service_uuid
    esp_ble_adv_data_t adv_data = {};
    adv_data.set_scan_rsp = false;
    adv_data.include_name = true;
    adv_data.include_txpower = true;
    adv_data.min_interval = 0x20;
    adv_data.max_interval = 0x40;
    adv_data.appearance = 0x00;
    adv_data.manufacturer_len = 0;
    adv_data.p_manufacturer_data = NULL;
    adv_data.service_data_len = 0;
    adv_data.p_service_data = NULL;
    adv_data.service_uuid_len = sizeof(uint16_t);
    adv_data.p_service_uuid = (uint8_t*)&s_service_uuid;  // persistent address
    adv_data.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);

    esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
    if (ret != ESP_OK) {
        ESP_LOGE(ADVERTISING_TAG, "config adv data failed, error code = %x", ret);
        return;
    }

    // Store advertising parameters for later use
    s_adv_params = (esp_ble_adv_params_t){
        .adv_int_min = 0x20,
        .adv_int_max = 0x40,
        .adv_type = ADV_TYPE_IND,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .peer_addr = {0},
        .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };

    s_adv_config_pending = true;
    ESP_LOGI(ADVERTISING_TAG, "Advertising data config sent, waiting for complete event");
}


static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
    
    switch (event) {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
            break;

        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
            if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
                break;
            }
            s_scanning = true;
            ESP_LOGI(GATTC_TAG, "scan start success");
            break;
        }

        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
            switch (scan_result->scan_rst.search_evt) {

                case ESP_GAP_SEARCH_INQ_RES_EVT: {
                    ble_device_t dev;
                    memcpy(dev.bda, scan_result->scan_rst.bda, 6);
                    dev.name[0] = '\0';
                    uint8_t name_len = 0;
                    uint8_t* name_ptr = NULL;

                    // Try complete name, then short name from advertising data
                    name_ptr = esp_ble_resolve_adv_data(
                        scan_result->scan_rst.ble_adv,
                        ESP_BLE_AD_TYPE_NAME_CMPL,
                        &name_len
                    );
                    if (name_ptr == NULL) {
                        name_ptr = esp_ble_resolve_adv_data(
                            scan_result->scan_rst.ble_adv,
                            ESP_BLE_AD_TYPE_NAME_SHORT,
                            &name_len
                        );
                    }

                    if (name_ptr && name_len > 0) {
                        size_t copy_len = (name_len < BLE_NAME_MAX_LEN - 1) ? name_len : BLE_NAME_MAX_LEN - 1;
                        memcpy(dev.name, name_ptr, copy_len);
                        dev.name[copy_len] = '\0';
                    }

                    if (xQueueSend(ble_Queue, &dev, 0) == pdTRUE) {
                        // item sent
                    }

                    ESP_LOGI("SCAN", "Device found, MAC %02x:%02x:%02x:%02x:%02x:%02x, name: %s",
                        dev.bda[0], dev.bda[1], dev.bda[2], dev.bda[3], dev.bda[4], dev.bda[5],
                        dev.name[0] ? dev.name : "(none)");
                    break;
                }

                case ESP_GAP_SEARCH_INQ_CMPL_EVT:
                    s_scanning = false;
                    ESP_LOGI(GATTC_TAG, "Scan finished naturally");
                    break;

                default:
                    break;
            }
            break;
        }

        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
            if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTC_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
                break;
            }
            s_scanning = false;
            ESP_LOGI(GATTC_TAG, "stop scan successfully");

            // If we have a pending advertising request, start it now
            if (s_adv_request.pending) {
                s_adv_request.pending = false;
                start_advertising_internal(s_adv_request.name, s_adv_request.service_uuid);
            }
            break;
        }

        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT: {
            if (param->adv_data_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(ADVERTISING_TAG, "adv data set failed, status = %d", param->adv_data_cmpl.status);
                s_adv_config_pending = false;
                break;
            }
            ESP_LOGI(ADVERTISING_TAG, "adv data set complete, starting advertising");
            s_adv_config_pending = false;
            esp_err_t ret = esp_ble_gap_start_advertising(&s_adv_params);
            if (ret != ESP_OK) {
                ESP_LOGE(ADVERTISING_TAG, "start advertising failed, error code = %x", ret);
            }
            break;
        }

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT: {
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(ADVERTISING_TAG, "advertising start failed, status = %d", param->adv_start_cmpl.status);
                break;
            }
            s_advertising = true;
            ESP_LOGI(ADVERTISING_TAG, "Advertising started successfully");
            break;
        }

        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT: {
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(ADVERTISING_TAG, "advertising stop failed, status = %d", param->adv_stop_cmpl.status);
                break;
            }
            s_advertising = false;
            s_adv_request.pending = false;   // clear any pending request
            ESP_LOGI(ADVERTISING_TAG, "Advertising stopped");
            break;
        }

        default:
            break;
    }
}


static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param) {

    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {                   // 当GATT客户端注册时，事件发生
        if (param->reg.status == ESP_GATT_OK) {         // 获取运行状态
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if; // param->reg.app_id获取ID,就是注册时的那个gattc_if是系统自动生成
            printf("gattc_if:%d\n",gattc_if);

        } else {

            ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d",param->reg.app_id,param->reg.status); // 否则输出对应的错误结构
            return;                                     //返回,不执行后面的
        }
        
    } do {

        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || gattc_if == gl_profile_tab[idx].gattc_if) { //如果回调报告 gattc_if/gatts_if 为ESP_GATT_IF_NONE宏，则表示此事件不对应任何应用程序
                if (gl_profile_tab[idx].gattc_cb) 
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param); //调用回调函数 
            }
        }
    } while (0);
}


static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param) {  // 该函数由esp_gattc_cb回调函数调用而具备执行机会

    //esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;
    switch (event) {

        case ESP_GATTC_REG_EVT: //通过esp_gattc_cb回调函数执行的这个回调函数传入事件参数
            ESP_LOGI(GATTC_TAG, "REG_EVT");
            //esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params); //设置扫描参数
            //if (scan_ret)
            //{
            //    ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
            //}
            break;

        case ESP_GATTC_CFG_MTU_EVT: //当 GATT 客户端通过调用 esp_ble_gattc_set_mtu() 函数成功配置了 MTU 时，系统会触发此事件
            if (param->cfg_mtu.status != ESP_GATT_OK)
                ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);

            ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
            break;

        case ESP_GATTC_OPEN_EVT:
            if (param->open.status != ESP_GATT_OK) {
                ESP_LOGE(GATTC_TAG, "Open failed, status %d", param->open.status);
                break;
            }
            s_conn_id = param->open.conn_id;
            ESP_LOGI(GATTC_TAG, "Connection open, conn_id %d", s_conn_id);
            // Optionally set MTU
            esp_ble_gattc_send_mtu_req(gattc_if, s_conn_id);
            break;

        case ESP_GATTC_CONNECT_EVT:
            s_connected = true;
            ESP_LOGI(GATTC_TAG, "Connected to " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(param->connect.remote_bda));
            // You may want to start service discovery here
            // esp_ble_gattc_search_service(gattc_if, s_conn_id, NULL);
            break;

        case ESP_GATTC_DISCONNECT_EVT:
            s_connected = false;
            s_conn_id = 0;
            ESP_LOGI(GATTC_TAG, "Disconnected");
            break;

        case ESP_GATTC_SEARCH_CMPL_EVT:
            // Service discovery completed – you can now read/write characteristics
            ESP_LOGI(GATTC_TAG, "Service search complete");
            break;
            
        default:
            break;
    }
}

// TEMPLATE IMPLEMENTATION =============================================================================================

// FUNCTION IMPLEMENTATION =============================================================================================

void ble_scan_mem_init(void) {
    ble_Even = xEventGroupCreate();
    ble_Queue = xQueueCreate(20, sizeof(ble_device_t));
}


void ble_scan_class_init(void) {

    ble_scan_mem_init();
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
}


void ble_scan_Init(void) {

    ble_scan_class_init(); 
    esp_err_t ret;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
        return;
    }
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if(ret) {
        ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
        return;
    }
    ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
    if (ret)
        ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret)
        ESP_LOGE(GATTC_TAG, "set local MTU failed, error code = %x", local_mtu_ret);
}


void ble_scan_setconf(void) {

    esp_ble_gap_set_scan_params(&ble_scan_params);
    esp_ble_gap_start_scanning(3);         // 3 seconds scan
}


void ble_scan_Deinit(void) {

    if (s_scanning)
        esp_ble_gap_stop_scanning();

    if (s_advertising)
        esp_ble_gap_stop_advertising();

    esp_ble_gattc_app_unregister(gl_profile_tab[0].gattc_if);
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
}


esp_err_t ble_connect_to_device(uint8_t *bda) {

    if (s_scanning) {

        ESP_LOGW(GATTC_TAG, "Scanning active, stop first");
        esp_ble_gap_stop_scanning();
        // Optionally wait for stop event, but for simplicity we can try anyway
    }

    // Use the registered GATT interface
    if (gl_profile_tab[PROFILE_A_APP_ID].gattc_if == ESP_GATT_IF_NONE) {

        ESP_LOGE(GATTC_TAG, "GATT client not registered");
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t ret = esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, 
        bda, BLE_ADDR_TYPE_PUBLIC, true);  // direct connection

    if (ret == ESP_OK) {
        memcpy(s_connected_bda, bda, 6);
        ESP_LOGI(GATTC_TAG, "Connection attempt to " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bda));
    }
    return ret;
}


esp_err_t ble_disconnect(void) {

    if (!s_connected)
        return ESP_ERR_INVALID_STATE;

    return esp_ble_gattc_close(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, s_conn_id);
}


bool ble_is_connected(void)         { return s_connected; }


void ble_advertising_start(const char* name, uint16_t service_uuid) {

    if (s_advertising) {
        ESP_LOGI(ADVERTISING_TAG, "Already advertising");
        return;
    }

    // If we already have a pending config, just update the stored request and return
    if (s_adv_config_pending) {
        strncpy(s_adv_request.name, name, BLE_NAME_MAX_LEN - 1);
        s_adv_request.name[BLE_NAME_MAX_LEN - 1] = '\0';
        s_adv_request.service_uuid = service_uuid;
        s_adv_request.pending = true;
        ESP_LOGW(ADVERTISING_TAG, "Adv config pending, request stored for later");
        return;
    }

    // If scanning is active, stop it first and defer advertising start
    if (s_scanning) {
        ESP_LOGI(ADVERTISING_TAG, "Scanning active, stopping scan first");
        strncpy(s_adv_request.name, name, BLE_NAME_MAX_LEN - 1);
        s_adv_request.name[BLE_NAME_MAX_LEN - 1] = '\0';
        s_adv_request.service_uuid = service_uuid;
        s_adv_request.pending = true;
        esp_ble_gap_stop_scanning();
        return;
    }

    start_advertising_internal(name, service_uuid);         // Otherwise start directly
}


void ble_advertising_stop(void) {

    if (!s_advertising)
        return;

    esp_err_t ret = esp_ble_gap_stop_advertising();
    if (ret != ESP_OK)
        ESP_LOGE(ADVERTISING_TAG, "stop advertising failed, error code = %x", ret);
    // The flag will be cleared in the ADV_STOP_COMPLETE_EVT
}


bool ble_is_advertising(void)       { return s_advertising; }

// CLASS IMPLEMENTATION ================================================================================================

// CLASS PUBLIC ========================================================================================================

// CLASS PROTECTED =====================================================================================================

// CLASS PRIVATE =======================================================================================================
