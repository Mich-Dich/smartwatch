
#include "util/pch.hpp"
#include "RTC.hpp"

#include "driver/i2c_master.h"



// FORWARD DECLARATIONS ================================================================================================

namespace APP::RTC {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    constexpr u8 PCF8563_ADDR        = 0x51;
    constexpr u8 PCF8563_REG_CTRL1   = 0x00;
    constexpr u8 PCF8563_REG_SECONDS = 0x02;

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    static i2c_master_dev_handle_t rtc_dev_handle = nullptr;

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    static inline u8 bin2bcd(u8 val);
    static inline u8 bcd2bin(u8 val);

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    // BCD helpers
    static inline u8 bin2bcd(u8 val) { return ((val / 10) << 4) | (val % 10); }
    static inline u8 bcd2bin(u8 val) { return (val >> 4) * 10 + (val & 0x0F); }

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================
    
    bool init() {

        i2c_master_bus_handle_t bus_handle = bsp_i2c_get_handle();
        if (!bus_handle) {
            ESP_LOGE("RTC", "Failed to get I2C bus handle");
            return false;
        }

        // Add the RTC device to the bus
        i2c_device_config_t dev_cfg = {};
        dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        dev_cfg.device_address = PCF8563_ADDR;
        dev_cfg.scl_speed_hz = 100000;   // PCF8563 works fine at 100 kHz
        dev_cfg.scl_wait_us = 0;        // default

        esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &rtc_dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGE("RTC", "Failed to add RTC device: %s", esp_err_to_name(ret));
            return false;
        }

        // Clear the STOP bit (normal mode)
        u8 ctrl1[2] = { PCF8563_REG_CTRL1, 0x00 };
        ret = i2c_master_transmit(rtc_dev_handle, ctrl1, sizeof(ctrl1), -1);
        return ret == ESP_OK;
    }

    
    bool set_time(const struct tm* timeinfo) {

        if (!rtc_dev_handle) return false;

        u8 buf[8] = {
            PCF8563_REG_SECONDS,
            bin2bcd(timeinfo->tm_sec),
            bin2bcd(timeinfo->tm_min),
            bin2bcd(timeinfo->tm_hour),
            bin2bcd(timeinfo->tm_mday),
            bin2bcd(timeinfo->tm_wday),
            bin2bcd(timeinfo->tm_mon + 1),
            bin2bcd((timeinfo->tm_year + 1900) % 100)
        };
        return i2c_master_transmit(rtc_dev_handle, buf, sizeof(buf), -1) == ESP_OK;
    }

    
    bool get_time(struct tm* timeinfo) {

        if (!rtc_dev_handle) return false;

        u8 reg = PCF8563_REG_SECONDS;
        u8 buf[7];
        esp_err_t ret = i2c_master_transmit_receive(rtc_dev_handle, &reg, 1, buf, sizeof(buf), -1);
        if (ret != ESP_OK) {
            ESP_LOGE("RTC", "Read error: %s", esp_err_to_name(ret));
            return false;
        }

        timeinfo->tm_sec  = bcd2bin(buf[0] & 0x7F);
        timeinfo->tm_min  = bcd2bin(buf[1] & 0x7F);
        timeinfo->tm_hour = bcd2bin(buf[2] & 0x3F);
        timeinfo->tm_mday = bcd2bin(buf[3] & 0x3F);
        timeinfo->tm_wday = buf[4] & 0x07;
        timeinfo->tm_mon  = bcd2bin(buf[5] & 0x1F) - 1;
        timeinfo->tm_year = bcd2bin(buf[6]) + 100;  // assume 2000‑2099
        return true;
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
