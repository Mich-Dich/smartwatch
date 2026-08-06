#pragma once

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"



// FORWARD DECLARATIONS =====================================================================================

extern EventGroupHandle_t ble_Even;
extern QueueHandle_t ble_Queue;

// CONSTANTS ================================================================================================

#define BLE_NAME_MAX_LEN    64

// MACROS ===================================================================================================

// TYPES ====================================================================================================

typedef struct {

    uint8_t     bda[6];
    char        name[BLE_NAME_MAX_LEN];
} ble_device_t;

// STATIC VARIABLES =========================================================================================

// FUNCTION DECLARATION =====================================================================================

void ble_scan_mem_init(void);

void ble_scan_class_init(void);

void ble_scan_Init(void);

void ble_scan_setconf(void);

void ble_scan_Deinit(void);

esp_err_t ble_connect_to_device(uint8_t* bda);

esp_err_t ble_disconnect(void);

bool ble_is_connected(void);

void ble_advertising_start(const char* name, uint16_t service_uuid);

void ble_advertising_stop(void);

bool ble_is_advertising(void);

// TEMPLATE DECLARATION =====================================================================================

// CLASS DECLARATION ========================================================================================
