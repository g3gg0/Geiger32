
#include "Config.h"

#define BUZZER_LEDC 4
#define BUZZER_GPIO 32

#define PWM_BITS 9
#define PWM_PCT(x) ((uint32_t) ((100.0f-x) * ((1UL << (PWM_BITS)) - 1) / 100.0f))

void buz_setup()
{
    pinMode(BUZZER_GPIO, OUTPUT);
    digitalWrite(BUZZER_GPIO, LOW);
    ledcAttachPin(BUZZER_GPIO, BUZZER_LEDC);
    //ledcSetup(BUZZER_LEDC, current_config.buzz_freq, 12);
    //ledcWrite(BUZZER_LEDC, 0);
}

bool buz_loop()
{
    return false;
}

void buzz_on(uint32_t freq)
{
    ledcSetup(BUZZER_LEDC, freq, 12);
    ledcWrite(BUZZER_LEDC, PWM_PCT(50));
}

void buzz_off()
{
    ledcWrite(BUZZER_LEDC, 0);
}

void buzz_beep(uint32_t freq, uint32_t duration)
{
    buzz_on(freq);
    delay(duration);
    buzz_off();
}

void buz_tick()
{
    buzz_beep(current_config.buzz_freq, current_config.buzz_length);
}
