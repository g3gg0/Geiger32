
#include <FS.h>
#include <SPIFFS.h>

#include "Config.h"

t_cfg current_config;
bool config_valid = false;

void cfg_save()
{
    File file = SPIFFS.open("/config.dat", "w");
    if (!file || file.isDirectory())
    {
        return;
    }

    if (strlen(current_config.hostname) < 2)
    {
        strcpy(current_config.hostname, CONFIG_OTANAME);
    }

    file.write((uint8_t *)&current_config, sizeof(current_config));
    file.close();
}

void cfg_reset()
{
    memset(&current_config, 0x00, sizeof(current_config));

    current_config.magic = CONFIG_MAGIC;
    strcpy(current_config.hostname, CONFIG_OTANAME);
    strcpy(current_config.mqtt_server, "");
    current_config.mqtt_port = 11883;
    strcpy(current_config.mqtt_user, "");
    strcpy(current_config.mqtt_password, "");
    strcpy(current_config.mqtt_client, CONFIG_OTANAME);
    current_config.mqtt_publish = 0;

    current_config.adc_corr = 1.0f;
    current_config.conv_usv_per_bq = 1.0f;
    current_config.voltage_target = 380;
    current_config.voltage_min = 100;
    current_config.voltage_max = 450;
    current_config.voltage_avg = 512;
    current_config.pwm_pid_i = 30;
    current_config.pwm_freq = 30000;
    current_config.pwm_freq_min = 22000;
    current_config.pwm_freq_max = 40000;
    current_config.pwm_value = 80;
    current_config.idle_color = 0;
    current_config.elevated_color = 0xFF0000;
    current_config.flash_color = 0xFFFFFF;
    current_config.elevated_level = 100;
    current_config.buzz_length = 20;
    current_config.buzz_freq = 1000;
    current_config.verbose = 7;

    strcpy(current_config.wifi_ssid, "(not set)");
    strcpy(current_config.wifi_password, "(not set)");
}

void cfg_read()
{
    File file = SPIFFS.open("/config.dat", "r");

    config_valid = false;

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
            /* on a minor version change, just keep wifi settings and hostname */
            if ((current_config.magic & ~0xF) == (CONFIG_MAGIC & ~0xF))
            {
                char hostname[32];
                char wifi_ssid[32];
                char wifi_password[32];

                strcpy(hostname, current_config.hostname);
                strcpy(wifi_ssid, current_config.wifi_ssid);
                strcpy(wifi_password, current_config.wifi_password);

                cfg_reset();

                strcpy(current_config.hostname, hostname);
                strcpy(current_config.wifi_ssid, wifi_ssid);
                strcpy(current_config.wifi_password, wifi_password);
                config_valid = true;
            }
            else
            {
                cfg_reset();
            }
            return;
        }
        config_valid = true;
    }
}
