
#include "util/pch.hpp"
#include "system.hpp"

#include "esp_adc/adc_oneshot.h"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::system {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // Voltage difference between non charge and charge. not totally accurate because of non linear curve, but good enough
    static constexpr f32                        CHARGING_VOLTAGE_OFFSET = 0.485f;

    // MACROS ==========================================================================================================

    #define ADC_Calibrate

    // STATIC VARIABLES ================================================================================================

    #ifdef ADC_Calibrate
        static adc_cali_handle_t                cali_handle;
    #endif

    static adc_oneshot_unit_handle_t            adc1_handle;

    static volatile bool                        s_charger_connected = false;

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    void adc_init(void);

    void adc_get_value(f32* value/*, i32* data*/);

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    void adc_init(void) {

        #ifdef ADC_Calibrate
            adc_cali_curve_fitting_config_t cali_config =  {
                .unit_id = ADC_UNIT_1,
                .chan = ADC_CHANNEL_0,          // your channel
                .atten = ADC_ATTEN_DB_12,
                .bitwidth = ADC_BITWIDTH_12, //4096
            };
            ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));
        #endif

        adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT_1,
            .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
            .ulp_mode = ADC_ULP_MODE_DISABLE
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
        adc_oneshot_chan_cfg_t config = {
            .atten = ADC_ATTEN_DB_12,//ADC_ATTEN_DB_12,         //    1.1          ADC_ATTEN_DB_12:3.3
            .bitwidth = ADC_BITWIDTH_12,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config));
    }


    void adc_get_value(f32* value/*, i32* data*/) {

        int adc_data;
        #ifdef ADC_Calibrate
            int vol = 0;
        #endif
        esp_err_t err;
        err = adc_oneshot_read(adc1_handle,ADC_CHANNEL_3,&adc_data);
        if(err == ESP_OK) {

            #ifdef ADC_Calibrate
                adc_cali_raw_to_voltage(cali_handle,adc_data,&vol);
                *value = 0.001 * vol * 3;
            #else
                *value = ((f32)adc_data * 3.3/4096) * 3;
            #endif
            // *data = adc_data;

        } else {

            *value = 0;
            // *data = 0;
        }
    }

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    void init() {

        adc_init();
    }


    void get_battery_voltage(f32& adjusted, f32& raw_voltage) {

        // i32 adc_data = 0;
        f32 raw = 0;
        adc_get_value(&raw/*, &adc_data*/);

        adjusted = raw;
        raw_voltage = raw;

        // If charger is connected, the measured voltage includes the charger's contribution.
        // Subtract an estimated offset to get the actual battery voltage.
        if (is_charger_connected()) {
            adjusted -= CHARGING_VOLTAGE_OFFSET;
            if (adjusted < 0)
                adjusted = 0;
        }
    }


    u8 get_battery_voltage_in_percent() {

        // Table: {voltage (V), percent (%)} – sorted by voltage ascending
        static constexpr std::array<std::pair<f32, f32>, 9> table = {{
            {3.00f,  0.0f},
            {3.30f,  5.0f},
            {3.50f, 20.0f},
            {3.65f, 40.0f},
            {3.75f, 55.0f},
            {3.85f, 70.0f},
            {3.97f, 85.0f},
            {4.03f, 93.0f},
            {4.10f,100.0f}
        }};

        f32 adjusted{};
        f32 raw_voltage{};
        get_battery_voltage(adjusted, raw_voltage);

        if (adjusted <= table[0].first)                      // Clamp to table range
            return 0;

        if (adjusted >= table[table.size() - 1].first)
            return 100;

        for (size_t i = 0; i < table.size() - 1; ++i) {     // Find the segment containing voltage
            if (adjusted >= table[i].first && adjusted < table[i+1].first) {

                const f32 frac = (adjusted - table[i].first) / (table[i+1].first - table[i].first);
                const f32 percent = table[i].second + frac * (table[i+1].second - table[i].second);
                return static_cast<u8>(std::clamp(percent, 0.0f, 100.0f));
            }
        }
        return 100; // fallback
    }


    bool is_charger_connected()                         { return s_charger_connected; }


    void set_charger_connected(const bool connected)    { s_charger_connected = connected; }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
