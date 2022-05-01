

#include "Config.h"

#define ADC_GPIO 34

//#define R3 (14700000.0f)
#define R3 (13800000.0f) /* the 14.7M falls in resistance with raising current */
#define R4 (82000.0f)
#define RSUM (R3 + R4)

#define ADC_VADC(adc_raw) ((adc_raw)*3.3f / 4096)
#define ADC_VHV(v_adc) ((v_adc)*RSUM / R4 * current_config.adc_corr)

float adc_raw = 0;
float adc_vadc = 0.0f;
float adc_voltage = 0.0f;
float adc_voltage_avg = 0.0f;

void adc_setup()
{
    analogReadResolution(12);
}

float adc_get_voltage()
{
    return adc_voltage_avg;
}

bool adc_loop()
{
    float raw = 0;
    for (int sample = 0; sample < current_config.voltage_avg; sample++)
    {
        raw += analogRead(ADC_GPIO);
    }
    adc_raw = raw / current_config.voltage_avg;
    adc_vadc = ADC_VADC(adc_raw);
    adc_voltage = ADC_VHV(adc_vadc);

    if(adc_voltage_avg > 1.0f)
    {
        adc_voltage_avg = (3 * adc_voltage_avg + adc_voltage) / 4;
    }
    else
    {
        adc_voltage_avg = adc_voltage;
    }

    /*
      Serial.printf("raw:%2.0f,", adc_raw);
      Serial.printf("Vadc:%2.2f,", adc_vadc);
      Serial.printf("Vhv:%2.2f,", adc_voltage);
      Serial.printf("Vhvavg:%2.2f", adc_voltage_avg);
      Serial.printf("\n");
      */
    return true;
}
