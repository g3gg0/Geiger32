
#include "Config.h"
#include "Macros.h"

#define PWM_LEDC 1
#define PWM_GPIO 27

#define PWM_BITS 9
#define PWM_PCT(x) ((uint32_t) ((100.0f-x) * ((1UL << (PWM_BITS)) - 1) / 100.0f))


float pwm_value = 0;
float pwm_deviation = 0;
uint32_t pwm_freq = 0;
uint32_t pwm_test_active = 0;
long pwm_next_check = 0;
long pwm_confirm_start = 0;
bool pwm_confirmed = false;


void pwm_setup()
{
    digitalWrite(PWM_GPIO, LOW);
    pinMode(PWM_GPIO, OUTPUT);

    Serial.printf("[i]     Frequency       %d Hz\n", current_config.pwm_freq);
    Serial.printf("[i]     PWM resolution  %d Bits\n", PWM_BITS);
    Serial.printf("[i]     Startup freq    %d Hz\n", current_config.pwm_freq);
    Serial.printf("[i]     Startup duty    %2.2f %%\n", current_config.pwm_value);
    pwm_value = current_config.pwm_value;
    pwm_freq = current_config.pwm_freq;

    ledcAttachPin(PWM_GPIO, PWM_LEDC);
    ledcSetup(PWM_LEDC, current_config.pwm_freq, PWM_BITS);
    ledcWrite(PWM_LEDC, PWM_PCT(pwm_value));

    pwm_next_check = millis() + 500;

    pwm_confirmed = false;
    pwm_confirm_start = millis();

    adc_reset_voltage();
}

void pwm_testmode(uint32_t state)
{
    if(pwm_test_active && !state)
    {
        pwm_setup();
    }

    pwm_test_active = state;
}

void pwm_test(uint32_t frequency, float duty)
{
    if(pwm_test_active)
    {
        pwm_value = duty;
        ledcSetup(PWM_LEDC, frequency, PWM_BITS);
        ledcWrite(PWM_LEDC, PWM_PCT(pwm_value));
        adc_reset_voltage();
    }
}

bool pwm_is_stable()
{
    double deltaVoltage = current_config.voltage_target - adc_voltage_avg;

    return fabs(deltaVoltage) < 5;
}

bool pwm_loop()
{
    /* safety check high prio */
    if(adc_voltage_avg > current_config.voltage_max)
    {
        char msg[128];

        pwm_value = 0;
        ledcWrite(PWM_LEDC, PWM_PCT(0));

        sprintf(msg, "[PWM] Voltage %2.2f V avg (%2.2f V last sample) > %2.2f V, shutdown. PWM %d Hz", adc_voltage_avg, adc_voltage, current_config.voltage_max, pwm_freq);
        mqtt_publish_string((char *)"feeds/string/%s/error", msg);
        Serial.println(msg);
        
        return false;
    }

    if(pwm_test_active)
    {
        return false;
    }
    
    /* run every 500ms (also have a startup delay) */
    if((millis() < pwm_next_check) || (pwm_value == 0))
    {
        return false;
    }

    pwm_next_check = millis() + 500;

    if(current_config.verbose & 1)
    {
        Serial.printf("[PWM] Voltage %2.2f V (%2.2f V averaged)\n", adc_voltage, adc_voltage_avg);
    }

    /* safety check low prio, averaged */
    if(adc_voltage_avg < current_config.voltage_min)
    {
        char msg[128];

        pwm_value = 0;
        ledcWrite(PWM_LEDC, PWM_PCT(0));

        sprintf(msg, "[PWM] Voltage %2.2f V avg (%2.2f V last sample) < %2.2f V, shutdown", adc_voltage_avg, adc_voltage, current_config.voltage_min);
        mqtt_publish_string((char *)"feeds/string/%s/error", msg);
        Serial.println(msg);
    }

    /* emergency disable active? */
    if(pwm_value == 0)
    {
        ledcWrite(PWM_LEDC, PWM_PCT(0));
        return false;
    }

    /* determine deviation */
    double deltaVoltage = current_config.voltage_target - adc_voltage_avg;

    pwm_deviation *= 0.9f;
    pwm_deviation = (7 * pwm_deviation + 1 * (deltaVoltage*deltaVoltage)) / 8;

    /* calc I-term */
    double deltaFreq = -deltaVoltage * current_config.pwm_pid_i;

    /* saturate I-term */
    coerce(deltaFreq, -1000, 1000);

    /* integrate */
    pwm_freq += deltaFreq;

    /* saturate PWM frequency */
    coerce(pwm_freq, current_config.pwm_freq_min, current_config.pwm_freq_max);

    /* write PWM frequency */
    ledcSetup(PWM_LEDC, pwm_freq, PWM_BITS);

    if(current_config.verbose & 1)
    {
        Serial.printf("[PWM] %2.2f V vs. %2.2f, PWM => %d Hz, deviation %1.2f\n", adc_voltage_avg, current_config.voltage_target, pwm_freq, pwm_deviation);
    }

    /* startup phase, check for stabilization */
    if(!pwm_confirmed)
    {
        long expired = millis() - pwm_confirm_start;

        led_set_inhibit(false);

        for(int pos = 0; pos < 6; pos++)
        {
            led_set_adv(pos, 0, 0, 255, pos == 5);
        }

        /* wait at least 1 second */
        if(expired >= 1000)
        {
            /* voltage within 10 V tolerance */
            if(fabsf(deltaVoltage) < 10)
            {
                pwm_confirmed = true;

                if(!config_valid)
                {
                    rtttl_play("Halloween:d=4, o=5, b=180:8d6, 8g, 8g, 8d6, 8g, 8g, 8d6, 8g, 8d#6, 8g, 8d6, 8g, 8g, 8d6, 8g, 8g, 8d6, 8g, 8d#6, 8g, 8c#6, 8f#, 8f#, 8c#6, 8f#, 8f#, 8c#6, 8f#, 8d6, 8f#, 8c#6, 8f#, 8f#, 8c#6, 8f#, 8f#, 8c#6, 8f#, 8d6, 8f#");
                }
                else
                {
                    buzz_beep(1000, 150);
                    buzz_beep(1500, 150);
                    buzz_beep(2500, 500);
                }
                Serial.printf("[PWM] Voltage came up after %d seconds\n", expired / 1000);
            }
            else
            {
                Serial.printf("[PWM] Voltage not up\n");

                /* and up to 20 seconds if it gets stable */
                if(expired >= 20000)
                {
                    char msg[128];

                    for(int pos = 0; pos < 6; pos++)
                    {
                        led_set(pos, 255, 0, 0);
                    }
                    pwm_confirmed = true;

                    buzz_beep(2500, 500);
                    buzz_beep(1500, 500);
                    buzz_beep(1000, 750);

                    pwm_value = 0;
                    ledcWrite(PWM_LEDC, PWM_PCT(0));

                    sprintf(msg, "[PWM] Voltage didn't come up properly after %d seconds (%2.2f V)", expired / 1000, adc_voltage_avg);
                    mqtt_publish_string((char *)"feeds/string/%s/error", msg);
                    Serial.println(msg);

                    delay(500);
                }
            }
        }

        for(int pos = 0; pos < 6; pos++)
        {
            led_set_adv(pos, 0, 0, 0, pos == 5);
        }
        
        led_set_inhibit(!pwm_confirmed);
    }
    
    return !pwm_is_stable();
}
