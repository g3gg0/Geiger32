
#include "Config.h"

#define BUZZER_LEDC 0
#define BUZZER_GPIO 32

void buz_setup()
{
    pinMode(BUZZER_GPIO, OUTPUT);
    digitalWrite(BUZZER_GPIO, LOW);
}

bool buz_loop()
{
    return false;
}

void buz_tick()
{
    uint32_t target_millis = millis() + current_config.buzz_length;

    for (int loop = 0; millis() < target_millis; loop++)
    {
        digitalWrite(BUZZER_GPIO, (loop & 1) ? LOW : HIGH);
        delayMicroseconds(1000000 / current_config.buzz_freq);
    }
    digitalWrite(BUZZER_GPIO, LOW);
}
