
#include "Config.h"

#define PWM_LEDC 1
#define PWM_GPIO 27

#define PWM_BITS 11
#define PWM_PCT(x) ((uint32_t) ((100.0f-x) * ((1UL << (PWM_BITS)) - 1) / 100.0f))

float pwm_value = 50;

long pwm_last_time = 0;
float pwm_learn_max_voltage = 0;
float pwm_learn_max_value = 0;

int pwm_learn_state = 0;

void pwm_setup()
{
    digitalWrite(PWM_GPIO, LOW);
    pinMode(PWM_GPIO, OUTPUT);

    Serial.printf("[i]     Frequency       %d Hz\n", current_config.pwm_freq);
    Serial.printf("[i]     PWM resolution  %d Bits\n", PWM_BITS);
    Serial.printf("[i]     Startup duty    %2.2f %%\n", current_config.pwm_start);
    pwm_value = current_config.pwm_start;

    ledcAttachPin(PWM_GPIO, PWM_LEDC);
    ledcSetup(PWM_LEDC, current_config.pwm_freq, PWM_BITS);
    ledcWrite(PWM_LEDC, PWM_PCT(pwm_value));

    //adc_voltage_avg = 0;
    pwm_learn_state = 0;
    pwm_last_time = millis();
}

bool pwm_learning()
{
    return pwm_learn_state != 0;
}

void pwm_learn()
{
    //pwm_value = 1;
    pwm_learn_max_voltage = 0;
    pwm_learn_max_value = 0;
    pwm_learn_state = 1;
}

bool pwm_loop()
{
    /* safety check high prio */
    if (adc_voltage > current_config.voltage_max)
    {
        pwm_value = 0;
        Serial.printf("[PWM] Voltage %2.2f > %2.2f, shutdown\n", adc_voltage, current_config.voltage_max);
        ledcWrite(PWM_LEDC, PWM_PCT(0));
        return false;
    }


    /* run every 500ms (also have a startup delay) */
    if ((millis() - pwm_last_time < 500) || (pwm_value == 0))
    {
        return false;
    }

    pwm_last_time = millis();

    if(current_config.verbose & 1)
    {
        Serial.printf("[PWM] Voltage %2.2f V (%2.2f V averaged)\n", adc_voltage, adc_voltage_avg);
    }

    /* safety check low prio, averaged */
    if (adc_voltage_avg < current_config.voltage_min && !pwm_learn_state)
    {
        pwm_value = 0;
        Serial.printf("[PWM] Voltage %2.2f < %2.2f, shutdown\n", adc_voltage_avg, current_config.voltage_min);
    }

    /* emergency disable active? */
    if (pwm_value == 0)
    {
        ledcWrite(PWM_LEDC, PWM_PCT(0));
        return false;
    }

    if(pwm_learn_state == 1)
    {
        if(adc_voltage_avg >= current_config.voltage_target)
        {
            Serial.printf("[PWM] Peak: Voltage %2.2f at %2.0f %% - DONE\n", pwm_learn_max_voltage, pwm_learn_max_value);
            pwm_value = pwm_learn_max_value;
            pwm_learn_state = 0;
            current_config.pwm_start = pwm_learn_max_value;
            cfg_save();
            buz_tick();
            pwm_setup();
        }
        else
        {
            if(adc_voltage_avg > pwm_learn_max_voltage)
            {
                Serial.printf("[PWM] Voltage %2.2f <- new maximum at %2.0f %%\n", adc_voltage, pwm_value);
                pwm_learn_max_voltage = adc_voltage_avg;
                pwm_learn_max_value = pwm_value;
            }
            pwm_value++;
        }

        if(pwm_value > 80)
        {
            pwm_learn_state = 0;
            ledcWrite(PWM_LEDC, PWM_PCT(0));
            return false;
        }
    }

    if (adc_voltage_avg < 0.95f * current_config.voltage_target)
    {
        if(pwm_value < current_config.pwm_max)
        {
            pwm_value += 0.05f;
        }
        Serial.printf("[PWM] %2.2f V < %2.2f, PWM => %2.2f\n", adc_voltage_avg, current_config.voltage_target, pwm_value);
    }
    if (adc_voltage_avg > 1.05f * current_config.voltage_target)
    {
        if(pwm_value > current_config.pwm_min)
        {
            pwm_value -= 0.05f;
        }
        Serial.printf("[PWM] %2.2f V > %2.2f, PWM => %2.2f\n", adc_voltage_avg, current_config.voltage_target, pwm_value);
    }
    
    pwm_value = min(pwm_value, (float)current_config.pwm_max);
    pwm_value = max(pwm_value, (float)current_config.pwm_min);

    ledcWrite(PWM_LEDC, PWM_PCT(pwm_value));

    return false;
}
