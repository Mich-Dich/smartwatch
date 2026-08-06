
#include "util/pch.hpp"
#include "system.hpp"

#include "esp_adc/adc_oneshot.h"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::system {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    #define ADC_Calibrate

    // STATIC VARIABLES ================================================================================================

    #ifdef ADC_Calibrate
        static adc_cali_handle_t                cali_handle;
    #endif

    static adc_oneshot_unit_handle_t            adc1_handle;

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


    f32 get_battery_voltage() {

        // i32 adc_data = 0;
        f32 voltage = 0;
        adc_get_value(&voltage/*, &adc_data*/);
        return voltage;
    }


    u8 get_battery_voltage_in_percent() {

        // Table: {voltage (V), percent (%)} – sorted by voltage ascending
        static constexpr std::pair<f32, f32> table[] = {
            {3.00f,  0.0f},
            {3.30f,  5.0f},
            {3.50f, 20.0f},
            {3.65f, 40.0f},
            {3.75f, 55.0f},
            {3.85f, 70.0f},
            {3.98f, 85.0f},
            {4.05f, 93.0f},
            {4.12f,100.0f}
        };
        constexpr size_t N = sizeof(table) / sizeof(table[0]);

        f32 voltage = get_battery_voltage();

        // Clamp to table range
        if (voltage <= table[0].first)
            return 0;

        if (voltage >= table[N-1].first)
            return 100;

        // Find the segment containing voltage
        for (size_t i = 0; i < N - 1; ++i) {
            if (voltage >= table[i].first && voltage < table[i+1].first) {

                f32 frac = (voltage - table[i].first) / (table[i+1].first - table[i].first);
                f32 percent = table[i].second + frac * (table[i+1].second - table[i].second);
                return static_cast<u8>(std::clamp(percent, 0.0f, 100.0f));
            }
        }
        return 100; // fallback
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
