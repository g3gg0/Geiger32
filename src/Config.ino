
#include <FS.h>
#include <SPIFFS.h>

#include "Config.h"

t_cfg current_config;

void cfg_save()
{
    File file = SPIFFS.open("/config.dat", "w");
    if (!file || file.isDirectory())
    {
        return;
    }

    if (strlen(current_config.hostname) < 2)
    {
        strcpy(current_config.hostname, "Geigerv3");
    }

    file.write((uint8_t *)&current_config, sizeof(current_config));
    file.close();
}

void cfg_reset()
{
    memset(&current_config, 0x00, sizeof(current_config));

    current_config.magic = CONFIG_MAGIC;
    strcpy(current_config.hostname, "Geiger");

    current_config.adc_corr = 1.0f;
    current_config.voltage_target = 380;
    current_config.voltage_min = 200;
    current_config.voltage_max = 450;
    current_config.voltage_avg = 512;
    current_config.pwm_freq = 22000;
    current_config.pwm_start = 1;
    current_config.pwm_min = 1;
    current_config.pwm_max = 90;
    current_config.idle_color = 0;
    current_config.elevated_color = 0xFF0000;
    current_config.flash_color = 0xFFFFFF;
    current_config.elevated_level = 100;
    current_config.buzz_length = 20;
    current_config.buzz_freq = 1000;
    current_config.verbose = 7;

    cfg_save();
}

void cfg_read()
{
    File file = SPIFFS.open("/config.dat", "r");

    if (!file || file.isDirectory())
    {
        cfg_reset();
    }
    else
    {
        file.read((uint8_t *)&current_config, sizeof(current_config));
        file.close();

        if (current_config.magic != CONFIG_MAGIC)
        {
            cfg_reset();
        }
    }
}
