#include "battery.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "board_jc3636k518c.h"

static adc_oneshot_unit_handle_t battery_adc;
static adc_cali_handle_t battery_calibration;

void battery_init(void)
{
    adc_oneshot_unit_init_cfg_t adc_config = {.unit_id = ADC_UNIT_1};
    if (adc_oneshot_new_unit(&adc_config, &battery_adc) == ESP_OK) {
        adc_oneshot_chan_cfg_t channel_config = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT};
        adc_oneshot_config_channel(battery_adc, ADC_CHANNEL_0, &channel_config);
        adc_cali_curve_fitting_config_t calibration_config = {
            .unit_id = ADC_UNIT_1, .chan = ADC_CHANNEL_0, .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        adc_cali_create_scheme_curve_fitting(&calibration_config, &battery_calibration);
    }
}

int battery_estimate(void)
{
    int raw = 0, pin_mv = 0;
    if (battery_adc && adc_oneshot_read(battery_adc, ADC_CHANNEL_0, &raw) == ESP_OK &&
        battery_calibration && adc_cali_raw_to_voltage(battery_calibration, raw, &pin_mv) == ESP_OK) {
        int battery_mv = pin_mv * 2;
        int percent = (battery_mv - 3300) * 100 / 900;
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        return percent;
    }
    return -1;
}
